#include "ChageShot.h"
#include "Engine/Input/Input.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Enemy/Enemy.h"
#include <algorithm>
#include <cmath>

// 前方ベクトルを回転値から算出（Y軸回転のみ考慮の例）
static Vector3 GetForwardFromRotation(const Vector3 &rotation) {
    float y = rotation.y;
    return Vector3(std::sinf(y), 0.0f, std::cosf(y));
}

void ChageShot::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    BaseObject::SetTexture("debug/white1x1.png");
    BaseObject::SetColor({0.0f, 0.5f, 1.0f, 1.0f});
    BaseObject::AddCollider();
    BaseObject::SetCollisionType(CollisionType::Sphere);
    isAlive_ = false;
    isMaxScale_ = false;
    isFired_ = false;
    scale_ = 1.0f;
    velocity_ = {0, 0, 0};
    // 初期位置もリセット
    transform_.translation_ = {0, 0, 0};

    chageEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chageEmitter");
    bulletEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chageBullet");
}

void ChageShot::Update() {
    Input *input = Input::GetInstance();
    if (isAlive_) {
        Collider::SetCollisionEnabled(true);
        bulletEmitter_->Update();
        bulletEmitter_->SetPosition(transform_.translation_);
        bulletEmitter_->SetStartScale("chageBullet", transform_.scale_ * 2.0f);
        if (!isMaxScale_) {
            bulletEmitter_->SetStartScale("chageAround", {(0.6f + scale_) * 1.4f, (0.6f + scale_) * 1.4f, (0.6f + scale_) * 1.4f});
            bulletEmitter_->SetEndScale("chageAround", {(0.8f + scale_) * 1.4f, (0.8f + scale_) * 1.4f, (0.8f + scale_) * 1.4f});
        }
    }

    if (isAlive_ && !isFired_) {
        // チャージ中はプレイヤーの前方にオフセットして配置
        if (player_) {
            Vector3 playerPos = player_->GetWorldPosition();
            Vector3 forwardDir = GetForwardFromRotation(player_->GetWorldRotation());

            // プレイヤーのサイズとチャージショットの現在のスケールを考慮
            float chargeRadius = scale_; // チャージショットの現在の半径
            float offsetDistance = playerRadius_ + chargeRadius + offsetMargin_;

            // 前方方向にオフセット
            Vector3 offset = forwardDir * offsetDistance;

            // Y軸オフセット（プレイヤーの中心付近に配置）
            offset.y = verticalOffset_;

            transform_.translation_ = playerPos + offset;
            chageEmitter_->SetPosition(transform_.translation_);
        }
    }

    if (!isAlive_) {
        if (input->TriggerKey(DIK_K)) {
            isAlive_ = true;
            isFired_ = false;
            scale_ = 1.0f;
            isMaxScale_ = false;
        }
    } else {
        if (input->PushKey(DIK_K) && !isFired_) {
            scale_ += scaleSpeed_ * (1.0f / 60.0f); // 60FPS前提
            if (scale_ >= maxScale_) {
                scale_ = maxScale_;
                isMaxScale_ = true;
            }
            if (!isMaxScale_) {
                chageEmitter_->Update();
            }
        }
        if (input->ReleaseMomentKey(DIK_K) && !isFired_) {
            // 発射方向決定
            Vector3 dir = {0, 0, 1};               // デフォルト前方
            Vector3 pos = transform_.translation_; // 現在のチャージショット位置から発射
            if (player_) {
                if (player_->GetIsLockOn() && player_->GetEnemy()) {
                    // ロックオン時は敵方向
                    dir = player_->GetEnemy()->GetWorldPosition() - pos;
                    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                    if (len > 0.0001f) {
                        dir.x /= len;
                        dir.y /= len;
                        dir.z /= len;
                    }
                } else {
                    // プレイヤーの回転から前方ベクトルを算出
                    dir = GetForwardFromRotation(player_->GetWorldRotation());
                }
            }
            Fire(pos, dir);
            isFired_ = true;
        }
    }

    // 発射後の移動
    if (isFired_ && isAlive_) {
        transform_.translation_.x += velocity_.x * (1.0f / 60.0f);
        transform_.translation_.y += velocity_.y * (1.0f / 60.0f);
        transform_.translation_.z += velocity_.z * (1.0f / 60.0f);
        if ((transform_.translation_ - player_->GetWorldPosition()).Length() > 300.0f) {
            Reset();
        }
    }
}

void ChageShot::Fire(const Vector3 &pos, const Vector3 &dir) {
    transform_.translation_ = pos;
    velocity_ = dir * speed_;
}

void ChageShot::Reset() {
    isAlive_ = false;
    isFired_ = false;
    scale_ = 1.0f;
    isMaxScale_ = false;
    velocity_ = {0, 0, 0};
    transform_.translation_ = {0, 0, 0};
    
}

void ChageShot::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    if (!isAlive_)
        return;
    // スケールを反映
    transform_.scale_ = {scale_, scale_, scale_};
    BaseObject::SetRadius(scale_);
    // BaseObject::Draw(viewProjection, offSet);
}
void ChageShot::DrawParticle(const ViewProjection &viewProjection) {
    chageEmitter_->Draw(viewProjection);
    bulletEmitter_->Draw(viewProjection);
}

void ChageShot::imgui() {
    // デバッグ用
}

void ChageShot::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<Enemy *>(other)) {
        isAlive_ = false;
        Collider::SetCollisionEnabled(false);
        Reset();
    }
}