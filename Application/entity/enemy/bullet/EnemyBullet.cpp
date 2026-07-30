#include "EnemyBullet.h"
#include "collider/CollisionManager.h"
#include "debug/log/Logger.h"
#include "particle/ParticleEditor.h"
#include "application/entity/enemy/Enemy.h"
#include "application/entity/player/Player.h"
#include <frame/Frame.h>
#include <cmath>

using namespace Hagine;
void EnemyBullet::Init(const std::string objectName)
{
    BaseObject::Init(objectName);
    this->CreatePrimitiveModel(PrimitiveType::Sphere);
    this->SetTexture("debug/white1x1.png");
    BaseObject::SetColor({0.0f, 0.0f, 1.0f, 1.0f});

    // 生存時間と加速度の初期設定
    lifeTime_ = kDefaultLifeTime;
    currentLifeTime_ = 0.0f;
    acce_ = kDefaultAcceleration;

    // パーティクルエミッターの生成
    emitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("enemyBulletEmitter");
}

void EnemyBullet::Update()
{
    // 衝突済みかつパーティクルが全て終了していたら生存フラグを折る
    if (isHit_ && emitter_->IsAllParticlesComplete())
    {
        isAlive_ = false;
    }

    // 生存中かつ未衝突ならパーティクルを更新
    if (isAlive_ && !isHit_)
    {
        emitter_->SetPosition(GetLocalPosition());
        emitter_->Update();
    }

    float deltaTime = Frame::DeltaTime();

    // 生存タイマーの更新
    currentLifeTime_ += deltaTime;

    // 生存時間を超えたら消滅
    if (currentLifeTime_ >= lifeTime_)
    {
        isAlive_ = false;
        return;
    }

    // 地面との衝突はメッシュコライダーで判定するので、これは地形外へ抜け落ちた場合の保険
    if (transform_->translation_.y <= kFallbackKillY)
    {
        isAlive_ = false;
        return;
    }

    // ホーミング処理
    if (isLockOnBullet_ && pTarget_)
    {
        Vector3 bulletPos = GetLocalPosition();
        Vector3 enemyPos = {pTarget_->GetLocalPosition().x, pTarget_->GetLocalPosition().y + kHomingHeightOffset, pTarget_->GetLocalPosition().z};

        Vector3 toEnemy = enemyPos - bulletPos;
        float distance = toEnemy.Length();

        if (distance > kMinDistanceThreshold)
        {
            toEnemy = toEnemy / distance;

            Vector3 currentDir = velocity_;
            float currentSpeed = currentDir.Length();

            if (currentSpeed > kMinSpeedThreshold)
            {
                currentDir = currentDir / currentSpeed;

                // 現在の方向からターゲットの方向へ徐々に補間
                Vector3 newDir = currentDir + (toEnemy - currentDir) * kHomingStrength * deltaTime;
                float newDirLength = newDir.Length();

                if (newDirLength > kMinSpeedThreshold)
                {
                    newDir = newDir / newDirLength;
                    velocity_ = newDir * currentSpeed;
                }
            }
        }
    }

    // 加速度による速度更新
    Vector3 currentDir = velocity_;
    float currentSpeed = currentDir.Length();

    if (currentSpeed > kMinSpeedThreshold)
    {
        currentDir = currentDir / currentSpeed;

        float newSpeed = currentSpeed + acce_ * deltaTime;
        if (newSpeed > kMaxSpeed)
            newSpeed = kMaxSpeed;

        velocity_ = currentDir * newSpeed;
    }

    // 最終的な座標更新
    transform_->translation_ += velocity_ * deltaTime;
}
void EnemyBullet::Draw(const ViewProjection &viewProjection)
{
    // モデルの直接描画は行わない（必要に応じて実装）
}

void EnemyBullet::DrawParticle(const ViewProjection &viewProjection)
{
    // 生きている場合のみパーティクルを描画
    if (isAlive_)
    {
        emitter_->Draw(viewProjection);
    }
}

