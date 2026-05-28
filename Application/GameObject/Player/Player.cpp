#define NOMINMAX
#include "Player.h"
#include "Engine/Frame/Frame.h"
#include "Engine/Utility/Debug/ImGui/ImGuiNotification.h"
#include "State/Air/PlayerStateAir.h"
#include "State/Fly/PlayerStateFlyIdle.h"

#include "Bullet/ChargeShot/ChargeShot.h"
#include "Collider/CollisionManager.h"
#include "Engine/3d/Line/DrawLine3D.h"
#include "Object/Base/BaseObjectManager.h"
#include "State/Action/PlayerEnergyCharge.h"
#include "State/Action/PlayerStateGuard.h"
#include "State/Action/PlayerStateRush.h"
#include "State/Fly/PlayerStateFlyMove.h"
#include "State/Ground/PlayerStateIdle.h"
#include "State/Ground/PlayerStateJump.h"
#include "State/Ground/PlayerStateMove.h"
#include "application/Camera/FollowCamera.h"
#include "application/GameObject/Enemy/Enemy.h"
#include "numbers"
#include <Application/Utility/MotionEditor/MotionEditor.h>
#include <Particle/CSParticle/ParticleCSEditor.h>
#include <Particle/ParticleEditor.h>
#include <cmath>

Player::Player() {
}

Player::~Player() {
}

void Player::Init(const std::string objectName) {
    BaseObject::Init(objectName);
     BaseObject::CreatePrimitiveModel(PrimitiveType::Cube);
    ///BaseObject::CreateModel("animation/Player/Idle_Ground.gltf");
    // ────────────────────────────────────────────────
    // ループあり：待機・移動など継続する動作
    // ────────────────────────────────────────────────
    ///BaseObject::AddAnimation("animation/Player/Idle_Ground.gltf", true); // 浮遊待機
    ///BaseObject::AddAnimation("animation/Player/Idle_Flying.gltf", true); // 浮遊待機
    ///BaseObject::AddAnimation("animation/Run.gltf", true);                // 地上移動
    // ────────────────────────────────────────────────
    // ループなし：攻撃・ジャンプなど一回きりの動作
    // ────────────────────────────────────────────────
    ///BaseObject::AddAnimation("animation/Player/Punch_1.gltf", false); // Jab
    ///BaseObject::AddAnimation("animation/Player/Punch_2.gltf", false); // Hook
    ///BaseObject::AddAnimation("animation/Player/Punch_3.gltf", false); // Cross
    ///BaseObject::AddAnimation("animation/Player/Punch_4.gltf", false); // Uppercut
    ///BaseObject::AddAnimation("animation/Player/Kick_1.gltf", false);  // Overhand
    ///BaseObject::AddAnimation("animation/Player/Kick_2.gltf", false);  // Swing
    ///BaseObject::AddAnimation("animation/Player/Kick_3.gltf", false);  // Elbow
    ///BaseObject::AddAnimation("animation/Jump.gltf", false);           // ジャンプ

    ///BaseObject::SetOffset({0.0f, -0.5f, 0.0f}); // 描画オフセット（地面に足がつくように）
   /// BaseObject::GetLocalScale() = {3.0f, 3.0f, 3.0f};
   /// BaseObject::SetAnimationSpeed(1.5f);
   /// BaseObject::SetAnimationBlendDuration(0.75f);

    playerCollider_ = AddOBBCollider("player_Collider");
    playerCollider_->SetTag("Player");
    playerCollider_->AddCollisionMask("Enemy");
    playerCollider_->AddCollisionMask("CylinderField");

    playerWallCollider_ = AddAABBCollider("player_WallCollider");
    playerWallCollider_->SetTag("PlayerWall");
    playerWallCollider_->AddCollisionMask("EnemyWall");
    playerWallCollider_->SetSize({2.75f, 1000.0f, 2.5f});

    playerCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    playerCollider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
    });

    playerWallCollider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
    });

    states_["Idle"] = std::make_unique<PlayerStateIdle>();
    states_["Move"] = std::make_unique<PlayerStateMove>();
    states_["Jump"] = std::make_unique<PlayerStateJump>();
    states_["Air"] = std::make_unique<PlayerStateAir>();
    states_["FlyIdle"] = std::make_unique<PlayerStateFlyIdle>();
    states_["FlyMove"] = std::make_unique<PlayerStateFlyMove>();
    states_["Rush"] = std::make_unique<PlayerStateRush>();
    states_["EnergyCharge"] = std::make_unique<PlayerEnergyCharge>();
    states_["Guard"] = std::make_unique<PlayerStateGuard>();
    currentState_ = states_["Idle"].get();
    isGrounded_ = true; // 初期状態は地面にいる

    data_ = std::make_unique<DataHandler>("EntityData", "Player");
   /* shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetWorldTransform()->SetRotationEuler(Vector3(degreesToRadians(kShadowRotationDegrees), kRotationZero, kRotationZero));
    shadow_->GetLocalScale() = {kShadowScale, kShadowScale, kShadowScale};*/

    chargeShot_ = std::make_unique<ChargeShot>();
    chargeShot_->SetPlayer(this);
    chargeShot_->Init("chageShot");

    // 手の生成
    leftHand_ = std::make_unique<PlayerHand>();
    leftHand_->Init("leftHand");

    rightHand_ = std::make_unique<PlayerHand>();
    rightHand_->Init("rightHand");

    makanAttack_ = std::make_unique<MakanAttackSkill>();
    makanAttack_->Init("makanAttack");

    this->AddChild(leftHand_.get());
    this->AddChild(rightHand_.get());

    MotionEditor::GetInstance()->Register(leftHand_.get());
    MotionEditor::GetInstance()->Register(rightHand_.get());

    rightHand_ptr_ = rightHand_.get();
    leftHand_ptr_ = leftHand_.get();
    makanAttack_ptr_ = makanAttack_.get();

    BaseObjectManager::GetInstance()->AddObject(std::move(leftHand_));
    BaseObjectManager::GetInstance()->AddObject(std::move(rightHand_));
    BaseObjectManager::GetInstance()->AddObject(std::move(makanAttack_));

    Load();

    punchCombo_.SetName("PunchCombo"); // DataHandlerのファイル名に使われる
    punchCombo_
        .Add(GetRightHand(), "Jab", 10.0f, 3.0f, 0.25f, 0.08f)
        .Add(GetLeftHand(), "Hook", 12.0f, 4.0f, 0.25f, 0.08f)
        .Add(GetRightHand(), "Cross", 12.0f, 4.0f, 0.25f, 0.08f)
        .Add(GetLeftHand(), "Uppercut", 15.0f, 6.0f, 0.30f, 0.10f)
        .Add(GetRightHand(), "Overhand", 15.0f, 6.0f, 0.30f, 0.10f)
        .Add(GetLeftHand(), "Swing", 18.0f, 7.0f, 0.30f, 0.10f)
        .Add(GetRightHand(), "Elbow", 20.0f, 8.0f, 0.25f, 0.06f)
        .Add(GetLeftHand(), "Slam", 25.0f, 12.0f, 0.35f, 0.12f);

    punchCombo_.LoadAttackParams(); // JSONがあれば値を上書き読み込み

    // コンボ段ごとのプレイヤー本体アニメーション（適宜差し替え可）
    // Punch_1〜4 → パンチ系、Kick_1〜3 → キック系で割り振り
    // 対応するアニメーションがない段は空文字（何も再生しない）
    comboAnimations_ = {
        "animation/Player/Punch_1.gltf", // 1段目: Jab
        "animation/Player/Punch_2.gltf", // 2段目: Hook
        "animation/Player/Punch_3.gltf", // 3段目: Cross
        "animation/Player/Punch_4.gltf", // 4段目: Uppercut
        "animation/Player/Kick_1.gltf",  // 5段目: Overhand
        "animation/Player/Kick_2.gltf",  // 6段目: Swing
        "animation/Player/Punch_3.gltf",  // 7段目: Elbow
        "animation/Player/Kick_3.gltf",  // 8段目: Slam
    };

    shake_ = std::make_unique<Shake>();

    auraEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("playerAura");
    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("smokeEmitter");

    deathStaging_ = std::make_unique<DeathStaging>();

    input_ = Input::GetInstance();

    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);

    generatedField_ = ParticleCSFieldManager::GetInstance()->GetField(0); // 0番目のフィールドを使用

    attackCollider_ = std::make_unique<PlayerAttackCollider>();
    attackCollider_->Init(this);

    // コンボが攻撃を発火したとき attackCollider_ を有効化するコールバックを登録
    punchCombo_.SetOnAttackFired(
        [this](float damage, float knockback, float duration, float delay) {
            if (attackCollider_) {
                attackCollider_->Activate(damage, knockback, duration, delay);
            }
        });
}

