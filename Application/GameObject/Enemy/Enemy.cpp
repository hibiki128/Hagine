#define NOMINMAX
#include "Enemy.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChargeShot/ChargeShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include <Debug/Log/Logger.h>
#include <Frame.h>

Enemy::Enemy() {
}

Enemy::~Enemy() {
}

void Enemy::Init(const std::string objectName) {

    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cube);
    enemyCollider_ = AddOBBCollider("enemy_Collider");
    enemyCollider_->SetTag("Enemy");
    enemyCollider_->AddCollisionMask("PlayerBullet");
    enemyCollider_->AddCollisionMask("Player");
    enemyCollider_->AddCollisionMask("PlayerHand");
    enemyCollider_->AddCollisionMask("PlayerChargeBullet");
    enemyCollider_->AddCollisionMask("makan");

    enemyCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    BaseObject::SetTexture("debug/white1x1.png", kTextureIndex);
    BaseObject::SetColor(Vector4(kColorRed, kColorZero, kColorZero, kColorOpaque));
    shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetWorldTransform()->SetRotationEuler(Vector3(degreesToRadians(kShadowRotationDegrees), kRotationZero, kRotationZero));
    shadow_->GetLocalScale() = {kShadowScale, kShadowScale, kShadowScale};
    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("smokeEmitter");
    chargeShake_ = std::make_unique<Shake>();
    isGuarding_ = false;

    // ★追加: 物理パラメータの初期化
    fallSpeed_ = 30.0f; // 重力加速度（推奨: 20.0~50.0）
    moveSpeed_ = 5.0f;  // 移動速度
    jumpSpeed_ = 15.0f; // ジャンプ力
    maxSpeed_ = 10.0f;  // 最大速度
    accelRate_ = 1.0f;  // 加速率

    // 初期状態は地上にいる
    isGrounded_ = true;
    velocity_ = Vector3(0.0f, 0.0f, 0.0f);
    acceleration_ = Vector3(0.0f, 0.0f, 0.0f);
}

void Enemy::Update() {
    shadow_->GetLocalPosition() = {transform_->translation_.x, kShadowYPosition, transform_->translation_.z};
    shadow_->Update();

    if (started_) {
        if (damage_ > kNoDamage) {
            float actualDamage = static_cast<float>(damage_);

            if (isGuarding_) {
                actualDamage *= kGuardDamageMultiplier;
            }

            HP_ -= actualDamage;
            damage_ = kNoDamage;
        }

        if (HP_ <= kMinHP) {
            isAlive_ = false;
            HP_ = kMinHP;
        }

        if (isGuarding_) {
            const float blinkInterval = kBlinkInterval;
            int blinkCount = static_cast<int>(Frame::Time() / blinkInterval);
            if (blinkCount % kBlinkModulo == kEvenBlink) {
                SetColor(Vector4(kColorOpaque, kColorZero, kColorZero, kColorOpaque));
            } else {
                SetColor(Vector4(kColorOpaque, kColorOpaque, kColorOpaque, kColorOpaque));
            }
        } else {
            SetColor(Vector4(kColorOpaque, kColorZero, kColorZero, kColorOpaque));
        }

        // ダメージリアクション中でない時のみ向きを更新
        if (!isDamageReact_) {
            RotateUpdate();
        }

        UpdateShadowScale();
        chargeShake_->Update();

        if (isDamageReact_) {
            damageReactTimer_ += Frame::DeltaTime();

            // イージングされたX回転角
            float angleX = tiltEase_.Update(Frame::DeltaTime());

            // ワールド空間のX軸で回転を作成
            tiltRotation_ = Quaternion::FromAxisAngle(Vector3(kXAxisX, kXAxisY, kXAxisZ), angleX);

            // 重要: tiltRotation_ × baseRotation_ の順序(ワールド空間での回転を先に適用)
            transform_->quateRotation_ = tiltRotation_ * baseRotation_;

            // 高速点滅
            float blinkInterval = kDamageBlinkInterval;
            int blink = static_cast<int>(damageReactTimer_ / blinkInterval);
            SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kAlphaTransparent : kAlphaOpaque);

            // 終了処理
            if (damageReactTimer_ >= damageReactDuration_) {
                isDamageReact_ = false;
                transform_->quateRotation_ = baseRotation_;
                SetAlpha(kAlphaOpaque);
            }
        }

        if (rootNode_) {
            // ターゲット情報などが変わっているかもしれないので更新しても良い
            rootNode_->SetContext(this, target_);

            // ツリーを実行
            rootNode_->Tick();

            // ★修正: 速度のイージングを適用（Y軸は除外）
            if (velocityEase_.isActive) {
                Vector3 easedVelocity = velocityEase_.Update(Frame::DeltaTime());
                velocity_.x = easedVelocity.x;
                velocity_.z = easedVelocity.z;
                // velocity_.y はイージングで上書きしない（重力を優先）
            }

            // ★修正: 重力による速度更新
            if (!isGrounded_) {
                // 空中にいる場合のみ重力を適用
                velocity_.y += acceleration_.y * Frame::DeltaTime();
            } else {
                // 地上にいる場合は垂直加速度をゼロにする
                // （ジャンプ等で再設定されるまで重力の影響を受けない）
                acceleration_.y = 0.0f;
            }
        } else {
            // ★追加: ツリーがない場合は速度をゼロに
            // (エディタで停止した後も動き続けるのを防ぐ)
            velocity_.x = 0.0f;
            velocity_.z = 0.0f;
            // 地上にいる場合は垂直速度と加速度もゼロに
            if (isGrounded_) {
                velocity_.y = 0.0f;
                acceleration_.y = 0.0f;
            }
        }

        CollisionGround();

        BaseObject::Update();
    }
}

