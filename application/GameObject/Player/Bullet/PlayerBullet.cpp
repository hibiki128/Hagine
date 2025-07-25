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

    // 加速度の初期設定
    acce_ = 10.0f; // デフォルトの加速度

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
    // 生きている場合のみ描画
    if (isAlive_) {
        //BaseObject::SetBlendMode(BlendMode::kAdd);
        //BaseObject::Draw(viewProjection, offSet);
    }
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
        // 通常時：プレイヤーの回転から方向ベクトルを計算
        isLockOnBullet_ = false;
        targetEnemy_ = nullptr;

        // プレイヤーの回転（クォータニオン）を使って前方ベクトルを計算
        Quaternion rot = player->GetLocalRotation();
        Vector3 baseForward = Vector3(0.0f, 0.0f, 1.0f);
        Vector3 direction = rot * baseForward;

        // 左右反転補正（必要に応じて有効化）
        direction.x = -direction.x;

        // 前後反転補正（基本は不要）
        // direction.z = -direction.z;

        // 速度ベクトル設定
        velocity_ = direction.Normalize() * speed_;
    }

    // 弾を少し前方に配置（プレイヤーの中から出ないように）
    Vector3 forwardOffset = velocity_.Normalize() * 2.0f;
    forwardOffset.y += 1.0f; // 少し上に

    this->transform_->translation_ += forwardOffset;
}

void PlayerBullet::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<Enemy *>(other) && isAlive_) {
        SetCollisionEnabled(false);
        isHit_ = true;
    }
}