void Player::Update() {

    if (activeDebugCamrera_)
        return;

    dt_ = Frame::DeltaTime();

    if (makanAttack_ptr_->IsActive()) {
        return;
    }

    if (!isAlive_) {
        // 死亡時：ダメージリアクションの回転をイージングでリセット
        if (isDeathRotationReset_) {
            deathRotationResetTimer_ += dt_;
            float t = deathRotationResetTimer_ / kDeathRotationResetDuration;
            if (t >= 1.0f) {
                t = 1.0f;
                isDeathRotationReset_ = false;
            }
            // EaseOutQuad: t を二次イージングで補間
            float easedT = 1.0f - (1.0f - t) * (1.0f - t);

            // Y軸回転（baseRotation_）のみ維持し、X軸傾き（tiltRotation_）を除去した目標回転へ
            Quaternion targetRotation = baseRotation_;
            transform_->quateRotation_ = Quaternion::Slerp(deathRotationStart_, targetRotation, easedT);
        }
    } else {
        generatedField_->data.position = GetWorldPosition();
        gamePad_->Update();

        //shadow_->GetLocalPosition() = {transform_->translation_.x, kShadowYPosition, transform_->translation_.z};
        //shadow_->Update();

        if (isInvincible_) {
            InvincibleUpdate();
        }

        DamageUpdate();

        if (started_ && !isPause_) {
            RecoverEnergy();
            ComboUpdate();
            if (attackCollider_) {
                attackCollider_->Update(dt_);
            }

            UpdateDashState();

            UpdateGuardInput();

            if (currentState_) {
                currentState_->Update(*this);
            }

           // UpdateAnimation(); // ステート・コンボに応じてアニメーションを切り替え

            // RotateUpdate はロックオン追従と手動スティック回転を行う
            // Rush ステートは自前で UpdateRotation() を持つため除外する
            if (currentState_ != states_["Rush"].get()) {
                RotateUpdate();
            }

            if (chargeShot_) {
                chargeShot_->SetIsSkillMenu(isSkillMenu_);
                chargeShot_->Update();
            }

            Shot();

            SkillShot();
        }

        // ダメージリアクション処理
        if (isDamageReact_) {
            damageReactTimer_ += dt_;

            float angleX = tiltEase_.Update(dt_);

            tiltRotation_ = Quaternion::FromAxisAngle(Vector3(kXAxisX, kXAxisY, kXAxisZ), angleX);

            transform_->quateRotation_ = tiltRotation_ * baseRotation_;

            // 高速点滅
            float blinkInterval = kPlayerBlinkInterval;
            int blink = static_cast<int>(damageReactTimer_ / blinkInterval);
            SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kPlayerAlphaTransparent : kAlphaOpaque);

            // 終了処理
            if (damageReactTimer_ >= damageReactDuration_) {
                isDamageReact_ = false;
                transform_->quateRotation_ = baseRotation_;
                SetAlpha(kAlphaOpaque);
            }
        }

        // 下方向の速度を制限
        if (velocity_.y < kMaxFallVelocity) {
            velocity_.y = kMaxFallVelocity;
        }

        if (isDashing_) {
            targetFov_ = kDashingFov;
        } else {
            targetFov_ = kNormalFov;
        }

        // 現在のFOVを滑らかに補間してカメラに適用
        currentFov_ += (targetFov_ - currentFov_) * fovLerpSpeed_ * dt_;
        FollowCamera_->SetCameraFov(currentFov_);

        CollisionGround();

        BaseObject::Update();

        shake_->Update();

        auraEmitter_->SetTranslate(GetWorldPosition());
        auraEmitter_->SetRotation(-GetWorldRotation());
        auraEmitter_->Update();

        if (chargeShot_) {
            if (chargeShot_->GetIsCharge()) {
                auraEmitter_->SetAuto(chargeShot_->GetIsCharge());
            } else {
                auraEmitter_->SetAuto(false);
            }
        }

        //UpdateShadowScale();
        if (HP_ <= kMinHP) {
            if (isAlive_) {
                // 死亡した瞬間：回転リセットのイージングを開始
                isDeathRotationReset_ = true;
                deathRotationResetTimer_ = 0.0f;
                deathRotationStart_ = transform_->quateRotation_;

                // ダメージリアクション中なら即座に終了させる
                isDamageReact_ = false;
                SetAlpha(kAlphaOpaque);
            }
            isAlive_ = false;
        }
    }

    if (isPause_) {
        velocity_ = {0.0f, 0.0f, 0.0f};
    } else {
        if (gamePad_->GetLeftTrigger() > 0.25f) {
            isSkillMenu_ = true;
        } else {
            isSkillMenu_ = false;
        }
    }
}

