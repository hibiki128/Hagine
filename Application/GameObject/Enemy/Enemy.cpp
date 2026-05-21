#define NOMINMAX
#include "Enemy.h"
#include "Collider/CollisionManager.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChargeShot/ChargeShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Debug/Log/Logger.h>
#include <Engine/3d/Line/DrawLine3D.h>
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
    enemyCollider_->AddCollisionMask("PlayerHand"); // PlayerAttackColliderのタグ
    enemyCollider_->AddCollisionMask("PlayerChargeBullet");
    enemyCollider_->AddCollisionMask("makan");
    enemyCollider_->AddCollisionMask("CylinderField");

    enemyWallCollider_ = AddAABBCollider("enemy_WallCollider");
    enemyWallCollider_->SetTag("EnemyWall");
    enemyWallCollider_->AddCollisionMask("PlayerWall");
    enemyWallCollider_->SetSize({2.75f, 1000.0f, 2.5f});

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

    // -----------------------------------------------
    // 手の生成（ビジュアルと攻撃判定を担う）
    // -----------------------------------------------
    leftHand_ = std::make_unique<EnemyHand>();
    leftHand_->Init("enemy_leftHand");

    rightHand_ = std::make_unique<EnemyHand>();
    rightHand_->Init("enemy_rightHand");
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

    // -----------------------------------------------
    // 前方攻撃判定コライダーの生成
    // EnemyHandのコライダーは無効化し、こちらで近接攻撃の判定を一元管理する
    // PlayerAttackColliderと対称の設計
    // -----------------------------------------------
    attackCollider_ = std::make_unique<EnemyAttackCollider>();
    attackCollider_->Init(this);

    // -----------------------------------------------
    // コンボ登録
    // ダメージ・ノックバックをAdd()で指定し、後でImGuiで調整してセーブ可能
    // -----------------------------------------------
    if (!comboInitialized_) {
        punchCombo_.SetName("EnemyPunchCombo"); // DataHandlerのファイル名

        punchCombo_
            .Add(GetRightHand(), "Jab", 8.0f, 2.0f, 0.25f, 0.08f)
            .Add(GetLeftHand(), "Hook", 10.0f, 3.0f, 0.25f, 0.08f)
            .Add(GetRightHand(), "Cross", 10.0f, 3.0f, 0.25f, 0.08f)
            .Add(GetLeftHand(), "Uppercut", 12.0f, 5.0f, 0.30f, 0.10f)
            .Add(GetRightHand(), "Overhand", 12.0f, 5.0f, 0.30f, 0.10f)
            .Add(GetLeftHand(), "Swing", 14.0f, 6.0f, 0.30f, 0.10f)
            .Add(GetRightHand(), "Elbow", 16.0f, 7.0f, 0.25f, 0.06f)
            .Add(GetLeftHand(), "Slam", 20.0f, 10.0f, 0.35f, 0.12f);

        // JSONがあれば保存済みの値で上書き
        punchCombo_.LoadAttackParams();

        // -----------------------------------------------
        // 攻撃発火コールバック
        // ComboSystemが次の攻撃を実行するとき、ダメージ・ノックバックを
        // currentAttackDamage_ / currentAttackKnockback_ に保存する
        // EnemyHandのOnCollisionEnterがGetCurrentAttackDamage()で参照する
        // -----------------------------------------------
        punchCombo_.SetOnAttackFired(
            [this](float damage, float knockback, float duration, float /*delay*/) {
                currentAttackDamage_ = damage;
                currentAttackKnockback_ = knockback;
                currentAttackDuration_ = duration;
                // -----------------------------------------------
                // 前方攻撃判定コライダーをここで直接 Activate する
                // タイミングはコンボシステムが管理し、
                // ConboUpdate の IsObjectAttackCompleted に依存しない
                // -----------------------------------------------
                if (attackCollider_) {
                    attackCollider_->Activate(damage, knockback, duration);
                }
            });

        comboInitialized_ = true;
    }

    isGrounded_ = true;
    velocity_ = Vector3(0.0f, 0.0f, 0.0f);
    acceleration_ = Vector3(0.0f, 0.0f, 0.0f);

    transform_->SetRotationEuler({0.0f, degreesToRadians(180.0f), 0.0f});
}

