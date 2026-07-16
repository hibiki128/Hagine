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

    // 地面との衝突はメッシュコライダーで判定するため、
    // これは地形の外へ抜け落ちた場合の保険（地形の最低高さより下）
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

void EnemyBullet::InitTransform(Enemy *enemy)
{
    // 手（右手ジョイント）を発射起点にする。
    // ジョイントが取得できない場合は従来どおり本体位置＋オフセットで代用する
    std::optional<Vector3> handPos = enemy->GetJointWorldPosition(kHandJointName);
    const bool fromHand = handPos.has_value();
    this->transform_->translation_ = fromHand ? *handPos : enemy->GetLocalPosition();

    // コライダーの設定
    pCollider_ = AddSphereCollider("enemy_bullet");
    pCollider_->SetTag("EnemyBullet");
    pCollider_->AddCollisionMask("Player");
    pCollider_->AddCollisionMask("Ground");

    pCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    pTarget_ = enemy->GetTarget();

    // ロックオン状態ならターゲットへ向ける
    if (enemy->GetIsLockOn() && enemy->GetTarget())
    {
        isLockOnBullet_ = true;

        // 発射起点（手）からターゲットへ向けて狙う
        Vector3 spawnPos = this->transform_->translation_;
        Vector3 enemyPos = {enemy->GetTarget()->GetLocalPosition().x, enemy->GetTarget()->GetLocalPosition().y + kHomingHeightOffset, enemy->GetTarget()->GetLocalPosition().z};

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

        Quaternion rot = enemy->GetLocalRotation();
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

void EnemyBullet::OnCollisionEnter(ColliderBase *other)
{
    // プレイヤーに当たった時の処理
    if (other->GetTag() == "Player" && isAlive_ && pTarget_ && pTarget_->GetAlive())
    {
        isHit_ = true;
        pTarget_->SetDamage(damage_);
    }

    // 地形メッシュに当たったら消滅させる（パーティクル終了後に isAlive_ が折れる）
    if (other->GetTag() == "Ground" && isAlive_)
    {
        isHit_ = true;
    }
}