void Player::Draw(const ViewProjection &viewProjection) {
    if (deathStaging_->GetIsStart()) {
        leftHand_ptr_->SetIsAlive(false);
        rightHand_ptr_->SetIsAlive(false);
        return;
    }
    BaseObject::Draw(viewProjection);
    //shadow_->Draw(viewProjection);
    for (auto &bullet : bullets_) {
        bullet->Draw(viewProjection);
    }
    chargeShot_->Draw(viewProjection);
    if (transform_->translation_.y < kGroundLevel) {
        return;
    }
}

void Player::DrawParticle(const ViewProjection &viewProjection) {

    if (!isAlive_ && isDeathStaging_) {
        deathStaging_->Initialize(
            GetWorldPosition(), GetColor(),
            rightHand_ptr_->GetWorldPosition(), rightHand_ptr_->GetColor(),
            leftHand_ptr_->GetWorldPosition(), leftHand_ptr_->GetColor());
        deathStaging_->Update();
        deathStaging_->Draw(viewProjection);
    }

    chargeShot_->DrawParticle(viewProjection);
    auraEmitter_->Draw(viewProjection);
    hitEmitter_->Draw(viewProjection);

    for (auto &bullet : bullets_) {
        bullet->DrawParticle(viewProjection);
    }

    if (attackCollider_) {
        attackCollider_->DrawParticle(viewProjection);
    }

    if (makanAttack_ptr_) {
        makanAttack_ptr_->DrawParticle(viewProjection);
    }

    // 全ステートのパーティクル描画を呼び出す
    for (const auto &[name, state] : states_) {
        if (state) {
            state->DrawParticle(*this, viewProjection);
        }
    }
}

void Player::ChangeState(const std::string &stateName) {
    auto it = states_.find(stateName);
    if (it != states_.end()) {
        previousStateName = GetCurrentStateName();
        if (currentState_) {
            currentState_->Exit(*this);
        }
        currentState_ = it->second.get();
        currentState_->Enter(*this);
    }
}

void Player::OnCollision(ColliderBase *other) {
    // フィールド円柱との押し戻し
    if (other->GetTag() == "CylinderField") {
        if (other->GetType() != ColliderType::Cylinder) {
            return;
        }
        auto *cyl = static_cast<CylinderCollider *>(other);
        Vector3 mtv;
        if (CollisionManager::GetInstance()->CalculateDepenetrationOBBCylinder(playerCollider_, cyl, mtv)) {
            transform_->translation_ += mtv;
            Vector3 mtvDir = mtv.Normalize();
            float dot = velocity_.Dot(mtvDir);
            if (dot < 0.0f) {
                velocity_ -= mtvDir * dot;
            }
        }
        return;
    }

    // PlayerWall との押し戻し（AABB）
    if (other->GetType() != ColliderType::AABB) {
        return;
    }
    auto *otherAABB = static_cast<AABBCollider *>(other);
    Vector3 mtv;
    if (CollisionManager::GetInstance()->CalculateDepenetration(playerWallCollider_, otherAABB, mtv)) {
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

void Player::OnCollisionEnter(ColliderBase *other) {
    if (other->GetTag() == "EnemyBullet") {
        hitEmitter_->SetPosition(transform_->translation_);
        hitEmitter_->UpdateOnce();
    }
}

void Player::DirectionUpdate() {
    if (!gamePad_->IsConnected()) {
        // キーボード入力
        if (input_->PushKey(DIK_D)) {
            moveDir_ = MoveDirection::Right;
        } else if (input_->PushKey(DIK_A)) {
            moveDir_ = MoveDirection::Left;
        } else if (input_->PushKey(DIK_W)) {
            moveDir_ = MoveDirection::Forward;
        } else if (input_->PushKey(DIK_S)) {
            moveDir_ = MoveDirection::Behind;
        }
    } else {
        // ゲームパッド入力 - 左スティック
        float xInput = -gamePad_->GetLeftStickX(); // 左スティックX軸
        float zInput = gamePad_->GetLeftStickY();  // 左スティックY軸

        // スティック入力から方向を判定
        if (xInput != 0.0f || zInput != 0.0f) {
            float angle = std::atan2(xInput, zInput);

            const float PI = std::numbers::pi_v<float>;
            const float segment = PI / 4.0f; // 45度

            if (angle >= -segment && angle < segment) {
                moveDir_ = MoveDirection::Forward;
            } else if (angle >= segment && angle < segment * 3.0f) {
                moveDir_ = MoveDirection::Right;
            } else if (angle >= segment * 3.0f || angle < -segment * 3.0f) {
                moveDir_ = MoveDirection::Behind;
            } else if (angle >= -segment * 3.0f && angle < -segment) {
                moveDir_ = MoveDirection::Left;
            }
        }
    }

    // 向いてる方向は回転値から計算(ロックオン時以外)
    if (!isLockOn_) {
        dir_ = CalculateDirectionFromRotation();
    } else {
        dir_ = Direction::Forward;
    }
}

void Player::Shot() {
    if (currentState_ != states_["EnergyCharge"].get() && !isSkillMenu_) {
        if (!gamePad_->IsConnected()) {
            // キーボード入力
            if (input_->TriggerKey(DIK_J)) {
                if (!ConsumeEnergy(kNormalShotEnergyCost)) {
                    return; // エネルギー不足なら発射しない
                }

                std::string bulletName = "PlayerBullet_" + std::to_string(bullets_.size());
                auto bullet = std::make_unique<PlayerBullet>();
                bullet->Init(bulletName);
                bullet->InitTransform(this);
                bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
                bullet->SetColliderRadius(kBulletColliderRadius);
                bullets_.push_back(std::move(bullet));

                timeSinceLastShot_ = kTimerReset; // 射撃タイマーをリセット
            }
        } else {
            // ゲームパッド入力 - Yボタンの押下時間を計測
            if (gamePad_->IsPress(XINPUT_GAMEPAD_Y) && !isSkillMenu_) {
                yButtonHoldTime_ += dt_;
            }

            // Yボタンが離された瞬間、長押し判定閾値未満なら通常弾を発射
            if (gamePad_->IsRelease(XINPUT_GAMEPAD_Y) && !isSkillMenu_) {
                if (yButtonHoldTime_ < kYButtonChargeThreshold) {
                    if (!ConsumeEnergy(kNormalShotEnergyCost)) {
                        yButtonHoldTime_ = 0.0f;
                        return; // エネルギー不足なら発射しない
                    }

                    std::string bulletName = "PlayerBullet_" + std::to_string(bullets_.size());
                    auto bullet = std::make_unique<PlayerBullet>();
                    bullet->Init(bulletName);
                    bullet->InitTransform(this);
                    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
                    bullet->SetColliderRadius(kBulletColliderRadius);
                    bullets_.push_back(std::move(bullet));

                    timeSinceLastShot_ = kTimerReset; // 射撃タイマーをリセット
                }

                yButtonHoldTime_ = 0.0f; // 押下時間をリセット
            }

            // Yボタンが押されていない時はタイマーをリセット
            if (!gamePad_->IsPress(XINPUT_GAMEPAD_Y)) {
                yButtonHoldTime_ = 0.0f;
            }
        }
    }

    // 弾の更新と生存チェック
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        (*it)->Update();
        (*it)->SetSpeed(B_speed_);
        (*it)->SetAcce(B_acce_);
        (*it)->UpdateWorldTransformHierarchy();

        if (!(*it)->IsAlive()) {
            it = bullets_.erase(it);
        } else {
            ++it;
        }
    }
}

void Player::SkillShot() {
    if (!chargeShot_->GetIsCharge()) {
        if (!gamePad_->IsConnected()) {
            // キーボード入力
            if (input_->TriggerKey(DIK_G)) {
                if (!makanAttack_ptr_ || makanAttack_ptr_->IsActive()) {
                    return; // 既に発動中なら何もしない
                }
                if (!ConsumeEnergy(kSkillShotEnergyCost)) {
                    return; // エネルギー不足なら発射しない
                }
                makanAttack_ptr_->SetPlayer(this);
                makanAttack_ptr_->Activate(transform_.get());
            }
        } else {
            // ゲームパッド入力
            if (isSkillMenu_) {
                if (gamePad_->IsTrigger(XINPUT_GAMEPAD_Y)) {
                    if (!makanAttack_ptr_ || makanAttack_ptr_->IsActive()) {
                        return; // 既に発動中なら何もしない
                    }
                    if (!ConsumeEnergy(kSkillShotEnergyCost)) {
                        return; // エネルギー不足なら発射しない
                    }
                    makanAttack_ptr_->SetPlayer(this);
                    makanAttack_ptr_->Activate(transform_.get());
                }
            }
        }
    }
}

void Player::RotateUpdate() {
    if (isLockOn_ && enemy_) {
        Vector3 toEnemy = enemy_->GetWorldPosition() - GetWorldPosition();
        if (toEnemy.Length() > kMinRotationDistance) {
            toEnemy = toEnemy.Normalize();

            // プレイヤーの正面方向(+Z方向)を敵の方向に向ける
            Vector3 forward = toEnemy;
            Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ};

            // forwardとworldUpが平行になる場合の対処
            Vector3 right;
            if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
                right = {kRightVectorX, kRightVectorY, kRightVectorZ};
            } else {
                right = (worldUp.Cross(forward)).Normalize();
            }

            Vector3 up = (forward.Cross(right)).Normalize();

            // 回転行列から目標クォータニオンを作成
            Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
            Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

            float rotateSpeed = kPlayerRotationSpeed;
            transform_->quateRotation_ = Quaternion::Slerp(transform_->quateRotation_, targetRot, rotateSpeed * dt_);
        }
    } else {
        if (!gamePad_->IsConnected()) {
            // キーボード入力（左右矢印キーで手動回転）
            Vector3 euler = transform_->quateRotation_.ToEulerAngles();
            bool rotationChanged = false;
            if (input_->PushKey(DIK_RIGHT)) {
                euler.y -= kManualRotationSpeed;
                rotationChanged = true;
            }
            if (input_->PushKey(DIK_LEFT)) {
                euler.y += kManualRotationSpeed;
                rotationChanged = true;
            }
            if (rotationChanged) {
                transform_->quateRotation_ = Quaternion::FromEulerAngles(euler);
            }
        } else {
            // ゲームパッド入力（右スティックX軸で左右回転）
            float rightStickX = gamePad_->GetRightStickX();
            float rightStickY = gamePad_->GetRightStickY();

            if (rightStickX != 0.0f || rightStickY != 0.0f) {
                Vector3 euler = transform_->quateRotation_.ToEulerAngles();
                euler.y += rightStickX * kManualRotationSpeed * 2.0f;
                transform_->quateRotation_ = Quaternion::FromEulerAngles(euler);
            }
        }
    }
}

