#define NOMINMAX
#include "ChageShot.h"
#include "Engine/Input/Input.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Enemy/Enemy.h"
#include <Frame.h>
#include <algorithm>
#include <cmath>

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
    transform_->translation_ = player_->GetWorldPosition();
    Collider::SetCollisionEnabled(false);
    chageEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chageEmitter");
    bulletEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chageBullet");
}

void ChageShot::Update() {
    Input *input = Input::GetInstance();

    if (isAlive_) {
        Collider::SetCollisionEnabled(true);

        // 弾エミッターの更新と位置・スケール設定
        bulletEmitter_->Update();
        bulletEmitter_->SetPosition(transform_->translation_);
        bulletEmitter_->SetStartScale("chageBullet", transform_->scale_ * 2.0f);
        if (!isMaxScale_) {
            bulletEmitter_->SetStartScale("chageAround", {(0.6f + scale_) * 1.4f, (0.6f + scale_) * 1.4f, (0.6f + scale_) * 1.4f});
            bulletEmitter_->SetEndScale("chageAround", {(0.8f + scale_) * 1.4f, (0.8f + scale_) * 1.4f, (0.8f + scale_) * 1.4f});
        }
    }

    if (isAlive_ && !isFired_) {
        if (player_) {
            Vector3 playerPos = player_->GetLocalPosition();

            Quaternion rot = player_->GetLocalRotation();

            Vector3 baseForward = Vector3(0.0f, 0.0f, 1.0f);

            Vector3 forwardDir = rot * baseForward;

            forwardDir.x = -forwardDir.x;

            Vector3 normForward = forwardDir.Normalize();

            // チャージ弾のオフセット距離
            float chargeRadius = scale_;
            float offsetDistance = playerRadius_ + chargeRadius + offsetMargin_;

            // オフセット計算
            Vector3 offset = normForward * offsetDistance;

            // 高さ（Y軸）オフセット
            offset.y = verticalOffset_;

            // チャージ弾の位置更新
            transform_->translation_ = playerPos + offset;

            // エミッター位置も更新
            chageEmitter_->SetPosition(transform_->translation_);

            Vector3 playerEuler = rot.ToEulerAngles();
        }
    }

    if (!isAlive_) {
        if (input->TriggerKey(DIK_K)) {
            Collider::SetCollisionEnabled(true);
            isAlive_ = true;
            isFired_ = false;
            scale_ = 1.0f;
            isMaxScale_ = false;
        }
    } else {
        if (input->PushKey(DIK_K) && !isFired_) {
            scale_ += scaleSpeed_ * (Frame::DeltaTime());
            if (scale_ >= maxScale_) {
                scale_ = maxScale_;
                isMaxScale_ = true;
            }
            if (!isMaxScale_) {
                chageEmitter_->Update();
            }
        }
        if (input->ReleaseMomentKey(DIK_K) && !isFired_) {
            Vector3 dir = {0, 0, 1};
            Vector3 pos = transform_->translation_;

            if (player_) {
                if (player_->GetIsLockOn() && player_->GetEnemy()) {
                    // ロックオン時は敵方向に向けて発射
                    dir = player_->GetEnemy()->GetLocalPosition() - pos;
                    float len = dir.Length();
                    if (len > 0.0001f) {
                        dir = dir / len;
                    }
                } else {
                    // プレイヤーの回転をかけて発射方向を計算
                    dir = (player_->GetLocalRotation() * Vector3(0.0f, 0.0f, 1.0f)).Normalize();

                    dir.x = -dir.x;
                }
            }

            Fire(pos, dir);
            isFired_ = true;
        }
    }

    // 発射後の移動処理
    if (isFired_ && isAlive_) {
        transform_->translation_.x += velocity_.x * (1.0f / 60.0f);
        transform_->translation_.y += velocity_.y * (1.0f / 60.0f);
        transform_->translation_.z += velocity_.z * (1.0f / 60.0f);

        // プレイヤーから一定距離離れたらリセット
        if ((transform_->translation_ - player_->GetLocalPosition()).Length() > 300.0f) {
            Reset();
        }
    }

    // 階層的ワールド変換更新
    BaseObject::UpdateWorldTransformHierarchy();
}

void ChageShot::Fire(const Vector3 &pos, const Vector3 &dir) {
    transform_->translation_ = pos;
    velocity_ = dir * speed_;
}

void ChageShot::Reset() {
    isAlive_ = false;
    isFired_ = false;
    scale_ = 1.0f;
    isMaxScale_ = false;
    velocity_ = {0, 0, 0};
    transform_->translation_ = {0, 0, 0};
}

int ChageShot::GetDamage() const {
    // スケールの割合を計算（1.0f〜maxScale_の範囲を0.0f〜1.0fに正規化）
    float scaleRatio = (scale_ - 1.0f) / (maxScale_ - 1.0f);

    // 割合に応じてダメージを計算（最小1ダメージは保証）
    int damage = static_cast<int>(maxDamage_ * scaleRatio);
    return std::max(1, damage);
}

void ChageShot::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    if (!isAlive_)
        return;
    // スケールを反映
    transform_->scale_ = {scale_, scale_, scale_};
    BaseObject::SetRadius(scale_);
}
void ChageShot::DrawParticle(const ViewProjection &viewProjection) {
    chageEmitter_->Draw(viewProjection);
    bulletEmitter_->Draw(viewProjection);
}

void ChageShot::imgui() {
    // デバッグ用
}

void ChageShot::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<Enemy *>(other) && player_->GetEnemy()->GetAlive()) {
        isAlive_ = false;
        Collider::SetCollisionEnabled(false);

        // チャージ度合いに応じたダメージを計算して適用
        int damage = GetDamage();
        player_->GetEnemy()->SetDamage(damage);

        Reset();
    }
}