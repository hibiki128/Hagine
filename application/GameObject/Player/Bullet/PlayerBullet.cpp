#include "PlayerBullet.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "application/GameObject/Player/Player.h"
#include <cmath>

void PlayerBullet::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    this->CreatePrimitiveModel(PrimitiveType::Sphere);
    this->AddCollider();
    this->SetCollisionType(CollisionType::Sphere);
    this->SetTexture("debug/white1x1.png");
    BaseObject::SetColor({0.0f, 0.0f, 1.0f, 1.0f});

    // 弾の生存時間を設定（5秒後に消える）
    lifeTime_ = 5.0f;
    currentLifeTime_ = 0.0f;
}

void PlayerBullet::Update() {
    // 生存時間の更新
    currentLifeTime_ += ImGui::GetIO().DeltaTime;

    // 生存時間が過ぎたら弾を無効化
    if (currentLifeTime_ >= lifeTime_) {
        isAlive_ = false;
        return;
    }

    // ロックオンモードの場合、敵への追尾処理
    if (isLockOnBullet_ && targetEnemy_) {
        Vector3 bulletPos = GetWorldPosition();
        Vector3 enemyPos = targetEnemy_->GetWorldPosition();

        // 敵への方向ベクトルを計算
        Vector3 toEnemy = enemyPos - bulletPos;
        float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

        // 敵が存在し、距離がある場合
        if (distance > 0.1f) {
            // 方向ベクトルを正規化
            toEnemy.x /= distance;
            toEnemy.y /= distance;
            toEnemy.z /= distance;

            // 現在の速度方向と敵への方向を補間（追尾の強さを調整）
            float homingStrength = 2.0f; // 追尾の強さ（大きいほど強く追尾）
            Vector3 currentDir = velocity_;
            float currentSpeed = std::sqrt(currentDir.x * currentDir.x + currentDir.y * currentDir.y + currentDir.z * currentDir.z);

            if (currentSpeed > 0.1f) {
                // 現在の方向を正規化
                currentDir.x /= currentSpeed;
                currentDir.y /= currentSpeed;
                currentDir.z /= currentSpeed;

                // 追尾方向へ徐々に向きを変える
                Vector3 newDir;
                newDir.x = currentDir.x + (toEnemy.x - currentDir.x) * homingStrength * ImGui::GetIO().DeltaTime;
                newDir.y = currentDir.y + (toEnemy.y - currentDir.y) * homingStrength * ImGui::GetIO().DeltaTime;
                newDir.z = currentDir.z + (toEnemy.z - currentDir.z) * homingStrength * ImGui::GetIO().DeltaTime;

                // 新しい方向を正規化
                float newDirLength = std::sqrt(newDir.x * newDir.x + newDir.y * newDir.y + newDir.z * newDir.z);
                if (newDirLength > 0.1f) {
                    newDir.x /= newDirLength;
                    newDir.y /= newDirLength;
                    newDir.z /= newDirLength;

                    // 速度を更新
                    velocity_ = newDir * speed_;
                }
            }
        }
    }

    // 位置を更新
    transform_.translation_ += velocity_ * ImGui::GetIO().DeltaTime;
}

void PlayerBullet::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    // 生きている場合のみ描画
    if (isAlive_) {
        BaseObject::Draw(viewProjection, offSet);
    }
}

void PlayerBullet::InitTransform(Player *player) {
    // プレイヤーの位置を弾の初期位置に設定
    this->transform_.translation_ = player->GetWorldPosition();

    // プレイヤーがロックオン中かチェック
    if (player->GetIsLockOn() && player->GetEnemy()) {
        // ロックオン時：敵に向かって発射
        isLockOnBullet_ = true;
        targetEnemy_ = player->GetEnemy();

        Vector3 playerPos = player->GetWorldPosition();
        Vector3 enemyPos = player->GetEnemy()->GetWorldPosition();

        // 敵への方向ベクトルを計算
        Vector3 direction = enemyPos - playerPos;

        // 方向ベクトルを正規化
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (length > 0.1f) {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
        } else {
            // 敵が非常に近い場合は前方に発射
            direction = {0.0f, 0.0f, 1.0f};
        }

        // 弾の速度を設定
        velocity_ = direction * speed_;
    } else {
        // 通常時：プレイヤーの向いている方向に発射
        isLockOnBullet_ = false;
        targetEnemy_ = nullptr;

        // プレイヤーの回転から方向ベクトルを計算
        float rotationY = player->GetWorldRotation().y;

        Vector3 direction;
        direction.x = std::sin(rotationY);
        direction.y = 0.0f; // 水平方向に発射
        direction.z = std::cos(rotationY);

        // 弾の速度を設定
        velocity_ = direction * speed_;
    }

    // 弾を少し前方に配置（プレイヤーの中から出ないように）
    Vector3 forwardOffset;
    forwardOffset.x = std::sin(player->GetWorldRotation().y) * 2.0f;
    forwardOffset.y = 1.0f; // 少し上に
    forwardOffset.z = std::cos(player->GetWorldRotation().y) * 2.0f;

    this->transform_.translation_ += forwardOffset;
}

void PlayerBullet::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<Enemy *>(other) && isAlive_) {
        isAlive_ = false;
        SetCollisionEnabled(false);
    }
}