void Player::ComboUpdate() {
    if (currentState_ != states_["EnergyCharge"].get() && !chargeShot_->GetIsCharge()) {
        punchCombo_.Update(Frame::DeltaTime());

        if (!gamePad_->IsConnected()) {
            // キーボード入力
            if (input_->TriggerKey(DIK_H)) {
                punchCombo_.TryExecuteCombo();
            }
        } else {
            // ゲームパッド入力
            if (gamePad_->IsTrigger(XINPUT_GAMEPAD_X)) {
                punchCombo_.TryExecuteCombo();
            }
        }
    }
}

void Player::Move() {
    float xInput = kInputZero;
    float zInput = kInputZero;

    if (!gamePad_->IsConnected()) {
        // キーボード入力
        if (input_->PushKey(DIK_A))
            xInput += kInputValue;
        if (input_->PushKey(DIK_D))
            xInput -= kInputValue;
        if (input_->PushKey(DIK_W))
            zInput += kInputValue;
        if (input_->PushKey(DIK_S))
            zInput -= kInputValue;
        isDashing_ = input_->PushKey(DIK_LCONTROL);
    } else {
        // ゲームパッド入力
        xInput = -gamePad_->GetLeftStickX(); // 左スティックX軸
        zInput = gamePad_->GetLeftStickY();  // 左スティックY軸

        bool isLTHeld = gamePad_->GetLeftTrigger() > 0.25f;

        if (isLTHeld) {
            // LT中にAボタンでダッシュ開始
            if (gamePad_->IsTrigger(XINPUT_GAMEPAD_A) && !isDashing_) {
                dashInputX_ = xInput;
                dashInputZ_ = zInput;
                isDashing_ = true;
                dashStartedThisFrame_ = true;
                dashDuration_ = 0.0f;
            }

            // エネルギー不足の場合はダッシュ中以外は移動させない
            if (energy_ < maxEnergy_ && !isDashing_) {
                xInput = kInputZero;
                zInput = kInputZero;
            }
        } else {
            // LTを離したらダッシュ解除
            isDashing_ = false;
            dashInputX_ = kInputZero;
            dashInputZ_ = kInputZero;
            dashDuration_ = 0.0f;
            dashStartedThisFrame_ = false;
        }
    }

    // 入力がない場合の減速処理
    if (xInput == kInputZero && zInput == kInputZero && !isDashing_) {
        velocity_.x *= kDecelerationFactor;
        velocity_.z *= kDecelerationFactor;
        if (std::abs(velocity_.x) < kVelocityStopThreshold)
            velocity_.x = kVelocityZero;
        if (std::abs(velocity_.z) < kVelocityStopThreshold)
            velocity_.z = kVelocityZero;
        return;
    }

    // カメラの方向ベクトル取得
    FollowCamera *camera = GetCamera();
    if (!camera)
        return;

    float yaw = camera->GetYaw();
    Vector3 cameraForward = {std::sin(yaw), kYComponentZero, std::cos(yaw)};
    Vector3 cameraRight = {-std::cos(yaw), kYComponentZero, std::sin(yaw)};

    Vector3 moveDir = cameraRight * xInput + cameraForward * zInput;

    // ダッシュ開始時のスティック入力がない場合、敵または自機正面方向へ移動
    if (dashStartedThisFrame_ && dashInputX_ == kInputZero && dashInputZ_ == kInputZero) {
        if (enemy_) {
            Vector3 toEnemy = enemy_->GetWorldPosition() - GetWorldPosition();
            toEnemy.y = 0;
            if (toEnemy.Length() > 0.001f) {
                moveDir = toEnemy.Normalize();
            }
        } else {
            moveDir = GetForward();
            moveDir.y = 0;
            moveDir = moveDir.Normalize();
        }
    } else if (moveDir.Length() > 0.001f) {
        moveDir = moveDir.Normalize();
    }

    // ロックオン中でない場合、移動方向に向けて回転
    if (!isLockOn_ && moveDir.Length() > 0.001f) {
        float targetYaw = std::atan2(-moveDir.x, moveDir.z);
        Quaternion targetRot = Quaternion::FromEulerAngles({kRotationZero, targetYaw, kRotationZero});
        float rotateSpeed = kPlayerRotationSpeed;
        transform_->quateRotation_ = Quaternion::Slerp(transform_->quateRotation_, targetRot, rotateSpeed * dt_);
    }

    // ダッシュ中は最大速度を倍増
    float currentMaxSpeed = isDashing_ ? maxSpeed_ * kDashSpeedMultiplier : maxSpeed_;

    velocity_.x += moveDir.x * accelRate_ * dt_;
    velocity_.z += moveDir.z * accelRate_ * dt_;

    // 最高速度制限
    float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
    if (speed > currentMaxSpeed) {
        float scale = currentMaxSpeed / speed;
        velocity_.x *= scale;
        velocity_.z *= scale;
    }
}