void Enemy::MoveToTarget(const Vector3 &targetPos) {
    // ★修正: イージングを使ってスムーズに移動
    if (!target_)
        return;

    Vector3 direction = targetPos - transform_->translation_;
    direction.y = 0; // 高さは合わせない場合
    direction = direction.Normalize();

    // 目標速度を計算
    velocityTarget_ = direction * moveSpeed_;

    // ★イージングで速度を変化させる（短い時間で滑らかに）
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void Enemy::MoveStrafe() {
    if (!target_)
        return;

    Vector3 right = GetRight();

    // 目標速度を計算
    velocityTarget_ = right * (float)strafeDirection_ * moveSpeed_;

    // ★イージングで速度を変化させる（短い時間で滑らかに）
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void Enemy::MoveRetreat() {
    if (!target_)
        return;

    // ターゲットと逆方向へ
    Vector3 direction = transform_->translation_ - target_->GetWorldPosition();
    direction.y = 0;
    direction = direction.Normalize();

    // 目標速度を計算
    velocityTarget_ = direction * moveSpeed_;

    // ★イージングで速度を変化させる（短い時間で滑らかに）
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void Enemy::PerformAttack() {
    // 攻撃ログを出力したり、アニメーションを再生したりする
    Logger::Log("Attack\n");
}

// ★追加: 移動を滑らかに停止
void Enemy::StopMovement() {
    // 現在の速度からゼロへイージング
    Vector3 zeroVel(0.0f, velocity_.y, 0.0f); // Y軸(重力)は維持
    velocityTarget_ = zeroVel;
    velocityEase_.Reset(velocity_, velocityTarget_, kStopEaseTime, EasingType::OutQuad);
}

// ★新規追加: 通常の移動処理（プレイヤーのMove()を参考）
void Enemy::Move() {
    if (!target_)
        return;

    // 横方向の移動は既存のMoveToTarget等で制御されているvelocityを使用
    // ここでは特に処理なし（既にvelocityに値が設定されている前提）

    // 位置を更新（BaseObject::Update()で自動的に行われる）
}

// ★新規追加: 方向更新処理（プレイヤーのDirectionUpdate()を参考）
void Enemy::DirectionUpdate() {
    // 既存のRotateUpdate()を使用
    // 飛行中も地上と同じように方向更新を行う
    // 特別な処理が必要なければ、既存のRotateUpdate()がUpdate()内で呼ばれているのでOK
}

void Enemy::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    if (!isAlive_) {
        enemyCollider_->SetEnabled(false);
        return;
    }
    BaseObject::Draw(viewProjection, offSet);
    if (transform_->translation_.y < kGroundLevel) {
        return;
    }
    shadow_->SetIsModelDraw(drawShadow_);
    shadow_->Draw(viewProjection, offSet);
}

void Enemy::DrawParticle(const ViewProjection &viewProjection) {
    hitEmitter_->Draw(viewProjection);
}

void Enemy::Debug() {
#ifdef USE_IMGUI
    if (ImGui::BeginTabBar("敵情報")) {
        if (ImGui::BeginTabItem("敵情報")) {

            ImGui::Text("敵のHP %.1f / %.1f", HP_, maxHP_);
            if (ImGui::Button("HP回復")) {
                HP_ = maxHP_;
            }

            ImGui::Checkbox("ストップ", &isStop_);

            // ガード状態の表示
            ImGui::Separator();
            ImGui::Text("ガード状態: %s", isGuarding_ ? "ON" : "OFF");
            if (isGuarding_) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                                   "ダメージ85%%軽減中");
            }

            ImGui::EndTabItem();
        }

        // ★追加: ジャンプ・重力デバッグタブ
        if (ImGui::BeginTabItem("ジャンプ/重力")) {
            ImGui::Text("=== 状態 ===");
            ImGui::Text("地上判定: %s", isGrounded_ ? "地上" : "空中");
            ImGui::TextColored(
                isGrounded_ ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.5f, 0.2f, 1.0f),
                "%s", isGrounded_ ? "■ 地上" : "■ 空中");

            ImGui::Separator();
            ImGui::Text("=== 位置 ===");
            ImGui::Text("Y座標: %.2f", transform_->translation_.y);

            ImGui::Separator();
            ImGui::Text("=== 速度 ===");
            ImGui::Text("垂直速度 (velocity.y): %.2f", velocity_.y);
            ImGui::TextColored(
                velocity_.y > 0.0f ? ImVec4(0.2f, 0.5f, 1.0f, 1.0f) : velocity_.y < 0.0f ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
                                                                                         : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                "%s",
                velocity_.y > 0.0f ? "↑ 上昇中" : velocity_.y < 0.0f ? "↓ 落下中"
                                                                     : "→ 静止");

            ImGui::Separator();
            ImGui::Text("=== 加速度 ===");
            ImGui::Text("垂直加速度 (acceleration.y): %.2f", acceleration_.y);
            if (acceleration_.y < 0.0f) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "↓ 重力適用中");
            } else if (acceleration_.y > 0.0f) {
                ImGui::TextColored(ImVec4(0.2f, 0.5f, 1.0f, 1.0f), "↑ 上昇加速中");
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "- 加速度なし");
            }

            ImGui::Separator();
            ImGui::Text("=== パラメータ ===");
            ImGui::DragFloat("重力加速度 (fallSpeed)", &fallSpeed_, 0.5f, 1.0f, 100.0f);
            ImGui::DragFloat("ジャンプ力 (jumpSpeed)", &jumpSpeed_, 0.5f, 1.0f, 50.0f);
            ImGui::DragFloat("移動速度 (moveSpeed)", &moveSpeed_, 0.1f, 0.0f, 20.0f);

            ImGui::Separator();
            if (ImGui::Button("手動ジャンプテスト")) {
                if (isGrounded_) {
                    velocity_.y = jumpSpeed_;
                    isGrounded_ = false;
                    acceleration_.y = -fallSpeed_;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("リセット（地上に戻す）")) {
                transform_->translation_.y = 0.0f;
                velocity_.y = 0.0f;
                acceleration_.y = 0.0f;
                isGrounded_ = true;
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
#endif // USE_IMGUI
}

void Enemy::OnCollisionEnter(ColliderBase *other) {

    if (other->GetTag() == "PlayerBullet" || other->GetTag() == "PlayerChargeBullet" || other->GetTag() == "Makan") {
        hitEmitter_->SetPosition(transform_->translation_);
        hitEmitter_->UpdateOnce();
    }
    if (other->GetTag() == "PlayerChargeBullet" || other->GetTag() == "Makan") {
        chargeShake_->StartShake();
    }

    // ダメージリアクション開始
    isDamageReact_ = true;
    damageReactTimer_ = kTimerReset;

    // 今の向きを保存
    baseRotation_ = transform_->quateRotation_;

    // X軸回転のみをイージング(上向きに8度)
    float startAngle = transform_->quateRotation_.x;
    float endAngle = degreesToRadians(kDamageTiltDegrees);
    tiltEase_.Reset(startAngle, endAngle, damageReactDuration_, EasingType::OutQuad);
}

Vector3 Enemy::GetMovementDirection() const {
    return Vector3();
}

float Enemy::GetVelocityMagnitude() const {
    return kVelocityZero;
}

void Enemy::Save() {
}

void Enemy::Load() {
}

void Enemy::UpdateShadowScale() {
    if (transform_->translation_.y < kGroundLevel) {
        return;
    }
    float height = transform_->translation_.y;
    float baseScale = kShadowBaseScale;
    float scaleFactor = std::max(kShadowMinScale, baseScale - height * kShadowScaleFactor);
    shadow_->GetLocalScale() = {scaleFactor, scaleFactor, scaleFactor};
}

void Enemy::RotateUpdate() {
    // ターゲットがいなければ何もしない
    if (!target_) {
        return;
    }

    // 自分とターゲットのワールド座標を取得
    Vector3 toTarget = target_->GetWorldPosition() - GetWorldPosition();

    // ほぼ同じ位置なら回転しない
    if (toTarget.Length() < kMinRotationDistance) {
        return;
    }

    // 正規化して方向ベクトルに
    toTarget = toTarget.Normalize();

    // プレイヤーと同様に基準ベクトルを作成
    Vector3 forward = toTarget;                             // 敵の正面方向(ターゲット方向)
    Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ}; // 上方向
    Vector3 right;                                          // 右方向

    // forwardとupがほぼ平行なら補正
    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
        right = {kRightVectorX, kRightVectorY, kRightVectorZ};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }

    // upを再計算して正規直交化
    Vector3 up = (forward.Cross(right)).Normalize();

    // 回転行列からクォータニオンを生成
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    // 回転速度(大きいほど素早く向く)
    float rotateSpeed = kRotationSpeed;
    transform_->quateRotation_ = Quaternion::Slerp(
        transform_->quateRotation_,
        targetRot,
        rotateSpeed * Frame::DeltaTime());
}

