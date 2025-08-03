#include "PlayerBullet.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <Engine/Frame/Frame.h>
#include <cmath>
#include"Debug/Log/Logger.h"

void PlayerBullet::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    this->CreatePrimitiveModel(PrimitiveType::Sphere);
    this->SetTexture("debug/white1x1.png");
    BaseObject::SetColor({0.0f, 0.0f, 1.0f, 1.0f});

    // 弾の生存時間を設定（5秒後に消える）
    lifeTime_ = 5.0f;
    currentLifeTime_ = 0.0f;
    acce_ = 10.0f;

    emitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("bulletEmitter");
}

void PlayerBullet::Update() {
    if (isHit_ && emitter_->IsAllParticlesComplete()) {
        isAlive_ = false;
    }

    if (isAlive_ && !isHit_) {
        emitter_->SetPosition(GetLocalPosition());
        emitter_->Update();
    }

    float deltaTime = Frame::DeltaTime();

    currentLifeTime_ += deltaTime;

    if (currentLifeTime_ >= lifeTime_) {
        isAlive_ = false;
        return;
    }

    if (isLockOnBullet_ && targetEnemy_) {
        Vector3 bulletPos = GetLocalPosition();
        Vector3 enemyPos = targetEnemy_->GetLocalPosition();

        Vector3 toEnemy = enemyPos - bulletPos;
        float distance = toEnemy.Length();

        if (distance > 0.1f) {
            toEnemy = toEnemy / distance;

            float homingStrength = 2.0f;
            Vector3 currentDir = velocity_;
            float currentSpeed = currentDir.Length();

            if (currentSpeed > 0.1f) {
                currentDir = currentDir / currentSpeed;

                Vector3 newDir = currentDir + (toEnemy - currentDir) * homingStrength * deltaTime;
                float newDirLength = newDir.Length();

                if (newDirLength > 0.1f) {
                    newDir = newDir / newDirLength;
                    velocity_ = newDir * currentSpeed;
                }
            }
        }
    }

    // 加速度処理
    Vector3 currentDir = velocity_;
    float currentSpeed = currentDir.Length();

    if (currentSpeed > 0.1f) {
        currentDir = currentDir / currentSpeed;

        float newSpeed = currentSpeed + acce_ * deltaTime;
        float maxSpeed = 200.0f;
        if (newSpeed > maxSpeed)
            newSpeed = maxSpeed;

        velocity_ = currentDir * newSpeed;
    }

    // 位置更新
    transform_->translation_ += velocity_ * deltaTime;
}
void PlayerBullet::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
}
void PlayerBullet::DrawParticle(const ViewProjection &viewProjection) {
    // 生きている場合のみ描画
    if (isAlive_) {
        emitter_->Draw(viewProjection);
    }
}
void PlayerBullet::InitTransform(Player *player) {
    // プレイヤーの位置を弾の初期位置に設定
    this->transform_->translation_ = player->GetLocalPosition();

    this->AddCollider();
    this->SetCollisionType(CollisionType::Sphere);

    if (player->GetIsLockOn() && player->GetEnemy()) {
        // ロックオン時：敵に向かって発射
        isLockOnBullet_ = true;
        targetEnemy_ = player->GetEnemy();

        Vector3 playerPos = player->GetLocalPosition();
        Vector3 enemyPos = player->GetEnemy()->GetLocalPosition();

        Vector3 direction = enemyPos - playerPos;

        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (length > 0.1f) {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
        } else {
            direction = {0.0f, 0.0f, 1.0f};
        }

        velocity_ = direction * speed_;
    } else {
        isLockOnBullet_ = false;
        targetEnemy_ = nullptr;

        Quaternion rot = player->GetLocalRotation();
        Vector3 baseForward = Vector3(0.0f, 0.0f, 1.0f);
        Vector3 direction = rot * baseForward;

        direction.x = -direction.x;

        velocity_ = direction.Normalize() * speed_;
    }

    Vector3 forwardOffset = velocity_.Normalize() * 2.0f;
    forwardOffset.y += 1.0f; // 少し上に

    this->transform_->translation_ += forwardOffset;
}

void PlayerBullet::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<Enemy *>(other) && isAlive_ && targetEnemy_->GetAlive()) {
        SetCollisionEnabled(false);
        isHit_ = true;
        targetEnemy_->SetDamage(2);
    }
}