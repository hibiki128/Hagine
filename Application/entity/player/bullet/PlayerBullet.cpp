#include "PlayerBullet.h"
#include "collider/CollisionManager.h"
#include "debug/log/Logger.h"
#include "particle/ParticleEditor.h"
#include "application/entity/enemy/Enemy.h"
#include "application/entity/player/Player.h"
#include <frame/Frame.h>
#include <cmath>

using namespace Hagine;
void PlayerBullet::Init(const std::string objectName)
{
    BaseObject::Init(objectName);
    this->CreatePrimitiveModel(PrimitiveType::Sphere);
    this->SetTexture("debug/white1x1.png");
    BaseObject::SetColor({0.0f, 0.0f, 1.0f, 1.0f});

    // 弾の生存時間を設定(5秒後に消える)
    lifeTime_ = kDefaultLifeTime;
    currentLifeTime_ = 0.0f;
    acce_ = kDefaultAcceleration;

    emitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("bulletEmitter");
}

void PlayerBullet::Update()
{
    if (isHit_ && emitter_->IsAllParticlesComplete())
    {
        isAlive_ = false;
    }

    if (isAlive_ && !isHit_)
    {
        emitter_->SetPosition(GetLocalPosition());
        emitter_->Update();
    }

    float deltaTime = Frame::DeltaTime();

    currentLifeTime_ += deltaTime;

    if (currentLifeTime_ >= lifeTime_)
    {
        isAlive_ = false;
        return;
    }

    // 地面に埋まったら消える
    if (transform_->translation_.y <= -0.5f)
    {
        isAlive_ = false;
        return;
    }

    if (isLockOnBullet_ && pTargetEnemy_)
    {
        Vector3 bulletPos = GetLocalPosition();
        Vector3 enemyPos = pTargetEnemy_->GetLocalPosition();

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

    // 加速度処理
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

    // 位置更新
    transform_->translation_ += velocity_ * deltaTime;
}
void PlayerBullet::Draw(const ViewProjection &viewProjection)
{
}

void PlayerBullet::DrawParticle(const ViewProjection &viewProjection)
{
    // 生きている場合のみ描画
    if (isAlive_)
    {
        emitter_->Draw(viewProjection);
    }
}

void PlayerBullet::InitTransform(Player *player)
{
    // プレイヤーの位置を弾の初期位置に設定
    this->transform_->translation_ = player->GetLocalPosition();
    pCollider_ = AddSphereCollider("player_bullet");
    pCollider_->SetTag("PlayerBullet");
    pCollider_->AddCollisionMask("Enemy");

    pCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    pTargetEnemy_ = player->GetEnemy();
    if (player->GetIsLockOn() && player->GetEnemy())
    {
        isLockOnBullet_ = true;

        Vector3 playerPos = player->GetLocalPosition();
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();

        Vector3 direction = enemyPos - playerPos;

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
        isLockOnBullet_ = false;

        Quaternion rot = player->GetLocalRotation();
        Vector3 baseForward = Vector3(0.0f, 0.0f, 1.0f);
        Vector3 direction = rot * baseForward;

        direction.x = -direction.x;

        velocity_ = direction.Normalize() * speed_;
    }

    Vector3 forwardOffset = velocity_.Normalize() * kForwardOffsetDistance;
    forwardOffset.y += kVerticalOffset; // 少し上に

    this->transform_->translation_ += forwardOffset;
}

void PlayerBullet::OnCollisionEnter(ColliderBase *other)
{
    if (other->GetTag() == "Enemy" && isAlive_ && pTargetEnemy_->GetAlive())
    {
        isHit_ = true;
        pTargetEnemy_->SetDamage(kBulletDamage);
    }
}