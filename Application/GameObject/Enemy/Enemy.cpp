#define NOMINMAX
#include "Enemy.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChargeShot/ChargeShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include <Debug/Log/Logger.h>
#include <Frame.h>

Enemy::Enemy() {}
Enemy::~Enemy() {}

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
    shadow_->GetWorldTransform()->SetRotationEuler(
        Vector3(degreesToRadians(kShadowRotationDegrees), kRotationZero, kRotationZero));
    shadow_->GetLocalScale() = {kShadowScale, kShadowScale, kShadowScale};
    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("smokeEmitter");
    chargeShake_ = std::make_unique<Shake>();
    isGuarding_ = false;

    // 物理パラメータの初期化
    fallSpeed_ = 30.0f;
    moveSpeed_ = 5.0f;
    jumpSpeed_ = 15.0f;
    maxSpeed_ = 10.0f;
    accelRate_ = 1.0f;

    isGrounded_ = true;
    velocity_ = Vector3(0.0f, 0.0f, 0.0f);
    acceleration_ = Vector3(0.0f, 0.0f, 0.0f);
}

void Enemy::Update() {
    shadow_->GetLocalPosition() = {
        transform_->translation_.x, kShadowYPosition, transform_->translation_.z};
    shadow_->Update();

    if (started_) {
        if (damage_ > kNoDamage) {
            float actualDamage = static_cast<float>(damage_);
            if (isGuarding_)
                actualDamage *= kGuardDamageMultiplier;
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

        if (!isDamageReact_) {
            RotateUpdate();
        }

        UpdateShadowScale();
        chargeShake_->Update();

        if (isDamageReact_) {
            damageReactTimer_ += Frame::DeltaTime();
            float angleX = tiltEase_.Update(Frame::DeltaTime());
            tiltRotation_ = Quaternion::FromAxisAngle(
                Vector3(kXAxisX, kXAxisY, kXAxisZ), angleX);
            transform_->quateRotation_ = tiltRotation_ * baseRotation_;

            float blinkInterval = kDamageBlinkInterval;
            int blink = static_cast<int>(damageReactTimer_ / blinkInterval);
            SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kAlphaTransparent : kAlphaOpaque);

            if (damageReactTimer_ >= damageReactDuration_) {
                isDamageReact_ = false;
                transform_->quateRotation_ = baseRotation_;
                SetAlpha(kAlphaOpaque);
            }
        }

        if (rootNode_) {
            rootNode_->SetContext(this, target_);
            rootNode_->Tick();

            if (velocityEase_.isActive) {
                Vector3 easedVelocity = velocityEase_.Update(Frame::DeltaTime());
                velocity_.x = easedVelocity.x;
                velocity_.z = easedVelocity.z;
                // velocity_.y は重力を優先するため上書きしない
            }

            if (!isGrounded_) {
                velocity_.y += acceleration_.y * Frame::DeltaTime();
            } else {
                acceleration_.y = 0.0f;
            }
        } else {
            velocity_.x = 0.0f;
            velocity_.z = 0.0f;
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
    if (!target_)
        return;

    Vector3 direction = targetPos - transform_->translation_;
    direction.y = 0;
    direction = direction.Normalize();

    velocityTarget_ = direction * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void Enemy::MoveStrafe() {
    if (!target_)
        return;

    Vector3 right = GetRight();
    velocityTarget_ = right * (float)strafeDirection_ * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void Enemy::MoveRetreat() {
    if (!target_)
        return;

    Vector3 direction = transform_->translation_ - target_->GetWorldPosition();
    direction.y = 0;
    direction = direction.Normalize();

    velocityTarget_ = direction * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void Enemy::PerformAttack() {
    Logger::Log("Attack\n");
}

void Enemy::StopMovement() {
    Vector3 zeroVel(0.0f, velocity_.y, 0.0f);
    velocityTarget_ = zeroVel;
    velocityEase_.Reset(velocity_, velocityTarget_, kStopEaseTime, EasingType::OutQuad);
}

void Enemy::Move() {
    if (!target_)
        return;
    // 横方向の移動は既存の velocity で制御
}

void Enemy::DirectionUpdate() {
    // RotateUpdate() は Update() 内で自動的に呼ばれるため、ここでは何もしない
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
    if (ImGui::BeginTabBar("EnemyTabs")) {
        if (ImGui::BeginTabItem("基本情報")) {
            ImGui::Text("HP: %.1f / %.1f", HP_, maxHP_);
            ImGui::Text("位置: (%.2f, %.2f, %.2f)",
                        transform_->translation_.x,
                        transform_->translation_.y,
                        transform_->translation_.z);
            ImGui::Text("速度: (%.2f, %.2f, %.2f)",
                        velocity_.x, velocity_.y, velocity_.z);
            ImGui::Text("地上判定: %s", isGrounded_ ? "地上" : "空中");

            ImGui::Separator();
            ImGui::Checkbox("ストップ", &isStop_);

            ImGui::Separator();
            ImGui::Text("ガード状態: %s", isGuarding_ ? "ON" : "OFF");
            if (isGuarding_) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "ダメージ85%%軽減中");
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ジャンプ/重力")) {
            ImGui::Text("=== 状態 ===");
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
                velocity_.y > 0.0f
                    ? ImVec4(0.2f, 0.5f, 1.0f, 1.0f)
                : velocity_.y < 0.0f
                    ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
                    : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                "%s",
                velocity_.y > 0.0f   ? "↑ 上昇中"
                : velocity_.y < 0.0f ? "↓ 落下中"
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
    if (other->GetTag() == "PlayerBullet" ||
        other->GetTag() == "PlayerChargeBullet" ||
        other->GetTag() == "Makan") {
        hitEmitter_->SetPosition(transform_->translation_);
        hitEmitter_->UpdateOnce();
    }
    if (other->GetTag() == "PlayerChargeBullet" ||
        other->GetTag() == "Makan") {
        chargeShake_->StartShake();
    }

    isDamageReact_ = true;
    damageReactTimer_ = kTimerReset;
    baseRotation_ = transform_->quateRotation_;

    float startAngle = transform_->quateRotation_.x;
    float endAngle = degreesToRadians(kDamageTiltDegrees);
    tiltEase_.Reset(startAngle, endAngle, damageReactDuration_, EasingType::OutQuad);
}

Vector3 Enemy::GetMovementDirection() const { return Vector3(); }

float Enemy::GetVelocityMagnitude() const { return kVelocityZero; }

void Enemy::Save() {}
void Enemy::Load() {}

void Enemy::UpdateShadowScale() {
    if (transform_->translation_.y < kGroundLevel)
        return;
    float height = transform_->translation_.y;
    float baseScale = kShadowBaseScale;
    float scale = std::max(kShadowMinScale, baseScale - height * kShadowScaleFactor);
    shadow_->GetLocalScale() = {scale, scale, scale};
}

void Enemy::RotateUpdate() {
    if (!target_)
        return;

    Vector3 toTarget = target_->GetWorldPosition() - GetWorldPosition();
    if (toTarget.Length() < kMinRotationDistance)
        return;

    toTarget = toTarget.Normalize();
    Vector3 forward = toTarget;
    Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ};
    Vector3 right;

    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
        right = {kRightVectorX, kRightVectorY, kRightVectorZ};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }

    Vector3 up = (forward.Cross(right)).Normalize();

    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    transform_->quateRotation_ = Quaternion::Slerp(
        transform_->quateRotation_, targetRot,
        kRotationSpeed * Frame::DeltaTime());
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

Direction Enemy::CalculateDirectionFromRotation() { return Direction(); }
const char *Enemy::GetDirectionName(Direction dir) { return nullptr; }

Vector3 Enemy::GetForward() const {
    return TransformNormal(
        Vector3(kForwardVectorX, kForwardVectorY, kForwardVectorZ),
        QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetBackward() const { return -GetForward(); }

Vector3 Enemy::GetRight() const {
    return TransformNormal(
        Vector3(kRightVectorX, kRightVectorY, kRightVectorZ),
        QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetLeft() const { return -GetRight(); }

Vector3 Enemy::GetUp() const {
    return TransformNormal(
        Vector3(kUpVectorX, kUpVectorY, kUpVectorZ),
        QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetDown() const { return -GetUp(); }

Vector3 Enemy::GetPositionBehind(float distance) const { return transform_->translation_ + GetBackward() * distance; }
Vector3 Enemy::GetPositionFront(float distance) const { return transform_->translation_ + GetForward() * distance; }
Vector3 Enemy::GetPositionRight(float distance) const { return transform_->translation_ + GetRight() * distance; }
Vector3 Enemy::GetPositionLeft(float distance) const { return transform_->translation_ + GetLeft() * distance; }
Vector3 Enemy::GetPositionAbove(float distance) const { return transform_->translation_ + GetUp() * distance; }
Vector3 Enemy::GetPositionBelow(float distance) const { return transform_->translation_ + GetDown() * distance; }

void Enemy::SetVp(ViewProjection *vp) {
    chargeShake_->Initialize(vp, "chargehit");
}