void Enemy::Update() {
    // 影の位置を更新
    shadow_->GetLocalPosition() = {
        transform_->translation_.x, kShadowYPosition, transform_->translation_.z};
    shadow_->Update();

    // 開始フラグが立っており、ポーズ中でなく、ターゲットが生きている場合に更新
    if (started_ && !isPause_ && target_->GetIsAlive()) {
        DamageUpdate();
        RecoverEnergy();

        // 死亡判定
        if (HP_ <= kMinHP) {
            isAlive_ = false;
            HP_ = kMinHP;
        }

        // ガード中のエフェクト（点滅）
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

        // ダメージリアクション中でなければ回転を更新
        if (!isDamageReact_) {
            RotateUpdate();
        }

        ConboUpdate();
        UpdateShadowScale();
        chargeShake_->Update();

        // ダメージリアクション処理（のけぞり回転と点滅）
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

        // ビヘイビアツリーの更新
        if (rootNode_) {
            rootNode_->SetContext(this, target_);
            rootNode_->Tick();

            // 速度イージングの更新
            if (velocityEase_.isActive) {
                Vector3 easedVelocity = velocityEase_.Update(Frame::DeltaTime());
                velocity_.x = easedVelocity.x;
                velocity_.z = easedVelocity.z;
            }

            // 重力処理
            if (!isGrounded_ && !isFlying_) {
                velocity_.y += acceleration_.y * Frame::DeltaTime();
            } else if (isGrounded_) {
                acceleration_.y = 0.0f;
            }
        } else {
            // ルートノードがなければ停止
            velocity_.x = 0.0f;
            velocity_.z = 0.0f;
            if (isGrounded_) {
                velocity_.y = 0.0f;
                acceleration_.y = 0.0f;
            }
        }

        // 接地判定と位置更新
        CollisionGround();
        BaseObject::Update();
        UpdateFrustumLockOn();

        // 弾の更新
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
    // ターゲットへの方向を計算
    Vector3 direction = targetPos - transform_->translation_;
    direction.y = 0;
    direction = direction.Normalize();
    // 目標速度を設定し、イージングを開始
    velocityTarget_ = direction * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void Enemy::MoveStrafe() {
    if (!target_)
        return;
    // 横移動方向へ速度を設定
    Vector3 right = GetRight();
    velocityTarget_ = right * (float)strafeDirection_ * moveSpeed_;
    velocityEase_.Reset(velocity_, velocityTarget_, kVelocityEaseTime, EasingType::OutQuad);
}

void Enemy::MoveRetreat() {
    if (!target_)
        return;
    // ターゲットから離れる方向へ速度を設定
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
    // 停止目標速度を設定
    Vector3 zeroVel(0.0f, velocity_.y, 0.0f);
    velocityTarget_ = zeroVel;
    velocityEase_.Reset(velocity_, velocityTarget_, kStopEaseTime, EasingType::OutQuad);
}

void Enemy::Move() {
    if (!target_)
        return;
}

void Enemy::DirectionUpdate() {}

void Enemy::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    if (!isAlive_) {
        enemyCollider_->SetEnabled(false);
        leftHand_ptr_->SetIsAlive(false);
        rightHand_ptr_->SetIsAlive(false);
        return;
    }
    BaseObject::Draw(viewProjection, offSet);
    if (transform_->translation_.y < kGroundLevel)
        return;
    shadow_->SetIsModelDraw(drawShadow_);
    shadow_->Draw(viewProjection, offSet);
}

void Enemy::DrawParticle(const ViewProjection &viewProjection) {
    hitEmitter_->Draw(viewProjection);
    // -----------------------------------------------
    // 前方攻撃判定コライダーのヒットエフェクト描画
    // -----------------------------------------------
    if (attackCollider_) {
        attackCollider_->DrawParticle(viewProjection);
    }
    for (auto &bullet : bullets_) {
        bullet->DrawParticle(viewProjection);
    }
}

void Enemy::Debug() {
#ifdef USE_IMGUI
    if (ImGui::BeginTabBar("EnemyTabs")) {

        // ──────────────────────────────────────────
        // 基本情報タブ
        // ──────────────────────────────────────────
        if (ImGui::BeginTabItem("基本情報")) {
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
            if (ImGui::SmallButton("全回復##hp"))
                HP_ = maxHP_;

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
            if (ImGui::SmallButton("全回復##energy"))
                energy_ = maxEnergy_;

            ImGui::Spacing();
            if (ImGui::Button("HP・Energy 全回復")) {
                HP_ = maxHP_;
                energy_ = maxEnergy_;
            }

            ImGui::Separator();
            ImGui::Text("位置: (%.2f, %.2f, %.2f)",
                        transform_->translation_.x, transform_->translation_.y, transform_->translation_.z);
            ImGui::Text("速度: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y, velocity_.z);
            ImGui::Text("地上判定: %s", isGrounded_ ? "地上" : "空中");
            ImGui::Separator();

            ImGui::Text("【視錐台ロックオン】");
            if (isLockOn_) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "ロックオン: ON");
                if (ImGui::SmallButton("解除##frustum"))
                    ReleaseLockOn();
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "ロックオン: OFF");
            }
            ImGui::Checkbox("視錐台デバッグ描画", &drawFrustumDebug_);
            if (target_) {
                float dist = (target_->GetLocalPosition() - transform_->translation_).Length();
                ImGui::Text("プレイヤーまでの距離: %.2f / %.1f", dist, frustumLockOnRange_);
            }
            const float kToDeg = 180.0f / 3.14159265f;
            const float kToRad = 3.14159265f / 180.0f;
            ImGui::DragFloat("有効距離##frustum", &frustumLockOnRange_, 0.5f, 1.0f, 300.0f, "%.1f");
            float halfFovHDeg = frustumLockOnHalfFovH_ * kToDeg;
            float halfFovVDeg = frustumLockOnHalfFovV_ * kToDeg;
            if (ImGui::DragFloat("水平半角 (度)##frustum", &halfFovHDeg, 0.5f, 1.0f, 89.0f, "%.1f"))
                frustumLockOnHalfFovH_ = halfFovHDeg * kToRad;
            if (ImGui::DragFloat("垂直半角 (度)##frustum", &halfFovVDeg, 0.5f, 1.0f, 89.0f, "%.1f"))
                frustumLockOnHalfFovV_ = halfFovVDeg * kToRad;
            ImGui::Text("  水平全角: %.1f°  垂直全角: %.1f°", halfFovHDeg * 2.0f, halfFovVDeg * 2.0f);

            ImGui::Separator();
            ImGui::Checkbox("ストップ", &isStop_);
            ImGui::Separator();
            ImGui::Text("ガード状態: %s", isGuarding_ ? "ON" : "OFF");
            if (isGuarding_) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "ダメージ85%%軽減中");
            }
            ImGui::EndTabItem();
        }

        // ──────────────────────────────────────────
        // ジャンプ/重力タブ
        // ──────────────────────────────────────────
        if (ImGui::BeginTabItem("ジャンプ/重力")) {
            ImGui::Text("=== 状態 ===");
            ImGui::TextColored(
                isGrounded_ ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.5f, 0.2f, 1.0f),
                "%s", isGrounded_ ? "■ 地上" : "■ 空中");
            ImGui::Separator();
            ImGui::Text("Y座標: %.2f  垂直速度: %.2f  垂直加速度: %.2f",
                        transform_->translation_.y, velocity_.y, acceleration_.y);
            ImGui::Separator();
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

        // ──────────────────────────────────────────
        // コンボ攻撃パラメータタブ（新規）
        // ──────────────────────────────────────────
        if (ImGui::BeginTabItem("コンボパラメータ")) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                               "現在の攻撃: ダメージ %.1f / ノックバック %.1f",
                               currentAttackDamage_, currentAttackKnockback_);
            ImGui::Separator();
            punchCombo_.DrawImGui();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
