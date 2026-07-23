#define NOMINMAX
#include "ChargeShot.h"
#include "input/Input.h"
#include "particle/ParticleEditor.h"
#include "application/entity/enemy/Enemy.h"
#include <Frame.h>
#include <particle/gpu/ParticleCSSpawner.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;
void ChargeShot::Init(const std::string objectName)
{
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Sphere);
    BaseObject::SetTexture("debug/white1x1.png");
    BaseObject::SetColor({0.0f, 0.5f, 1.0f, 1.0f});

    transform_->translation_ = pPlayer_->GetWorldPosition();

    pBulletCollider_ = AddSphereCollider("ChargeShot_Collider");
    pBulletCollider_->SetTag("PlayerChargeBullet");
    pBulletCollider_->AddCollisionMask("Enemy");
    pBulletCollider_->AddCollisionMask("Ground");
    pBulletCollider_->SetRadius(scale_);
    pBulletCollider_->SetEnabled(false);

    pBulletCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnterCallback(other);
    });

    isAlive_ = false;
    isMaxScale_ = false;
    isFired_ = false;
    scale_ = kInitialScale;
    velocity_ = {0, 0, 0};
    // 初期位置もリセット。
    // chargeEmitter_ は GPU パーティクル（Spawn した実体の更新・描画はエンジンが自動で回す）。
    // bulletEmitter_ は別システムの CPU パーティクルなので従来どおり手動で駆動する。
    chargeEmitter_ = ParticleCSSpawner::GetInstance()->Spawn("chargeEmitter");
    if (chargeEmitter_)
    {
        // chargeEmitter テンプレートは自動発生 ON なので、Spawn 直後に原点で発生してしまう。
        // 溜め中だけ Update で SetAuto(true) にするため、まずは発生を止めておく。
        chargeEmitter_->SetAuto(false);
    }
    bulletEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chargeBullet");
}

