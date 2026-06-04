#define NOMINMAX
#include "ChargeShot.h"
#include "Engine/Input/Input.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Enemy/Enemy.h"
#include <Frame.h>
#include <Particle/CSParticle/ParticleCSEditor.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;
void ChargeShot::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    BaseObject::SetTexture("debug/white1x1.png");
    BaseObject::SetColor({0.0f, 0.5f, 1.0f, 1.0f});

    transform_->translation_ = player_->GetWorldPosition();

    bulletCollider_ = AddSphereCollider("ChargeShot_Collider");
    bulletCollider_->SetTag("PlayerChargeBullet");
    bulletCollider_->AddCollisionMask("Enemy");
    bulletCollider_->SetRadius(scale_);
    bulletCollider_->SetEnabled(false);

    bulletCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnterCallback(other);
    });

    isAlive_ = false;
    isMaxScale_ = false;
    isFired_ = false;
    scale_ = kInitialScale;
    velocity_ = {0, 0, 0};
    // 初期位置もリセット
    chargeEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("chargeEmitter");
    bulletEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chageBullet");
}

void ChargeShot::Update() {
    Input *input = Input::GetInstance();

    if (isAlive_) {
        if (bulletCollider_) {
            bulletCollider_->SetEnabled(true);
            bulletCollider_->SetRadius(scale_);
        }

        // 弾エミッターの更新と位置・スケール設定
        bulletEmitter_->Update();
        bulletEmitter_->SetPosition(transform_->translation_);
        bulletEmitter_->SetStartScale("chageBullet", transform_->scale_ * kBulletParticleScaleMultiplier);
        if (!isMaxScale_) {
            bulletEmitter_->SetStartScale("chageAround", {(kParticleScaleBase + scale_) * kParticleScaleMultiplier,
                                                          (kParticleScaleBase + scale_) * kParticleScaleMultiplier,
                                                          (kParticleScaleBase + scale_) * kParticleScaleMultiplier});
            bulletEmitter_->SetEndScale("chageAround", {(kParticleScaleBase + kParticleEndScaleOffset + scale_) * kParticleScaleMultiplier,
                                                        (kParticleScaleBase + kParticleEndScaleOffset + scale_) * kParticleScaleMultiplier,
                                                        (kParticleScaleBase + kParticleEndScaleOffset + scale_) * kParticleScaleMultiplier});
        }
    }

    bool chargeHold = false;
    bool chargeRelease = false;

    if (!player_->GetGamePad()->IsConnected()) {
        // キーボード入力
        chargeHold = input->PushKey(DIK_K);
        chargeRelease = input->ReleaseMomentKey(DIK_K);
    } else {
        // ゲームパッド入力 - Yボタン
        chargeHold = player_->GetGamePad()->IsPress(XINPUT_GAMEPAD_Y);
        chargeRelease = player_->GetGamePad()->IsRelease(XINPUT_GAMEPAD_Y);
    }

    // ガード中はチャージショットを溜め・発射できない（発射済みの弾はそのまま飛ばす）
    if (player_ && player_->IsGuarding()) {
        chargeHold = false;
        chargeRelease = false;
    }

    if (!isAlive_ && !isSkillMenu_) {
        // チャージ開始判定：ボタンが押され続けている時間を計測
        if (chargeHold) {
            chargeStartTimer_ += Frame::DeltaTime();

            // kChargeStartThreshold以上押され続けた場合のみチャージ開始
            if (chargeStartTimer_ >= player_->GetChargeThreshold()) {
                // エネルギーチェック
                if (player_ && player_->GetEnergy() < kMinEnergyToStart) {
                    // エネルギー不足でチャージ開始できない
                    chargeStartTimer_ = 0.0f;
                    return;
                }
                isAlive_ = true;
                isFired_ = false;
                scale_ = kInitialScale;
                isMaxScale_ = false;
                isCharge_ = true;
            }
        } else {
            // ボタンが離されたらタイマーリセット
            chargeStartTimer_ = 0.0f;
        }
    } else {
        if (chargeHold && !isFired_) {
            scale_ += scaleSpeed_ * (Frame::DeltaTime());
            if (scale_ >= maxScale_) {
                scale_ = maxScale_;
                isMaxScale_ = true;
            }
            if (!isMaxScale_) {
                chargeEmitter_->Update();
            }
            isCharge_ = true;
        }
        if (chargeRelease && !isFired_) {
            // エネルギー消費量を計算(チャージ率に応じて5~20)
            float scaleRatio = (scale_ - kInitialScale) / (maxScale_ - kInitialScale);
            float energyCost = kMinEnergyCost + ((kMaxEnergyCost - kMinEnergyCost) * scaleRatio);

            // エネルギーチェック
            if (!player_->ConsumeEnergy(energyCost)) {
                // エネルギー不足ならリセット
                Reset();
                isCharge_ = false;
                chargeStartTimer_ = 0.0f;
                return;
            }

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
            isCharge_ = false;
            chargeStartTimer_ = 0.0f;
        }
    }

    // 発射後の移動処理
    if (isFired_ && isAlive_) {
        transform_->translation_.x += velocity_.x * (1.0f / 60.0f);
        transform_->translation_.y += velocity_.y * (1.0f / 60.0f);
        transform_->translation_.z += velocity_.z * (1.0f / 60.0f);

        // プレイヤーから一定距離離れたらリセット
        if ((transform_->translation_ - player_->GetLocalPosition()).Length() > kMaxDistance) {
            Reset();
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

            // 高さ(Y軸)オフセット
            offset.y = verticalOffset_;

            // チャージ弾の位置更新
            transform_->translation_ = playerPos + offset;

            // エミッター位置も更新
            chargeEmitter_->SetTranslate(transform_->translation_);

            Vector3 playerEuler = rot.ToEulerAngles();
            if (bulletCollider_) {
                bulletCollider_->SetEnabled(true);
            }
        }
    }
    // 階層的ワールド変換更新
    BaseObject::UpdateWorldTransformHierarchy();
}