#endif
}

// -----------------------------------------------
// OnCollisionEnter
// PlayerAttackCollider（タグ "PlayerHand"）が当たった時の
// ビジュアルエフェクトをここで処理する
// ダメージ/ノックバックの実計算はPlayerAttackCollider側が行う
// -----------------------------------------------
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

    // -----------------------------------------------
    // プレイヤーの前方攻撃判定（PlayerAttackCollider）との衝突時
    // ヒットパーティクルを再生する
    // ダメージとノックバックはPlayerAttackCollider::OnCollisionEnterで適用済み
    // -----------------------------------------------
    if (other->GetTag() == "PlayerHand") {
        hitEmitter_->SetPosition(transform_->translation_);
        hitEmitter_->UpdateOnce();
    }
}

void Enemy::OnCollision(ColliderBase *other) {
    if (other->GetTag() == "CylinderField") {
        if (other->GetType() != ColliderType::Cylinder)
            return;
        auto *cyl = static_cast<CylinderCollider *>(other);
        Vector3 mtv;
        if (CollisionManager::GetInstance()->CalculateDepenetrationOBBCylinder(enemyCollider_, cyl, mtv)) {
            transform_->translation_ += mtv;
            Vector3 mtvDir = mtv.Normalize();
            float dot = velocity_.Dot(mtvDir);
            if (dot < 0.0f)
                velocity_ -= mtvDir * dot;
        }
        return;
    }

    if (other->GetType() != ColliderType::AABB)
        return;
    auto *otherAABB = static_cast<AABBCollider *>(other);
    Vector3 mtv;
    if (CollisionManager::GetInstance()->CalculateDepenetration(enemyWallCollider_, otherAABB, mtv)) {
        if (mtv.Length() < 0.0001f)
            return;
        transform_->translation_ += mtv;
        Vector3 mtvDir = mtv.Normalize();
        float dot = velocity_.Dot(mtvDir);
        if (dot < 0.0f)
            velocity_ -= mtvDir * dot;
    }
}

