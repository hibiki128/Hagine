#define NOMINMAX
#include "Enemy.h"
#include "Collider/CollisionManager.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChargeShot/ChargeShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Debug/Log/Logger.h>
#include <Frame.h>
#include <Object/Base/BaseObjectManager.h>

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
    enemyCollider_->AddCollisionMask("CylinderField");

    enemyWallCollider_ = AddAABBCollider("enemy_WallCollider");
    enemyWallCollider_->SetTag("EnemyWall");
    enemyWallCollider_->AddCollisionMask("PlayerWall");
    enemyWallCollider_->SetSize({2.0f, 1000.0f, 2.0f});

    enemyCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    enemyCollider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
    });

    enemyWallCollider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
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

    // 手の生成
    leftHand_ = std::make_unique<EnemyHand>();
    leftHand_->Init("enemy_leftHand");
    leftHand_->SetEnemy(this);

    rightHand_ = std::make_unique<EnemyHand>();
    rightHand_->Init("enemy_rightHand");
    rightHand_->SetEnemy(this);

    // 物理パラメータの初期化
    fallSpeed_ = 30.0f;
    moveSpeed_ = 5.0f;
    jumpSpeed_ = 15.0f;
    maxSpeed_ = 10.0f;
    accelRate_ = 1.0f;

    this->AddChild(leftHand_.get());
    this->AddChild(rightHand_.get());

    MotionEditor::GetInstance()->Register(leftHand_.get());
    MotionEditor::GetInstance()->Register(rightHand_.get());

    rightHand_ptr_ = rightHand_.get();
    leftHand_ptr_ = leftHand_.get();

    BaseObjectManager::GetInstance()->AddObject(std::move(leftHand_));
    BaseObjectManager::GetInstance()->AddObject(std::move(rightHand_));

    if (!comboInitialized_) {
        punchCombo_.Add(GetRightHand(), "Jab") // 1段目:右手ジャブ
            .Add(GetLeftHand(), "Hook")        // 2段目:左手フック
            .Add(GetRightHand(), "Cross")      // 3段目:右手クロス
            .Add(GetLeftHand(), "Uppercut")    // 4段目:左手アッパーカット
            .Add(GetRightHand(), "Overhand")   // 5段目:右手オーバーハンド
            .Add(GetLeftHand(), "Swing")       // 6段目:左手スイング
            .Add(GetRightHand(), "Elbow")      // 7段目:右手肘打ち
            .Add(GetLeftHand(), "Slam");       // 8段目:左手スラム

        comboInitialized_ = true;
    }

    isGrounded_ = true;
    velocity_ = Vector3(0.0f, 0.0f, 0.0f);
    acceleration_ = Vector3(0.0f, 0.0f, 0.0f);
}

void Enemy::Update() {
    shadow_->GetLocalPosition() = {
        transform_->translation_.x, kShadowYPosition, transform_->translation_.z};
    shadow_->Update();

    if (started_ && !isPause_&&target_->GetIsAlive()) {
        DamageUpdate();
        RecoverEnergy();

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

        ConboUpdate();
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

            if (!isGrounded_ && !isFlying_) {
                velocity_.y += acceleration_.y * Frame::DeltaTime();
            } else if (isGrounded_) {
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

        // 弾の更新と生存チェック
        for (auto it = bullets_.begin(); it != bullets_.end();) {
            (*it)->Update();
            (*it)->UpdateWorldTransformHierarchy();
            if (!(*it)->IsAlive()) {
                it = bullets_.erase(it);
            } else {
                ++it;
            }
        }
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
        leftHand_ptr_->SetIsAlive(false);
        rightHand_ptr_->SetIsAlive(false);
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

    leftHand_ptr_->DrawParticle(viewProjection);
    rightHand_ptr_->DrawParticle(viewProjection);

    for (auto &bullet : bullets_) {
        bullet->DrawParticle(viewProjection);
    }
}

void Enemy::Debug() {
#ifdef USE_IMGUI
    if (ImGui::BeginTabBar("EnemyTabs")) {
        if (ImGui::BeginTabItem("基本情報")) {

            // ── HP ──────────────────────────────────────
            ImGui::Text("HP");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::DragFloat("##HP", &HP_, 1.0f, 0.0f, maxHP_, "%.1f"))
                HP_ = std::clamp(HP_, kMinHP, maxHP_);
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::DragFloat("最大HP##maxHP", &maxHP_, 1.0f, 1.0f, 9999.0f, "%.1f"))
                HP_ = std::clamp(HP_, kMinHP, maxHP_);
            ImGui::SameLine();
            if (ImGui::SmallButton("全回復##hp")) {
                HP_ = maxHP_;
            }

            // ── Energy ──────────────────────────────────
            ImGui::Text("Energy");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::DragFloat("##Energy", &energy_, 1.0f, 0.0f, maxEnergy_, "%.1f"))
                energy_ = std::clamp(energy_, 0.0f, maxEnergy_);
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::DragFloat("最大Energy##maxEnergy", &maxEnergy_, 1.0f, 1.0f, 9999.0f, "%.1f"))
                energy_ = std::clamp(energy_, 0.0f, maxEnergy_);
            ImGui::SameLine();
            if (ImGui::SmallButton("全回復##energy")) {
                energy_ = maxEnergy_;
            }

            // ── 全回復ボタン ─────────────────────────────
            ImGui::Spacing();
            if (ImGui::Button("HP・Energy 全回復")) {
                HP_ = maxHP_;
                energy_ = maxEnergy_;
            }

            // ── その他 ──────────────────────────────────
            ImGui::Separator();
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
}

