#include "EnemyBullet.h"
#include "Collider/CollisionManager.h"
#include "Debug/Log/Logger.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <Engine/Frame/Frame.h>
#include <cmath>

void EnemyBullet::Init(const std::string objectName) {
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

void EnemyBullet::Update() {
    // 衝突済みかつパーティクルが全て終了していたら生存フラグを折る
    if (isHit_ && emitter_->IsAllParticlesComplete()) {
        isAlive_ = false;
    }

    // 生存中かつ未衝突ならパーティクルを更新
    if (isAlive_ && !isHit_) {
        emitter_->SetPosition(GetLocalPosition());
        emitter_->Update();
    }

    float deltaTime = Frame::DeltaTime();

    // 生存タイマーの更新
    currentLifeTime_ += deltaTime;

    // 生存時間を超えたら消滅
    if (currentLifeTime_ >= lifeTime_) {
        isAlive_ = false;
        return;
    }

    // 地面に埋まったら消滅
    if (transform_->translation_.y <= -0.5f) {
        isAlive_ = false;
        return;
    }

    // ホーミング処理
    if (isLockOnBullet_ && target_) {
        Vector3 bulletPos = GetLocalPosition();
        Vector3 enemyPos = target_->GetLocalPosition();

        Vector3 toEnemy = enemyPos - bulletPos;
        float distance = toEnemy.Length();

        if (distance > kMinDistanceThreshold) {
            toEnemy = toEnemy / distance;

            Vector3 currentDir = velocity_;
            float currentSpeed = currentDir.Length();

            if (currentSpeed > kMinSpeedThreshold) {
                currentDir = currentDir / currentSpeed;

                // 現在の方向からターゲットの方向へ徐々に補間
                Vector3 newDir = currentDir + (toEnemy - currentDir) * kHomingStrength * deltaTime;
                float newDirLength = newDir.Length();

                if (newDirLength > kMinSpeedThreshold) {
                    newDir = newDir / newDirLength;
                    velocity_ = newDir * currentSpeed;
                }
            }
        }
    }

    // 加速度による速度更新
    Vector3 currentDir = velocity_;
    float currentSpeed = currentDir.Length();

    if (currentSpeed > kMinSpeedThreshold) {
        currentDir = currentDir / currentSpeed;

        float newSpeed = currentSpeed + acce_ * deltaTime;
        if (newSpeed > kMaxSpeed)
            newSpeed = kMaxSpeed;

        velocity_ = currentDir * newSpeed;
    }

    // 最終的な座標更新
    transform_->translation_ += velocity_ * deltaTime;
}
void EnemyBullet::Draw(const ViewProjection &viewProjection) {
    // モデルの直接描画は行わない（必要に応じて実装）
}

void EnemyBullet::DrawParticle(const ViewProjection &viewProjection) {
    // 生きている場合のみパーティクルを描画
    if (isAlive_) {
        emitter_->Draw(viewProjection);
    }
}

void EnemyBullet::InitTransform(Enemy *enemy) {
    // 発射元の敵の位置を初期座標に設定
    this->transform_->translation_ = enemy->GetLocalPosition();
    
    // コライダーの設定
    collider_ = AddSphereCollider("enemy_bullet");
    collider_->SetTag("EnemyBullet");
    collider_->AddCollisionMask("Player");

    collider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    target_ = enemy->GetTarget();

    // ロックオン状態ならターゲットへ向ける
    if (enemy->GetIsLockOn() && enemy->GetTarget()) {
        isLockOnBullet_ = true;

        Vector3 playerPos = enemy->GetLocalPosition();
        Vector3 enemyPos = enemy->GetTarget()->GetLocalPosition();

        Vector3 direction = enemyPos - playerPos;

        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (length > kMinSpeedThreshold) {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
        } else {
            direction = {0.0f, 0.0f, 1.0f};
        }

        velocity_ = direction * speed_;
    } else {
        // 非ロックオン時は敵の正面方向へ
        isLockOnBullet_ = false;

        Quaternion rot = enemy->GetLocalRotation();
        Vector3 baseForward = Vector3(0.0f, 0.0f, 1.0f);
        Vector3 direction = rot * baseForward;

        // X軸の反転（エンジンの仕様に合わせる）
        direction.x = -direction.x;

        velocity_ = direction.Normalize() * speed_;
    }

    // 発射位置を少し前方にずらす
    Vector3 forwardOffset = velocity_.Normalize() * kForwardOffsetDistance;
    forwardOffset.y += kVerticalOffset; // 少し上方に

    this->transform_->translation_ += forwardOffset;
}

void EnemyBullet::OnCollisionEnter(ColliderBase *other) {
    // プレイヤーに当たった時の処理
    if (other->GetTag() == "Player" && isAlive_ && target_ && target_->GetAlive()) {
        isHit_ = true;
        target_->SetDamage(damage_);
    }
}