// -----------------------------------------------
// ConboUpdate
// コンボシステムの更新と前方攻撃判定コライダーの有効時間管理
// コライダーの Activate は SetOnAttackFired コールバックが行う
// -----------------------------------------------
void Enemy::ConboUpdate() {
    punchCombo_.Update(Frame::DeltaTime());

    if (isComboAttack_) {
        punchCombo_.TryExecuteCombo();
        isComboAttack_ = false;
    }

    // -----------------------------------------------
    // 前方攻撃判定コライダーの毎フレーム更新
    // 遅延・有効時間タイマーを進める
    // -----------------------------------------------
    if (attackCollider_) {
        attackCollider_->Update(Frame::DeltaTime());
    }

    // コンボが非アクティブになったらコライダーも強制無効化
    if (!punchCombo_.IsComboActive()) {
        if (attackCollider_) {
            attackCollider_->Deactivate();
        }
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
    float scale = std::max(kShadowMinScale, kShadowBaseScale - height * kShadowScaleFactor);
    shadow_->GetLocalScale() = {scale, scale, scale};
}

void Enemy::RotateUpdate() {
    if (!isLockOn_ || !target_)
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
        transform_->quateRotation_, targetRot, kRotationSpeed * Frame::DeltaTime());
}

void Enemy::CollisionGround() {
    float nextY = GetLocalPosition().y + velocity_.y * Frame::DeltaTime();

    transform_->translation_.x += velocity_.x * Frame::DeltaTime();
    transform_->translation_.z += velocity_.z * Frame::DeltaTime();

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

// -----------------------------------------------
// DamageUpdate
// ダメージ処理とノックバックをまとめて行う
// -----------------------------------------------
void Enemy::DamageUpdate() {
    if (damage_ <= kNoDamage) {
        // ダメージがなくてもノックバックだけ適用する場合に備えてチェック
        if (hasKnockback_) {
            velocity_.x += pendingKnockback_.x;
            velocity_.y += pendingKnockback_.y;
            velocity_.z += pendingKnockback_.z;
            // velocityEaseをキャンセルしてノックバックが上書きされないようにする
            velocityEase_.isActive = false;
            // ノックバックでXZ方向に飛ぶ場合、地上判定を解除して浮かせる
            if (isGrounded_ && (pendingKnockback_.y > 0.0f)) {
                isGrounded_ = false;
                acceleration_.y = -fallSpeed_;
            }
            hasKnockback_ = false;
            pendingKnockback_ = {0.0f, 0.0f, 0.0f};
        }
        return;
    }

    // ダメージ計算（ガード時は軽減）
    float actualDamage = damage_;
    if (isGuarding_) {
        actualDamage *= kGuardDamageMultiplier;
    }
    HP_ -= actualDamage;
    damage_ = kNoDamage;

    // -----------------------------------------------
    // ノックバック適用
    // SetKnockback()で設定されたペンディング速度をvelocityに加算する
    // ガード中はノックバックも軽減する
    // -----------------------------------------------
    if (hasKnockback_) {
        float knockbackMult = isGuarding_ ? kGuardDamageMultiplier : 1.0f;
        velocity_.x += pendingKnockback_.x * knockbackMult;
        velocity_.y += pendingKnockback_.y * knockbackMult;
        velocity_.z += pendingKnockback_.z * knockbackMult;

        // velocityEaseをキャンセルしてBTの動きがノックバックを上書きしないようにする
        velocityEase_.isActive = false;

        // ノックバックに上方成分があれば空中に飛ばす
        if (isGrounded_ && pendingKnockback_.y > 0.0f) {
            isGrounded_ = false;
            acceleration_.y = -fallSpeed_;
        }
        hasKnockback_ = false;
        pendingKnockback_ = {0.0f, 0.0f, 0.0f};
    }

    // ダメージリアクション開始
    StartDamageReact();
}

// -----------------------------------------------
// SetKnockback
// PlayerAttackCollider::OnCollisionEnterから呼ばれる
// DamageUpdateのタイミングで一括処理するためペンディングに積む
// -----------------------------------------------
void Enemy::SetKnockback(const Vector3 &direction, float power) {
    if (power <= 0.0f) {
        return;
    }

    Vector3 dir = direction;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f) {
        return;
    }
    dir.x /= len;
    dir.y /= len;
    dir.z /= len;

    // ノックバック: 水平方向に強く + 少し上方に浮かせる
    pendingKnockback_ = {
        dir.x * power,
        power * 0.3f, // 上方への浮き（固定割合）
        dir.z * power,
    };
    hasKnockback_ = true;
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

void Enemy::UpdateFrustumLockOn() {
    if (!target_ || isLockOn_)
        return;

    Matrix4x4 rotMat = QuaternionToMatrix4x4(transform_->quateRotation_);
    const Vector3 origin = transform_->translation_;
    const Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
    const Vector3 right = {rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2]};
    const Vector3 up = {rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2]};

    Vector3 toTarget = target_->GetLocalPosition() - origin;
    float distance = toTarget.Length();
    if (distance < kMinRotationDistance || distance > frustumLockOnRange_)
        return;

    float dotF = toTarget.Dot(forward);
    if (dotF <= kVelocityZero)
        return;

    float tanH = toTarget.Dot(right) / dotF;
    if (std::abs(tanH) > std::tan(frustumLockOnHalfFovH_))
        return;

    float tanV = toTarget.Dot(up) / dotF;
    if (std::abs(tanV) > std::tan(frustumLockOnHalfFovV_))
        return;

    isLockOn_ = true;
}