void EnemyBullet::InitTransform(Enemy *pEnemy)
{
    // 手（右手ジョイント）を発射起点にする（取得できなければ本体位置＋オフセットで代用）
    std::optional<Vector3> handPos = pEnemy->GetJointWorldPosition(kHandJointName);
    const bool fromHand = handPos.has_value();
    this->transform_->translation_ = fromHand ? *handPos : pEnemy->GetLocalPosition();

    // コライダーの設定
    pCollider_ = AddSphereCollider("enemy_bullet");
    pCollider_->SetTag("EnemyBullet");
    pCollider_->AddCollisionMask("Player");
    pCollider_->AddCollisionMask("Ground");

    pCollider_->SetOnCollisionEnter([this](ColliderBase *pOther) {
        this->OnCollisionEnter(pOther);
    });

    pTarget_ = pEnemy->GetTarget();

    // ロックオン状態ならターゲットへ向ける
    if (pEnemy->GetIsLockOn() && pEnemy->GetTarget())
    {
        isLockOnBullet_ = true;

        // 発射起点（手）からターゲットへ向けて狙う
        Vector3 spawnPos = this->transform_->translation_;
        Vector3 enemyPos = {pEnemy->GetTarget()->GetLocalPosition().x, pEnemy->GetTarget()->GetLocalPosition().y + kHomingHeightOffset, pEnemy->GetTarget()->GetLocalPosition().z};

        Vector3 direction = enemyPos - spawnPos;

        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (length > kMinSpeedThreshold)
        {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
        }
        else
        {
            direction = {0.0f, 0.0f, 1.0f};
        }

        velocity_ = direction * speed_;
    }
    else
    {
        // 非ロックオン時は敵の正面方向へ
        isLockOnBullet_ = false;

        Quaternion rot = pEnemy->GetLocalRotation();
        Vector3 baseForward = Vector3(0.0f, 0.0f, 1.0f);
        Vector3 direction = rot * baseForward;

        // X軸の反転（エンジンの仕様に合わせる）
        direction.x = -direction.x;

        velocity_ = direction.Normalize() * speed_;
    }

    // 手から撃つ場合は手の少し先から、代用時は従来どおり前方＋少し上から発射する
    if (fromHand)
    {
        this->transform_->translation_ += velocity_.Normalize() * kHandForwardOffset;
    }
    else
    {
        Vector3 forwardOffset = velocity_.Normalize() * kForwardOffsetDistance;
        forwardOffset.y += kVerticalOffset; // 少し上方に
        this->transform_->translation_ += forwardOffset;
    }
}

void EnemyBullet::DeflectFrom(const Vector3 &guardPosition)
{
    isDeflected_ = true;
    isLockOnBullet_ = false; // 弾かれた弾は追尾しない
    acce_ = 0.0f;            // これ以上加速もしない

    // ガードした相手から見た外側（水平）方向。真後ろへ返すと撃った本人へ戻ってしまう
    Vector3 outward = GetWorldPosition() - guardPosition;
    outward.y = 0.0f;
    if (outward.Length() > kMinSpeedThreshold)
    {
        outward = outward.Normalize();
    }
    else
    {
        outward = {-velocity_.x, 0.0f, -velocity_.z};
        outward = (outward.Length() > kMinSpeedThreshold) ? outward.Normalize() : Vector3{0.0f, 0.0f, 1.0f};
    }

    // 外側方向と直交する水平方向。弾の進行方向に近い側へ流して自然に逸れて見せる
    Vector3 lateral = {-outward.z, 0.0f, outward.x};
    if (lateral.x * velocity_.x + lateral.z * velocity_.z < 0.0f)
    {
        lateral = {-lateral.x, 0.0f, -lateral.z};
    }

    Vector3 direction = outward * kDeflectBackRatio + lateral;
    direction = (direction.Length() > kMinSpeedThreshold) ? direction.Normalize() : outward;

    float speed = GetCurrentSpeed() * kDeflectSpeedRatio;
    if (speed < kDeflectMinSpeed)
    {
        speed = kDeflectMinSpeed;
    }
    velocity_ = {direction.x * speed, kDeflectUpSpeed, direction.z * speed};

    // 弾かれたあとは短時間で消滅させる（画面外まで飛び続けさせない）
    const float deflectDeadline = lifeTime_ - kDeflectLifeTime;
    if (currentLifeTime_ < deflectDeadline)
    {
        currentLifeTime_ = deflectDeadline;
    }
}

void EnemyBullet::OnCollisionEnter(ColliderBase *pOther)
{
    // プレイヤーに当たった時の処理
    if (pOther->GetTag() == "Player" && isAlive_ && !isDeflected_ && pTarget_ && pTarget_->GetAlive())
    {
        // ガード中は弾を外側へ弾き返し、ダメージは完全に無効化する
        if (pTarget_->ConsumeGuardDeflect())
        {
            DeflectFrom(pTarget_->GetWorldPosition());
            return;
        }

        isHit_ = true;
        pTarget_->SetDamage(damage_, true);
    }

    // 地形メッシュに当たったら消滅させる（パーティクル終了後に isAlive_ が折れる）
    if (pOther->GetTag() == "Ground" && isAlive_)
    {
        isHit_ = true;
    }
}