void ChargeShot::Fire(const Vector3 &pos, const Vector3 &dir) {
    transform_->translation_ = pos;
    velocity_ = dir * speed_;
}

void ChargeShot::Reset() {

    if (bulletCollider_) {
        bulletCollider_->SetEnabled(false);
    }

    isAlive_ = false;
    isFired_ = false;
    scale_ = kInitialScale;
    isMaxScale_ = false;
    velocity_ = {0, 0, 0};
    transform_->translation_ = {0, 0, 0};
}

float ChargeShot::GetDamage() const {
    // スケールの割合を計算(1.0f~maxScale_の範囲を0.0f~1.0fに正規化)
    float scaleRatio = (scale_ - kInitialScale) / (maxScale_ - kInitialScale);

    // 割合に応じてダメージを計算(最小1ダメージは保証)
    float damage = kMaxDamage * scaleRatio;
    return std::max(kMinDamage, damage);
}

void ChargeShot::Draw(const ViewProjection &viewProjection) {
    if (!isAlive_)
        return;
    // スケールを反映
    transform_->scale_ = {scale_, scale_, scale_};
}
void ChargeShot::DrawParticleCompute(const ViewProjection &viewProjection) {
    chargeEmitter_->DrawCompute(viewProjection);
}

void ChargeShot::DrawParticle(const ViewProjection &viewProjection) {
    chargeEmitter_->DrawGraphics(viewProjection);
    if (!isAlive_)
        return;
    bulletEmitter_->Draw(viewProjection); // CPU emitter
}

void ChargeShot::imgui() {
}

void ChargeShot::OnCollisionEnterCallback(ColliderBase *other) {
    // Enemyタグを持つコライダーと衝突した場合
    if (other->GetTag() == "Enemy") {
        // プレイヤーの敵が存在し、生きている場合
        if (player_ && player_->GetEnemy() && player_->GetEnemy()->GetAlive()) {
            isAlive_ = false;

            // チャージ度合いに応じたダメージを計算して適用
            float damage = GetDamage();
            player_->GetEnemy()->SetDamage(damage);

            Reset();
        }
    }
}