void Enemy::DrawFrustum() {
#ifdef USE_IMGUI
    if (!drawFrustumDebug_)
        return;

    DrawLine3D *drawLine3D = DrawLine3D::GetInstance();
    if (!drawLine3D)
        return;

    Matrix4x4 rotMat = QuaternionToMatrix4x4(transform_->quateRotation_);
    const Vector3 origin = transform_->translation_;
    const Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
    const Vector3 right = {rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2]};
    const Vector3 up = {rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2]};

    const Vector4 color = isLockOn_ ? Vector4{1.0f, 0.5f, 0.0f, 1.0f}
                                    : Vector4{1.0f, 1.0f, 1.0f, 0.8f};
    const Vector4 axisColor = isLockOn_ ? Vector4{1.0f, 0.5f, 0.0f, 0.5f}
                                        : Vector4{1.0f, 1.0f, 1.0f, 0.4f};

    static constexpr float kFrustumDebugNear = 1.0f;
    auto CalcCorners = [&](float depth, std::array<Vector3, 4> &corners) {
        float halfH = depth * std::tan(frustumLockOnHalfFovH_);
        float halfV = depth * std::tan(frustumLockOnHalfFovV_);
        Vector3 center = origin + forward * depth;
        corners[0] = center - right * halfH + up * halfV;
        corners[1] = center + right * halfH + up * halfV;
        corners[2] = center + right * halfH - up * halfV;
        corners[3] = center - right * halfH - up * halfV;
    };

    std::array<Vector3, 4> nearCorners, farCorners;
    CalcCorners(kFrustumDebugNear, nearCorners);
    CalcCorners(frustumLockOnRange_, farCorners);

    for (int i = 0; i < 4; ++i) {
        drawLine3D->SetPoints(nearCorners[i], nearCorners[(i + 1) % 4], color);
        drawLine3D->SetPoints(farCorners[i], farCorners[(i + 1) % 4], color);
    }
    for (int i = 0; i < 4; ++i) {
        drawLine3D->SetPoints(nearCorners[i], farCorners[i], color);
    }
    drawLine3D->SetPoints(origin, origin + forward * frustumLockOnRange_, axisColor);