void Player::UpdateShadowScale() {
  /*  if (transform_->translation_.y < kGroundLevel) {
        return;
    }
    float height = transform_->translation_.y;
    float baseScale = kShadowBaseScale;
    float scaleFactor = std::max(kShadowMinScale, baseScale - height * kShadowScaleFactor);
    shadow_->GetLocalScale() = {scaleFactor, scaleFactor, scaleFactor};*/
}

bool Player::ConsumeEnergy(float amount) {
    if (energy_ >= amount) {
        energy_ -= amount;
        timeSinceLastShot_ = kTimerReset;
        return true;
    }
    return false;
}

void Player::RecoverEnergy() {
    bool canRecover = false;

    // エナジーチャージ中なら即回復
    if (currentState_ == states_["EnergyCharge"].get()) {
        canRecover = true;
    }
    // EnergyCharge チュートリアルステップ中は自動回復を停止する
    else if (tutorialStep_ != TutorialStep::EnergyCharge) {
        timeSinceLastShot_ += dt_;
        if (timeSinceLastShot_ >= energyRecoveryDelay_) {
            canRecover = true;
        }
    }

    if (canRecover) {
        energy_ += energyRecoveryRate_ * dt_;
        if (energy_ > maxEnergy_) {
            energy_ = maxEnergy_;
        }
    }
}

void Player::SetTutorialStep(TutorialStep step) {
    // EnergyCharge ステップに切り替わった瞬間だけリセット処理を行う
    if (step == TutorialStep::EnergyCharge &&
        tutorialStep_ != TutorialStep::EnergyCharge) {
        energy_ = 0.0f;
        timeSinceLastShot_ = 0.0f; // 回復遅延タイマーもリセット
    }

    tutorialStep_ = step;
}

void Player::DamageUpdate() {
    if (damage_ <= kNoDamage) {
        return;
    }

    // 無敵中はダメージを無視
    if (isInvincible_) {
        damage_ = kNoDamage;
        return;
    }

    // ガード中はダメージ・ノックバックを軽減し、エネルギーを消費する
    float guardMult = isGuarding_ ? guardDamageMultiplier_ : 1.0f;
    if (isGuarding_) {
        ConsumeEnergy(guardEnergyCost_);
    }

    HP_ -= damage_ * guardMult;
    damage_ = kNoDamage;

    if (hasKnockback_) {
        velocity_.x += knockbackVelocity_.x * guardMult;
        velocity_.y += knockbackVelocity_.y * guardMult;
        velocity_.z += knockbackVelocity_.z * guardMult;
        if (isGrounded_ && knockbackVelocity_.y > 0.0f) {
            isGrounded_ = false;
        }
        hasKnockback_ = false;
        knockbackVelocity_ = {0.0f, 0.0f, 0.0f};
    }

    // 無敵時間の開始
    isInvincible_ = true;
    invincibleTime_ = kTimerReset;

    // ダメージリアクションの開始
    isDamageReact_ = true;
    damageReactTimer_ = kTimerReset;

    // 現在の向きを保存してのけぞりイージングをセット
    baseRotation_ = transform_->quateRotation_;
    float startAngle = kRotationZero;
    float endAngle = degreesToRadians(kPlayerDamageTiltDegrees);
    tiltEase_.Reset(startAngle, endAngle, damageReactDuration_, EasingType::OutQuad);
}

void Player::InvincibleUpdate() {
    invincibleTime_ += dt_;
    if (invincibleTime_ >= invincibleDuration_) {
        isInvincible_ = false;
        invincibleTime_ = kTimerReset;
    }
}

void Player::CollisionGround() {
    // 位置更新前に次フレームのY座標を計算
    float nextY = GetLocalPosition().y + velocity_.y * dt_;

    GetLocalPosition().x += velocity_.x * dt_;
    GetLocalPosition().z += velocity_.z * dt_;

    if (nextY <= kGroundLevel) {
        // Rush状態の場合は地面から押し戻す（地面に埋まらないよう浮かせる）
        if (currentState_ == states_["Rush"].get()) {
            GetLocalPosition().y = kRushGroundOffset;
            velocity_.y = kVelocityZero;
            return;
        }

        // 地面に接地（Rush 以外）
        GetLocalPosition().y = kGroundLevel;
        if (!isGrounded_) {
            velocity_.y = kVelocityZero;
            isGrounded_ = true;
            // 空中からの着地で水平速度に応じて状態遷移
            if (currentState_ == states_["Air"].get()) {
                float horizontalSpeed = sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
                if (horizontalSpeed > kLandingSpeedThreshold) {
                    ChangeState("Move");
                } else {
                    ChangeState("Idle");
                }
            }
        }
    } else {
        GetLocalPosition().y = nextY;
        isGrounded_ = false;
    }
}