void Enemy::CollisionGround() {
    float nextY = GetLocalPosition().y + velocity_.y * Frame::DeltaTime();

    transform_->translation_.x += velocity_.x * Frame::DeltaTime();
    transform_->translation_.z += velocity_.z * Frame::DeltaTime();

    if (nextY <= kGroundLevel) {
        transform_->translation_.y = kGroundLevel;

        if (!isGrounded_) {
            velocity_.y = kVelocityZero;
            isGrounded_ = true;
        }
    } else {
        transform_->translation_.y = nextY;
        isGrounded_ = false;
    }
}

Direction Enemy::CalculateDirectionFromRotation() {
    return Direction();
}

const char *Enemy::GetDirectionName(Direction dir) {
    return nullptr;
}

Vector3 Enemy::GetForward() const {
    // クォータニオンから前方向ベクトルを計算(Z軸の負方向が前方向)
    return TransformNormal(Vector3(kForwardVectorX, kForwardVectorY, kForwardVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetBackward() const {
    return -GetForward();
}

Vector3 Enemy::GetRight() const {
    // クォータニオンから右方向ベクトルを計算(X軸の正方向が右方向)
    return TransformNormal(Vector3(kRightVectorX, kRightVectorY, kRightVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetLeft() const {
    return -GetRight();
}

Vector3 Enemy::GetUp() const {
    // クォータニオンから上方向ベクトルを計算(Y軸の正方向が上方向)
    return TransformNormal(Vector3(kUpVectorX, kUpVectorY, kUpVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetDown() const {
    return -GetUp();
}

Vector3 Enemy::GetPositionBehind(float distance) const {
    return transform_->translation_ + GetBackward() * distance;
}

Vector3 Enemy::GetPositionFront(float distance) const {
    return transform_->translation_ + GetForward() * distance;
}

Vector3 Enemy::GetPositionRight(float distance) const {
    return transform_->translation_ + GetRight() * distance;
}

Vector3 Enemy::GetPositionLeft(float distance) const {
    return transform_->translation_ + GetLeft() * distance;
}

Vector3 Enemy::GetPositionAbove(float distance) const {
    return transform_->translation_ + GetUp() * distance;
}

Vector3 Enemy::GetPositionBelow(float distance) const {
    return transform_->translation_ + GetDown() * distance;
}

void Enemy::SetVp(ViewProjection *vp) {
    chargeShake_->Initialize(vp, "chagehit");
}