void Enemy::OnCollision(ColliderBase *other) {
    // フィールド円柱との押し戻し
    if (other->GetTag() == "CylinderField") {
        if (other->GetType() != ColliderType::Cylinder) {
            return;
        }
        auto *cyl = static_cast<CylinderCollider *>(other);
        Vector3 mtv;
        if (CollisionManager::GetInstance()->CalculateDepenetrationOBBCylinder(enemyCollider_, cyl, mtv)) {
            transform_->translation_ += mtv;
            Vector3 mtvDir = mtv.Normalize();
            float dot = velocity_.Dot(mtvDir);
            if (dot < 0.0f) {
                velocity_ -= mtvDir * dot;
            }
        }
        return;
    }

    // EnemyWall との押し戻し（AABB）
    if (other->GetType() != ColliderType::AABB) {
        return;
    }
    auto *otherAABB = static_cast<AABBCollider *>(other);
    Vector3 mtv;
    if (CollisionManager::GetInstance()->CalculateDepenetration(enemyWallCollider_, otherAABB, mtv)) {
        if (mtv.Length() < 0.0001f) {
            return;
        }
        transform_->translation_ += mtv;
        Vector3 mtvDir = mtv.Normalize();
        float dot = velocity_.Dot(mtvDir);
        if (dot < 0.0f) {
            velocity_ -= mtvDir * dot;
        }
    }
}

void Enemy::ConboUpdate() {
    punchCombo_.Update(Frame::DeltaTime());

    if (isComboAttack_) {
        punchCombo_.TryExecuteCombo();
        isComboAttack_ = false;
    }

    if (punchCombo_.IsComboActive()) {
        GetRightHand()->SetColliderEnabled(punchCombo_.IsObjectAttackCompleted(GetRightHand()));
        GetLeftHand()->SetColliderEnabled(punchCombo_.IsObjectAttackCompleted(GetLeftHand()));
    }
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
    // ロックオン中でない場合は回転しない
    if (!isLockOn_) {
        return;
    }

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

    // 飛行中は着地判定をスキップ（FlyToGroundノードが重力を復活させるまで）
    if (isFlying_) {
        transform_->translation_.y = nextY;
        return;
    }

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

void Enemy::DamageUpdate() {
    if (damage_ <= kNoDamage) {
        return;
    }

    float actualDamage = damage_;
    if (isGuarding_) {
        actualDamage *= kGuardDamageMultiplier;
    }
    HP_ -= actualDamage;
    damage_ = kNoDamage;

    // ダメージリアクションを開始
    StartDamageReact();
}

void Enemy::StartDamageReact() {
    isDamageReact_ = true;
    damageReactTimer_ = kTimerReset;
    baseRotation_ = transform_->quateRotation_;

    float startAngle = kRotationZero;
    float endAngle = degreesToRadians(kDamageTiltDegrees);
    tiltEase_.Reset(startAngle, endAngle, damageReactDuration_, EasingType::OutQuad);
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

void Enemy::SetEnergy(float energy) {
    energy_ = std::clamp(energy, 0.0f, maxEnergy_);
}

bool Enemy::ConsumeEnergy(float amount) {
    if (energy_ >= amount) {
        energy_ -= amount;
        timeSinceLastShot_ = kTimerReset;
        return true;
    }
    return false;
}

void Enemy::RecoverEnergy() {
    // 最後の射撃から一定時間経過していれば回復
    timeSinceLastShot_ += Frame::DeltaTime();
    if (timeSinceLastShot_ >= energyRecoveryDelay_) {
        energy_ += energyRecoveryRate_ * Frame::DeltaTime();
        if (energy_ > maxEnergy_) {
            energy_ = maxEnergy_;
        }
    }
}

void Enemy::Shot() {
    if (!target_) {
        return;
    }

    // エネルギーが不足している場合は発射しない
    if (!ConsumeEnergy(kNormalShotEnergyCost)) {
        return;
    }

    std::string bulletName = "EnemyBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(this);
    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);
    bullets_.push_back(std::move(bullet));
}

void Enemy::ShotWithDirection(const Vector3 &direction, bool forceHoming) {
    if (!target_) {
        return;
    }

    if (!ConsumeEnergy(kNormalShotEnergyCost)) {
        return;
    }

    std::string bulletName = "EnemyBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);

    // InitTransform はコライダー・初期座標・オフセットのセットアップに使う
    // ロックオンは常にOFFで呼び出し（ホーミング挙動を InitTransform に依存させない）
    bool prevLockOn = isLockOn_;
    isLockOn_ = false;
    bullet->InitTransform(this);
    isLockOn_ = prevLockOn;

    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);

    if (forceHoming) {
        // ホーミング：プレイヤーへの初期方向 + Update内での追従を有効化
        bullet->SetIsLockOnBullet(true);
        if (target_) {
            Vector3 toTarget = target_->GetLocalPosition() - GetLocalPosition();
            float len = toTarget.Length();
            if (len > kMinRotationDistance) {
                toTarget = toTarget / len;
            } else {
                toTarget = GetForward();
            }
            bullet->SetVelocity(toTarget * bullet->GetCurrentSpeed());
        }
    } else {
        // 拡散弾 or 直進弾：渡された direction をそのまま使い、ホーミングは無効
        bullet->SetIsLockOnBullet(false);
        float speed = bullet->GetCurrentSpeed();
        bullet->SetVelocity(-direction * speed);
    }

    bullets_.push_back(std::move(bullet));
}