Direction Player::CalculateDirectionFromRotation() {
    // クォータニオンからオイラー角（Yaw）を取得し、8方向に分類
    float yaw = transform_->quateRotation_.ToEulerAngles().y;
    float angle = NormalizeAngle(yaw);

    if (angle >= 7.0f * std::numbers::pi_v<float> / 4.0f || angle < std::numbers::pi_v<float> / 4.0f) {
        return Direction::Forward;
    } else if (angle >= std::numbers::pi_v<float> / 4.0f && angle < 2.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::ForwardRight;
    } else if (angle >= 2.0f * std::numbers::pi_v<float> / 4.0f && angle < 3.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::Right;
    } else if (angle >= 3.0f * std::numbers::pi_v<float> / 4.0f && angle < 4.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::BackwardRight;
    } else if (angle >= 4.0f * std::numbers::pi_v<float> / 4.0f && angle < 5.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::Behind;
    } else if (angle >= 5.0f * std::numbers::pi_v<float> / 4.0f && angle < 6.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::BackwardLeft;
    } else if (angle >= 6.0f * std::numbers::pi_v<float> / 4.0f && angle < 7.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::Left;
    } else if (angle >= 7.0f * std::numbers::pi_v<float> / 4.0f && angle < 8.0f * std::numbers::pi_v<float> / 4.0f) {
        return Direction::ForwardLeft;
    }
    return Direction::Forward;
}

const char *Player::GetDirectionName(Direction dir) {
    switch (dir) {
    case Direction::Forward:
        return "前";
    case Direction::ForwardRight:
        return "右前";
    case Direction::Right:
        return "右";
    case Direction::BackwardRight:
        return "右後ろ";
    case Direction::Behind:
        return "後ろ";
    case Direction::BackwardLeft:
        return "左後ろ";
    case Direction::Left:
        return "左";
    case Direction::ForwardLeft:
        return "左前";
    default:
        return "不明";
    }
}

float Player::NormalizeAngle(float angle) {
    const float TWO_PI = 2.0f * std::numbers::pi_v<float>;
    while (angle < 0.0f)
        angle += TWO_PI;
    while (angle >= TWO_PI)
        angle -= TWO_PI;
    return angle;
}

float Player::CalculateShortestRotation(float from, float to) {
    float diff = to - from;
    const float PI = std::numbers::pi_v<float>;

    while (diff > PI)
        diff -= 2.0f * PI;
    while (diff < -PI)
        diff += 2.0f * PI;

    return diff;
}