void ChargeShot::Update()
{
    Input *input = Input::GetInstance();

    if (isAlive_)
    {
        if (pBulletCollider_)
        {
            // 当たり判定は発射済みの弾のみ有効化する。
            // 溜め中(未発射)に大きく育った弾が敵へ触れて誤爆・フラグ不整合を起こすのを防ぐ
            pBulletCollider_->SetEnabled(isFired_);
            pBulletCollider_->SetRadius(scale_);
        }

        // 弾エミッターの更新と位置・スケール設定
        bulletEmitter_->Update();
        bulletEmitter_->SetPosition(transform_->translation_);
        bulletEmitter_->SetStartScale("chargeBullet", transform_->scale_ * kBulletParticleScaleMultiplier);
        if (!isMaxScale_)
        {
            bulletEmitter_->SetStartScale("chargeAround", {(kParticleScaleBase + scale_) * kParticleScaleMultiplier,
                                                          (kParticleScaleBase + scale_) * kParticleScaleMultiplier,
                                                          (kParticleScaleBase + scale_) * kParticleScaleMultiplier});
            bulletEmitter_->SetEndScale("chargeAround", {(kParticleScaleBase + kParticleEndScaleOffset + scale_) * kParticleScaleMultiplier,
                                                        (kParticleScaleBase + kParticleEndScaleOffset + scale_) * kParticleScaleMultiplier,
                                                        (kParticleScaleBase + kParticleEndScaleOffset + scale_) * kParticleScaleMultiplier});
        }
    }

    bool chargeHold = false;
    bool chargeRelease = false;

    if (!pPlayer_->GetGamePad()->IsConnected())
    {
        // キーボード入力
        chargeHold = input->PushKey(DIK_K);
        chargeRelease = input->ReleaseMomentKey(DIK_K);
    }
    else
    {
        // ゲームパッド入力 - Yボタン
        chargeHold = pPlayer_->GetGamePad()->IsPress(XINPUT_GAMEPAD_Y);
        chargeRelease = pPlayer_->GetGamePad()->IsRelease(XINPUT_GAMEPAD_Y);
    }

    // ガード中はチャージショットを溜め・発射できない（発射済みの弾はそのまま飛ばす）
    if (pPlayer_ && pPlayer_->IsGuarding())
    {
        chargeHold = false;
        chargeRelease = false;
    }

    // 必殺技のカメラ演出中など、溜め操作がロックされている間は入力を無効化する。
    // 溜め中だった場合はその場で凍結し（成長・発射しない）、演出の新規発生も止まる
    if (isActionLocked_)
    {
        chargeHold = false;
        chargeRelease = false;
    }

    if (!isAlive_)
    {
        // まだ溜めていない状態。
        // スキルメニュー中(LT長押しで必殺技を照準している間)は溜めを開始しない。
        // ここを分けないと、必殺技発動のYボタン押下で溜めロジックへ入り、
        // チャージ演出(chargeEmitter_)が誤って発生してしまう
        if (!isSkillMenu_ && chargeHold)
        {
            // チャージ開始判定：ボタンが押され続けている時間を計測
            chargeStartTimer_ += Frame::DeltaTime();

            // 閾値以上押され続けた場合のみチャージ開始
            if (chargeStartTimer_ >= pPlayer_->GetChargeThreshold())
            {
                // エネルギーチェック
                if (pPlayer_ && pPlayer_->GetEnergy() < kMinEnergyToStart)
                {
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
        }
        else
        {
            // ボタンを離した / スキルメニュー中 はタイマーをリセット
            chargeStartTimer_ = 0.0f;
        }
    }
    else
    {
        // 溜め中(isAlive_)のみスケール成長・発射を扱う
        if (chargeHold && !isFired_)
        {
            scale_ += scaleSpeed_ * (Frame::DeltaTime());
            if (scale_ >= maxScale_)
            {
                scale_ = maxScale_;
                isMaxScale_ = true;
            }
            // 溜め演出の Update() はフレーム末尾で毎フレーム呼ぶ（emitフラグ残留防止）。
            // ここでは溜め中フラグのみ立てる
            isCharge_ = true;
        }
        if (chargeRelease && !isFired_)
        {
            // エネルギー消費量を計算(チャージ率に応じて5~20)
            float scaleRatio = (scale_ - kInitialScale) / (maxScale_ - kInitialScale);
            float energyCost = kMinEnergyCost + ((kMaxEnergyCost - kMinEnergyCost) * scaleRatio);

            // エネルギーチェック
            if (!pPlayer_->ConsumeEnergy(energyCost))
            {
                // エネルギー不足ならリセット
                Reset();
                isCharge_ = false;
                chargeStartTimer_ = 0.0f;
                return;
            }

            Vector3 dir = {0, 0, 1};
            Vector3 pos = transform_->translation_;

            if (pPlayer_)
            {
                if (pPlayer_->GetIsLockOn() && pPlayer_->GetEnemy())
                {
                    // ロックオン時は敵方向に向けて発射
                    dir = pPlayer_->GetEnemy()->GetLocalPosition() - pos;
                    float len = dir.Length();
                    if (len > 0.0001f)
                    {
                        dir = dir / len;
                    }
                }
                else
                {
                    // プレイヤーの回転をかけて発射方向を計算
                    dir = (pPlayer_->GetLocalRotation() * Vector3(0.0f, 0.0f, 1.0f)).Normalize();
                    dir.x = -dir.x;
                }
            }

            Fire(pos, dir);
            isFired_ = true;
            isCharge_ = false;
            firedThisFrame_ = true; // 入力表示UI用：発射した瞬間を通知
            chargeStartTimer_ = 0.0f;
        }
    }

    // 発射後の移動処理
    if (isFired_ && isAlive_)
    {
        transform_->translation_.x += velocity_.x * (1.0f / 60.0f);
        transform_->translation_.y += velocity_.y * (1.0f / 60.0f);
        transform_->translation_.z += velocity_.z * (1.0f / 60.0f);

        // プレイヤーから一定距離離れたらリセット
        if ((transform_->translation_ - pPlayer_->GetLocalPosition()).Length() > kMaxDistance)
        {
            Reset();
        }
    }

    if (isAlive_ && !isFired_)
    {
        if (pPlayer_)
        {
            Vector3 playerPos = pPlayer_->GetLocalPosition();
            Quaternion rot = pPlayer_->GetLocalRotation();
            Vector3 baseForward = Vector3(0.0f, 0.0f, 1.0f);
            Vector3 forwardDir = rot * baseForward;
            forwardDir.x = -forwardDir.x;
            Vector3 normForward = forwardDir.Normalize();

            // チャージ弾のオフセット距離
            float chargeRadius = scale_;

            // 両手（ジョイント）の中点を基準に、弾の半径ぶん前方へ押し出して保持する。
            // ジョイントが取得できない場合は従来どおり本体位置＋オフセットで代用する
            auto rightHand = pPlayer_->GetJointWorldPosition(kRightHandJointName);
            auto leftHand = pPlayer_->GetJointWorldPosition(kLeftHandJointName);
            if (rightHand && leftHand)
            {
                Vector3 handMid = (*rightHand + *leftHand) * 0.5f;
                transform_->translation_ = handMid + normForward * (chargeRadius + offsetMargin_);
            }
            else
            {
                float offsetDistance = playerRadius_ + chargeRadius + offsetMargin_;

                // オフセット計算
                Vector3 offset = normForward * offsetDistance;

                // 高さ(Y軸)オフセット
                offset.y = verticalOffset_;

                // チャージ弾の位置更新
                transform_->translation_ = playerPos + offset;
            }

            // エミッター位置も更新
            chargeEmitter_->SetTranslate(transform_->translation_);

            // 溜め中(未発射)は当たり判定を持たせない
            if (pBulletCollider_)
            {
                pBulletCollider_->SetEnabled(false);
            }
        }
    }

    // チャージ溜め演出(GPUパーティクル)の発生制御。
    // 実際に溜め上げ中(未発射・最大到達前)で、かつロックされていないときだけ発生させる。
    // 発生フラグはエンジンが毎フレーム消費するので、ここでは値を与えるだけでよい。
    chargeEmitter_->SetAuto(isCharge_ && !isFired_ && !isMaxScale_ && !isActionLocked_);

    // 階層的ワールド変換更新
    BaseObject::UpdateWorldTransformHierarchy();
}

void ChargeShot::Fire(const Vector3 &pos, const Vector3 &dir)
{
    transform_->translation_ = pos;
    velocity_ = dir * speed_;

    // 発射した瞬間から当たり判定を有効化する（溜め中は無効だったものをここで有効化）
    if (pBulletCollider_)
    {
        pBulletCollider_->SetEnabled(true);
        pBulletCollider_->SetRadius(scale_);
    }
}

void ChargeShot::Reset()
{

    if (pBulletCollider_)
    {
        pBulletCollider_->SetEnabled(false);
    }

    isAlive_ = false;
    isFired_ = false;
    isCharge_ = false; // 溜め中フラグも必ずクリアする（オーラ演出等の残留防止）
    scale_ = kInitialScale;
    isMaxScale_ = false;
    velocity_ = {0, 0, 0};
    transform_->translation_ = {0, 0, 0};

    // 溜め演出(GPUパーティクル)の新規発生を止める（既存分は自然消滅させる）。
    if (chargeEmitter_)
    {
        chargeEmitter_->SetAuto(false);
    }
}

float ChargeShot::GetDamage() const
{
    // スケールの割合を計算(1.0f~maxScale_の範囲を0.0f~1.0fに正規化)
    float scaleRatio = (scale_ - kInitialScale) / (maxScale_ - kInitialScale);

    // 割合に応じてダメージを計算(最小1ダメージは保証)
    float damage = kMaxDamage * scaleRatio;
    return std::max(kMinDamage, damage);
}

void ChargeShot::Draw(const ViewProjection &viewProjection)
{
    if (!isAlive_)
        return;
    // スケールを反映
    transform_->scale_ = {scale_, scale_, scale_};
}
void ChargeShot::DrawParticle(const ViewProjection &viewProjection)
{
    // chargeEmitter_（GPUパーティクル）はエンジンが自動で描画する。
    // ここでは別システムの CPU パーティクル（弾本体）だけ描画する。
    if (!isAlive_)
        return;
    bulletEmitter_->Draw(viewProjection); // CPU emitter
}

void ChargeShot::imgui()
{
}

void ChargeShot::OnCollisionEnterCallback(ColliderBase *other)
{
    // 発射済みの弾のみ命中扱いにする。
    // 溜め中(未発射)の弾は当たり判定を無効化しているが、念のためここでも弾く
    if (!isFired_)
    {
        return;
    }

    // 地形メッシュに当たったら消滅させる
    if (other->GetTag() == "Ground")
    {
        Reset();
        return;
    }

    // Enemyタグを持つコライダーと衝突した場合
    if (other->GetTag() == "Enemy")
    {
        // プレイヤーの敵が存在し、生きている場合
        if (pPlayer_ && pPlayer_->GetEnemy() && pPlayer_->GetEnemy()->GetAlive())
        {
            isAlive_ = false;

            // チャージ度合いに応じたダメージを計算して適用（射撃扱い＝ひるみは近接より短い）
            float damage = GetDamage();
            pPlayer_->GetEnemy()->SetDamage(damage, true);

            Reset();
        }
    }
}