#endif
}

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
    timeSinceLastShot_ += Frame::DeltaTime();
    if (timeSinceLastShot_ >= energyRecoveryDelay_) {
        energy_ += energyRecoveryRate_ * Frame::DeltaTime();
        if (energy_ > maxEnergy_)
            energy_ = maxEnergy_;
    }
}

void Enemy::Shot() {
    if (!target_ || !ConsumeEnergy(kNormalShotEnergyCost))
        return;
    std::string bulletName = "EnemyBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(this);
    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);
    bullets_.push_back(std::move(bullet));
}

void Enemy::ShotWithDirection(const Vector3 &direction, bool forceHoming) {
    if (!target_ || !ConsumeEnergy(kNormalShotEnergyCost))
        return;

    std::string bulletName = "EnemyBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);

    bool prevLockOn = isLockOn_;
    isLockOn_ = false;
    bullet->InitTransform(this);
    isLockOn_ = prevLockOn;

    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);

    if (forceHoming) {
        bullet->SetIsLockOnBullet(true);
        if (target_) {
            Vector3 toTarget = target_->GetLocalPosition() - GetLocalPosition();
            float len = toTarget.Length();
            if (len > kMinRotationDistance)
                toTarget = toTarget / len;
            else
                toTarget = GetForward();
            bullet->SetVelocity(toTarget * bullet->GetCurrentSpeed());
        }
    } else {
        bullet->SetIsLockOnBullet(false);
        bullet->SetVelocity(-direction * bullet->GetCurrentSpeed());
    }

    bullets_.push_back(std::move(bullet));
}

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