void Player::Debug() {
#ifdef USE_IMGUI
    const char *currentStateName = "Unknown";
    for (const auto &[named, state] : states_) {
        if (state.get() == currentState_) {
            currentStateName = named.c_str();
            break;
        }
    }
    if (ImGui::BeginTabBar("プレイヤー")) {
        if (ImGui::BeginTabItem("プレイヤー")) {

            ImGui::Text("エネルギー: %.1f / %.1f", energy_, maxEnergy_);
            ImGui::DragFloat("エネルギー回復速度", &energyRecoveryRate_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("回復開始遅延", &energyRecoveryDelay_, 0.1f, 0.0f, 5.0f);

            ImGui::Text("無敵状態: %s", isInvincible_ ? "True" : "False");
            ImGui::Text("ダッシュ始めたフレームかどうか: %s", dashStartedThisFrame_ ? "True" : "False");
            ImGui::Text("ダッシュ時間: %f", dashDuration_);

            if (isInvincible_) {
                ImGui::Text("無敵残り時間: %.2f秒", invincibleDuration_ - invincibleTime_);
            }
            ImGui::DragFloat("無敵時間", &invincibleDuration_, 0.01f, 0.0f, 2.0f);

            ImGui::Text("Current State: %s", currentStateName);
            ImGui::Text("IsGrounded: %s", isGrounded_ ? "True" : "False");
            ImGui::Text("IsLockOn: %s", isLockOn_ ? "True" : "False");
            ImGui::Text("向いている方向: %s", GetDirectionName(dir_));
            ImGui::DragFloat("ジャンプ速度", &jumpSpeed_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("落下速度", &fallSpeed_, 0.1f, -20.0f, 0.0f);
            ImGui::DragFloat("現在速度", &moveSpeed_, 0.1f, 0.0f, maxSpeed_);
            ImGui::DragFloat("最大速度", &maxSpeed_, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("加速率", &accelRate_, 0.1f, 0.0f, 50.0f);
            ImGui::Text("現在位置: X=%.2f, Y=%.2f, Z=%.2f",
                        GetLocalPosition().x, GetLocalPosition().y, GetLocalPosition().z);
            ImGui::Text("現在速度: X=%.2f, Y=%.2f, Z=%.2f",
                        velocity_.x, velocity_.y, velocity_.z);
            ImGui::Text("現在角度(クォータニオン): X=%.2f, Y=%.2f, Z=%.2f, W=%.2f",
                        GetLocalRotation().x, GetLocalRotation().y, GetLocalRotation().z, GetLocalRotation().w);
            ImGui::Text("現在角度(オイラー): X=%.2f, Y=%.2f, Z=%.2f",
                        GetLocalRotation().ToEulerAngles().x, GetLocalRotation().ToEulerAngles().y, GetLocalRotation().ToEulerAngles().z);

            ImGui::DragFloat("弾の速度", &B_speed_, 0.1f);
            ImGui::DragFloat("弾の加速度", &B_acce_, 0.1f);

            ImGui::Separator();
            ImGui::Text("ガード設定");
            ImGui::Text("  現在状態: %s", isGuarding_ ? "ガード中" : "解除中");
            // 0.0=完全無敵, 1.0=ガード意味なし。軽減率 = (1 - multiplier)*100 %
            ImGui::DragFloat("ガード被ダメ倍率 (0=無敵, 1=無効)", &guardDamageMultiplier_, 0.01f, 0.0f, 1.0f);
            ImGui::Text("  -> 軽減率 %.0f%%", (1.0f - guardDamageMultiplier_) * 100.0f);
            ImGui::DragFloat("ガード時エネルギー消費", &guardEnergyCost_, 0.5f, 0.0f, 100.0f);

            if (ImGui::Button("セーブ")) {
                Save();
            }

            if (ImGui::TreeNode("操作説明")) {
                ImGui::Text("WASD : 移動");
                if (currentState_ != states_["FlyMove"].get() || currentState_ != states_["FlyIdle"].get()) {
                    ImGui::Text("SPACE : ジャンプ");
                    ImGui::Text("空中でSPACE : 浮遊");
                } else {
                    ImGui::Text("SPACE : 上昇");
                    ImGui::Text("LSHIFT : 下降");
                    ImGui::Text("LSHIFT2回押し : 落下");
                    ImGui::Text("Ctrl : ダッシュ");
                }
                ImGui::Text("J : 射撃");
                ImGui::Text("K長押し : チャージショット");
                ImGui::Text("L : ロックオン");

                ImGui::TreePop();
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    shake_->imgui();
    if (!isAlive_) {
        deathStaging_->imgui();
    }

    makanAttack_ptr_->DebugImGui();

    if (ImGui::CollapsingHeader("コンボパラメータ")) {
        punchCombo_.DrawImGui();
    }

    // ─── コンボアニメーション割り当て ───
    if (ImGui::CollapsingHeader("コンボアニメーション割り当て")) {
        static const char *kComboLabels[] = {
            "1段目: Jab",
            "2段目: Hook",
            "3段目: Cross",
            "4段目: Uppercut",
            "5段目: Overhand",
            "6段目: Swing",
            "7段目: Elbow",
            "8段目: Slam",
        };

        ImGui::TextDisabled("空白のままにすると、その段はアニメーションを変更しません");
        ImGui::Spacing();

        for (int i = 0; i < static_cast<int>(comboAnimations_.size()); ++i) {
            ImGui::PushID(i);

            // 現在実行中の段をハイライト
            bool isCurrentStage = punchCombo_.IsComboActive() &&
                                  ((punchCombo_.GetCurrentComboIndex() == 0
                                        ? punchCombo_.GetComboLength() - 1
                                        : punchCombo_.GetCurrentComboIndex() - 1) == i);
            if (isCurrentStage) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), ">>> %s", kComboLabels[i]);
            } else {
                ImGui::Text("%s", kComboLabels[i]);
            }

            char buf[256] = {};
            snprintf(buf, sizeof(buf), "%s", comboAnimations_[i].c_str());
            ImGui::SetNextItemWidth(380.0f);
            if (ImGui::InputText("##animPath", buf, sizeof(buf))) {
                comboAnimations_[i] = buf;
            }

            ImGui::PopID();
            ImGui::Spacing();
        }
    }

#endif // USE_IMGUI
}

void Player::ChangeEnergyCharge() {
    // EnergyCharge ステート内からは呼ばれないため、二重遷移防止のみチェック
    if (currentState_ == states_["EnergyCharge"].get()) {
        return;
    }
    if (energy_ >= maxEnergy_) {
        return;
    }

    if (!gamePad_->IsConnected()) {
        // キーボード入力
        if (input_->TriggerKey(DIK_C)) {
            ChangeState("EnergyCharge");
        }
    } else {
        // ゲームパッド入力 - LT のエッジ検出でチャージ状態に遷移
        bool isRTPressed = gamePad_->GetLeftTrigger() > 0.25f;

        if (isRTPressed && !wasRTPressed_ &&
            !gamePad_->IsPress(XINPUT_GAMEPAD_Y) &&
            !gamePad_->IsPress(XINPUT_GAMEPAD_A)) {
            ChangeState("EnergyCharge");
        }

        wasRTPressed_ = isRTPressed;
    }
}

void Player::UpdateDashState() {
    if (!gamePad_->IsConnected()) {
        return; // キーボードの場合はダッシュ継続時間を管理しない
    }

    bool isLTHeld = gamePad_->GetLeftTrigger() > 0.25f;

    if (isLTHeld && isDashing_) {
        dashDuration_ += dt_; // ダッシュ継続時間を更新
    }

    // dashStartedThisFrame_ は1フレームだけ true になるフラグ
    // wasDashing_ で前フレームのダッシュ状態を保持し、次フレームでリセットする
    if (dashStartedThisFrame_ && wasDashing_) {
        dashStartedThisFrame_ = false;
    }

    wasDashing_ = isDashing_;
}

void Player::UpdateGuardInput() {
    // すでにガード中なら遷移判定は Guard ステート側に任せる
    if (currentState_ == states_["Guard"].get()) {
        return;
    }

    // スキル発動中・スキルメニュー中はガード不可
    if (isSkillMenu_ || (makanAttack_ptr_ && makanAttack_ptr_->IsActive())) {
        return;
    }

    // 地上(Idle/Move)・飛行(FlyIdle/FlyMove)からのみガードへ入れる
    bool canGuard =
        currentState_ == states_["Idle"].get() ||
        currentState_ == states_["Move"].get() ||
        currentState_ == states_["FlyIdle"].get() ||
        currentState_ == states_["FlyMove"].get();
    if (!canGuard) {
        return;
    }

    bool guardHeld = gamePad_->IsPress(XINPUT_GAMEPAD_B) || input_->PushKey(DIK_RSHIFT);
    if (guardHeld) {
        ChangeState("Guard");
    }
}

void Player::UpdateAnimation() {
    // ──────────────────────────────────────────
    // コンボ攻撃中：段数に対応したアニメーションを再生
    // ──────────────────────────────────────────
    if (punchCombo_.IsComboActive()) {
        // GetCurrentComboIndex() は「次に実行する」インデックスを返す。
        // ExecuteComboAttack() が呼ばれるとインクリメントされるため、
        // 現在再生中の段 = nextIdx - 1（0 のときは最終段 Slam の後待機中）
        int nextIdx = punchCombo_.GetCurrentComboIndex();
        int comboLen = punchCombo_.GetComboLength();
        int animIdx = (nextIdx == 0) ? (comboLen - 1) : (nextIdx - 1);

        if (animIdx >= 0 && animIdx < static_cast<int>(comboAnimations_.size())) {
            const std::string &path = comboAnimations_[animIdx];
            if (!path.empty()) {
                SetAnima(path);
            }
            // else: 該当段のアニメーションが未設定（空文字）なので何もしない
            // SetAnima(""); ← パスなし段はここでコメントアウト
        }
        return; // 攻撃中は以降のステート判定をスキップ
    }

    // ──────────────────────────────────────────
    // 通常ステート：ステート名に応じてアニメーションを切り替え
    // ──────────────────────────────────────────
    const std::string stateName = GetCurrentStateName();

    if (stateName == "Idle") {
        // 地上待機
        SetAnima("animation/Player/Idle_Ground.gltf");

    } else if (stateName == "Move") {
        // 地上移動
        SetAnima("animation/Run.gltf");

    } else if (stateName == "Jump") {
        // ジャンプ直後（上昇）
        SetAnima("animation/Jump.gltf");

    } else if (stateName == "Air") {
        // 空中（下降・滞空）― 専用モーションがないためジャンプで代用
        SetAnima("animation/Jump.gltf");
        // SetAnima("animation/Player/Air.gltf"); // 専用モーションがあれば差し替え

    } else if (stateName == "FlyIdle") {
        // 浮遊待機
        SetAnima("animation/Player/Idle_Flying.gltf");

    } else if (stateName == "FlyMove") {
        // 浮遊移動 ― 専用モーションがないため Idle_Flying で代用
        SetAnima("animation/Player/Idle_Flying.gltf");
        // SetAnima("animation/Player/Fly_Move.gltf"); // 専用モーションがあれば差し替え

    } else if (stateName == "Rush") {
        // ダッシュ突進 ― 専用モーションがないため Run で代用
        SetAnima("animation/Run.gltf");
        // SetAnima("animation/Player/Rush.gltf"); // 専用モーションがあれば差し替え

    } else if (stateName == "EnergyCharge") {
        // エネルギーチャージ ― 専用モーションなし（現状は何もしない）
        // SetAnima("animation/Player/EnergyCharge.gltf"); // 専用モーションがあれば差し替え
    }
}

Vector3 Player::GetMovementDirection() const {
    Vector3 dir = velocity_;
    float len = GetVelocityMagnitude();

    if (len > 0.001f) {
        dir.x /= len;
        dir.y /= len;
        dir.z /= len;
    } else {
        dir = {0.0f, 0.0f, 0.0f};
    }

    return dir;
}

void Player::ReleaseLockOn() {
    isLockOn_ = false;
}

float Player::GetVelocityMagnitude() const {
    return std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
}

std::string Player::GetCurrentStateName() const {
    for (const auto &pair : states_) {
        if (pair.second.get() == currentState_) {
            return pair.first;
        }
    }
    return "Unknown";
}

void Player::Save() {
    data_->Save("fallSpeed", fallSpeed_);
    data_->Save("moveSpeed", moveSpeed_);
    data_->Save("jumpSpeed", jumpSpeed_);
    data_->Save("maxSpeed", maxSpeed_);
    data_->Save("accelRate", accelRate_);
    data_->Save("bulletSpeed", B_speed_);
    data_->Save("bulletAcce", B_acce_);
    data_->Save("maxEnergy", maxEnergy_);
    data_->Save("energyRecoveryRate", energyRecoveryRate_);
    data_->Save("energyRecoveryDelay", energyRecoveryDelay_);
    data_->Save("invincibleDuration", invincibleDuration_);
    data_->Save("guardDamageMultiplier", guardDamageMultiplier_);
    data_->Save("guardEnergyCost", guardEnergyCost_);
    ImGuiNotification::Post("プレイヤー設定を保存しました", {0.2f, 0.8f, 0.2f, 1.0f});
}

void Player::Load() {
    fallSpeed_ = data_->Load<float>("fallSpeed", -9.8f);
    moveSpeed_ = data_->Load<float>("moveSpeed", 0.0f);
    jumpSpeed_ = data_->Load<float>("jumpSpeed", 10.0f);
    maxSpeed_ = data_->Load<float>("maxSpeed", 10.0f);
    accelRate_ = data_->Load<float>("accelRate", 15.0f);
    B_speed_ = data_->Load<float>("bulletSpeed", 60.0f);
    B_acce_ = data_->Load<float>("bulletAcce", 5.0f);
    maxEnergy_ = data_->Load<float>("maxEnergy", 100.0f);
    energyRecoveryRate_ = data_->Load<float>("energyRecoveryRate", 0.01f);
    energyRecoveryDelay_ = data_->Load<float>("energyRecoveryDelay", 1.0f);
    energy_ = maxEnergy_; // 初期化時は最大値
    invincibleDuration_ = data_->Load<float>("invincibleDuration", 0.25f);
    guardDamageMultiplier_ = data_->Load<float>("guardDamageMultiplier", 0.20f);
    guardEnergyCost_ = data_->Load<float>("guardEnergyCost", 10.0f);
    ImGuiNotification::Post("プレイヤー設定を読み込みました", {0.2f, 0.8f, 0.8f, 1.0f});
}

Vector3 Player::GetForward() const { return -GetBackward(); }

Vector3 Player::GetBackward() const {
    // クォータニオンから前方向ベクトルを計算（Z軸の負方向が前方向）
    return TransformNormal(Vector3(kForwardVectorX, kForwardVectorY, kForwardVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Player::GetRight() const {
    // クォータニオンから右方向ベクトルを計算（X軸の正方向が右方向）
    return TransformNormal(Vector3(kRightVectorX, kRightVectorY, kRightVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Player::GetLeft() const { return -GetRight(); }

Vector3 Player::GetUp() const {
    // クォータニオンから上方向ベクトルを計算（Y軸の正方向が上方向）
    return TransformNormal(Vector3(kUpVectorX, kUpVectorY, kUpVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Player::GetDown() const { return -GetUp(); }

Vector3 Player::GetPositionBehind(float distance) const { return transform_->translation_ + GetBackward() * distance; }
Vector3 Player::GetPositionFront(float distance) const { return transform_->translation_ + GetForward() * distance; }
Vector3 Player::GetPositionRight(float distance) const { return transform_->translation_ + GetRight() * distance; }
Vector3 Player::GetPositionLeft(float distance) const { return transform_->translation_ + GetLeft() * distance; }
Vector3 Player::GetPositionAbove(float distance) const { return transform_->translation_ + GetUp() * distance; }
Vector3 Player::GetPositionBelow(float distance) const { return transform_->translation_ + GetDown() * distance; }

ViewProjection &Player::GetViewProjection() { return *vp_; }

void Player::SetCamera(FollowCamera *camera) { FollowCamera_ = camera; }

void Player::SetVp(ViewProjection *vp) {
    vp_ = vp;
    shake_->Initialize(vp_);
}

void Player::SetPause(bool flag) {
    isPause_ = flag;
}

void Player::SetKnockback(const Vector3 &direction, float power) {
    if (power <= 0.0f)
        return;

    Vector3 dir = direction;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.001f)
        return;

    dir.x /= len;
    dir.y /= len;
    dir.z /= len;

    // 水平方向 + 少し上方に浮かせる
    knockbackVelocity_ = {
        dir.x * power,
        power * 0.25f,
        dir.z * power,
    };
    hasKnockback_ = true;
}