#define NOMINMAX
#include "Player.h"
#include "Frame/Frame.h"
#include "Utility/Debug/ImGui/ImGuiNotification.h"
#include "State/Air/PlayerStateAir.h"
#include "State/Fly/PlayerStateFlyIdle.h"

#include "Bullet/ChargeShot/ChargeShot.h"
#include "Collider/CollisionManager.h"
#include "3d/Line/DrawLine3D.h"
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
#include "Edit/MotionEditor/MotionEditor.h"
#include <Particle/CSParticle/ParticleCSEditor.h>
#include <Particle/ParticleEditor.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;
Player::Player() {
}

Player::~Player() {
}

void Player::Init(const std::string objectName) {
    BaseObject::Init(objectName);

    // ベースモデル（地上待機）を生成する。各 gltf はメッシュ＋スケルトン＋
    // アニメーションを内包しており、スケルトンを共有するクリップを切り替えて再生する
    BaseObject::CreateModel("animation/Player/Idle_Ground.gltf");
    BaseObject::SetOffset({0.0f, -0.75f, 0.0f}); // 描画オフセット（地面に足がつくように）
    BaseObject::GetLocalScale() = {4.0f, 4.0f, 4.0f};
    BaseObject::SetAnimationSpeed(1.0f);
    BaseObject::SetAnimationBlendDuration(0.2f);

    // 体色を青系にする。モデル素体（マテリアル色）は赤なので、敵は赤のまま、
    // プレイヤーだけここで青へ上書きする（シェーダは material.color * texture）
    for (int i = 0; i < GetObject3d()->GetMaterialCount(); ++i) {
        BaseObject::SetColor({0.15f, 0.35f, 1.0f, 1.0f}, i);
    }

    // ───────────────────────────────────────────
    // アニメーションコントローラへ全クリップを登録する
    // RegisterClip(識別名, ファイルパス, ループ, 速度, 補間時間)
    // ───────────────────────────────────────────
    animationController_.Initialize(GetObject3d());

    // ループあり：待機・移動など継続する動作
    animationController_.RegisterClip("Idle", "animation/Player/Idle_Ground.gltf", true);
    animationController_.RegisterClip("FlyIdle", "animation/Player/Idle_Flying.gltf", true);
    animationController_.RegisterClip("Run", "animation/Player/Running.gltf", true);
    animationController_.RegisterClip("RunBack", "animation/Player/Running_Back.gltf", true);
    animationController_.RegisterClip("RunLeft", "animation/Player/Running_Left.gltf", true);
    animationController_.RegisterClip("RunRight", "animation/Player/Running_Right.gltf", true);
    animationController_.RegisterClip("FlyMove", "animation/Player/Running_Fly.gltf", true);
    animationController_.RegisterClip("Guard", "animation/Player/Block_Idle.gltf", true);

    // ループなし：攻撃・ジャンプなど一回きりの動作
    animationController_.RegisterClip("Jump", "animation/Player/Running_Jamp.gltf", false);
    animationController_.RegisterClip("GuardHit", "animation/Player/Block_Hit.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_1", "animation/Player/Punch_1.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_2", "animation/Player/Punch_2.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_3", "animation/Player/Punch_3.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_4", "animation/Player/Punch_4.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_1", "animation/Player/Kick_1.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_2", "animation/Player/Kick_2.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_3", "animation/Player/Kick_3.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Smash", "animation/Player/Smash.gltf", false, 1.0f, 0.1f);

    // JSONに保存済みの調整値があれば読み込む
    animationController_.LoadClips("AnimationController", "PlayerClips");

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

    chargeShot_ = std::make_unique<ChargeShot>();
    chargeShot_->SetPlayer(this);
    chargeShot_->Init("chageShot");

    makanAttack_ = std::make_unique<MakanAttackSkill>();
    makanAttack_->Init("makanAttack");
    makanAttack_ptr_ = makanAttack_.get();

    BaseObjectManager::GetInstance()->AddObject(std::move(makanAttack_));

    Load();

    punchCombo_.SetName("PunchCombo"); // DataHandlerのファイル名に使われる
    // 攻撃の見た目は本体アニメーション（comboAnimations_）で再生するため、
    // モーション再生用ターゲットは不要（nullptr）。ダメージ等のパラメータのみ指定する
    punchCombo_
        .Add(nullptr, "Jab", 10.0f, 3.0f, 0.25f, 0.08f)
        .Add(nullptr, "Hook", 12.0f, 4.0f, 0.25f, 0.08f)
        .Add(nullptr, "Cross", 12.0f, 4.0f, 0.25f, 0.08f)
        .Add(nullptr, "Uppercut", 15.0f, 6.0f, 0.30f, 0.10f)
        .Add(nullptr, "Overhand", 15.0f, 6.0f, 0.30f, 0.10f)
        .Add(nullptr, "Swing", 18.0f, 7.0f, 0.30f, 0.10f)
        .Add(nullptr, "Elbow", 20.0f, 8.0f, 0.25f, 0.06f)
        .Add(nullptr, "Slam", 25.0f, 12.0f, 0.35f, 0.12f);

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
        "animation/Player/Kick_3.gltf",  // 7段目: Elbow
        "animation/Player/Smash.gltf",   // 8段目: Slam
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
            // 入力表示UI用: 実際に発火した近接攻撃の段名を記録する
            // （先行入力バッファ経由の発火もここを通るため取りこぼしがない）
            meleeAttackFired_ = true;
            lastMeleeAttackName_ = punchCombo_.GetCurrentAttackName();
        });
}

void Player::Update() {

    // 入力表示UI用アクションイベントは毎フレーム先頭でクリアする
    actionEvents_.clear();

    if (activeDebugCamrera_)
        return;

    dt_ = Frame::DeltaTime();

    if (makanAttack_ptr_->IsActive()) {
        return;
    }

    if (!isAlive_) {
        // 死亡時は本体の回転処理なし（死亡演出は DrawParticle 側で描画する）
    } else {
        generatedField_->data.position = GetWorldPosition();
        gamePad_->Update();

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

            UpdateAnimation(); // ステート・コンボに応じてアニメーションを切り替え

            // RotateUpdate はロックオン追従と手動スティック回転を行う
            // Rush ステートは自前で UpdateRotation() を持つため除外する
            if (currentState_ != states_["Rush"].get()) {
                RotateUpdate();
            }

            // 飛行移動中の体の傾き（描画専用）を更新する。RotateUpdate 後の向きを基準にする
            UpdateFlyLean();

            if (chargeShot_) {
                chargeShot_->SetIsSkillMenu(isSkillMenu_);
                chargeShot_->Update();

                // 入力表示UI用：チャージ開始（溜め始め）とチャージ弾発射を通知
                bool nowCharge = chargeShot_->GetIsCharge();
                if (nowCharge && !prevChargeState_) {
                    EmitAction(ActionKind::ChargeStart);
                }
                prevChargeState_ = nowCharge;
                if (chargeShot_->ConsumeFired()) {
                    EmitAction(ActionKind::ChargeShot);
                }
            }

            Shot();

            SkillShot();
        }

        // ダメージリアクション処理（高速点滅のみ。傾き(のけぞり)演出は廃止）
        if (isDamageReact_) {
            damageReactTimer_ += dt_;

            // 高速点滅
            float blinkInterval = kPlayerBlinkInterval;
            int blink = static_cast<int>(damageReactTimer_ / blinkInterval);
            SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kPlayerAlphaTransparent : kAlphaOpaque);

            // 終了処理
            if (damageReactTimer_ >= damageReactDuration_) {
                isDamageReact_ = false;
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

        auraEmitter_->SetTranslate({GetWorldPosition().x, GetWorldPosition().y + auraEmitter_->GetScale().y, GetWorldPosition().z});
        auraEmitter_->SetRotation(-GetWorldRotation());
        auraEmitter_->Update();

        if (chargeShot_) {
            if (chargeShot_->GetIsCharge()) {
                auraEmitter_->SetAuto(chargeShot_->GetIsCharge());
            } else {
                auraEmitter_->SetAuto(false);
            }
        }

        if (HP_ <= kMinHP) {
            if (isAlive_) {
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
        return;
    }
    BaseObject::Draw(viewProjection);
    for (auto &bullet : bullets_) {
        bullet->Draw(viewProjection);
    }
    chargeShot_->Draw(viewProjection);
    if (transform_->translation_.y < kGroundLevel) {
        return;
    }
}

void Player::DrawParticleCompute(const ViewProjection &viewProjection) {
    auraEmitter_->DrawCompute(viewProjection);
    chargeShot_->DrawParticleCompute(viewProjection);
}

void Player::DrawParticle(const ViewProjection &viewProjection) {

    if (!isAlive_ && isDeathStaging_) {
        // 手クラス廃止に伴い、両腕の発生位置は体の左右オフセットで近似する
        deathStaging_->Initialize(
            GetWorldPosition(), GetColor(),
            GetPositionRight(1.0f), GetColor(),
            GetPositionLeft(1.0f), GetColor());
        deathStaging_->Update();
        deathStaging_->Draw(viewProjection);
    }

    chargeShot_->DrawParticle(viewProjection);
    auraEmitter_->DrawGraphics(viewProjection);
    hitEmitter_->Draw(viewProjection); // CPU emitter

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
        previousStateName_ = GetCurrentStateName();
        if (currentState_) {
            currentState_->Exit(*this);
        }
        currentState_ = it->second.get();
        currentState_->Enter(*this);

        // 入力表示UI用：ステート遷移で実際に発動したアクションを通知
        if (stateName == "Rush") {
            EmitAction(ActionKind::Rush);
        } else if (stateName == "Guard") {
            EmitAction(ActionKind::Guard);
        } else if (stateName == "EnergyCharge") {
            EmitAction(ActionKind::EnergyCharge);
        }
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
        // 方向分類用の X はスティックの符号そのままを使う（右スティック→右アニメ）。
        // Move() の移動計算では cameraRight が -X 基準のため符号を反転しているが、
        // ここは方向分類なので反転しないことでキーボード(D=Right)と左右を一致させる
        float xInput = gamePad_->GetLeftStickX(); // 左スティックX軸
        float zInput = gamePad_->GetLeftStickY(); // 左スティックY軸

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
    // ガード中は遠距離射撃を発射できない（既存弾の更新は下で継続する）
    if (currentState_ != states_["EnergyCharge"].get() && !isSkillMenu_ && !isGuarding_) {
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
                EmitAction(ActionKind::NormalShot); // 入力表示UI用：通常射撃を通知
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
                    EmitAction(ActionKind::NormalShot); // 入力表示UI用：通常射撃を通知
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
    // ガード中は必殺技を発動できない
    if (isGuarding_) {
        return;
    }
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
                EmitAction(ActionKind::Special); // 入力表示UI用：必殺技を通知
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
                    EmitAction(ActionKind::Special); // 入力表示UI用：必殺技を通知
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
    // ガード中は近接コンボを実行できない
    if (currentState_ != states_["EnergyCharge"].get() && !chargeShot_->GetIsCharge() && !isGuarding_) {
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
        if (input_->TriggerKey(DIK_LCONTROL)) {
            EmitAction(ActionKind::Dash); // 入力表示UI用：ダッシュ開始を通知
        }
    } else {
        // ゲームパッド入力
        xInput = -gamePad_->GetLeftStickX(); // 左スティックX軸
        zInput = gamePad_->GetLeftStickY();  // 左スティックY軸

        // 左スティックが倒れているか（ダッシュ維持の判定に使う）
        bool hasStickInput = (xInput != kInputZero || zInput != kInputZero);

        // Aボタンのトリガーでダッシュ開始（LTは不要）。
        // すでにダッシュ中の場合は再始動しないため、ダッシュ中のA入力は
        // Rush 側のトリガーとして扱われる（PlayerStateFly* の TryChangeToRush）。
        if (gamePad_->IsTrigger(XINPUT_GAMEPAD_A) && !isDashing_) {
            dashInputX_ = xInput;
            dashInputZ_ = zInput;
            isDashing_ = true;
            dashStartedThisFrame_ = true;
            dashDuration_ = 0.0f;
            // A押下時にスティックがニュートラルなら、猶予時間内に倒せばダッシュ継続を許可。
            // 既にスティックを倒していれば従来通り即ダッシュ確定（猶予不要）。
            dashGraceTimer_ = hasStickInput ? 0.0f : kDashGraceTime;
            EmitAction(ActionKind::Dash); // 入力表示UI用：ダッシュ開始を通知
        }

        // 猶予中にスティックを倒したらダッシュを確定（以降は通常の維持判定に従う）。
        // ニュートラルのままなら猶予時間をカウントダウンする。
        if (isDashing_ && hasStickInput) {
            dashGraceTimer_ = 0.0f;
        } else if (isDashing_ && dashGraceTimer_ > 0.0f) {
            dashGraceTimer_ -= dt_;
        }

        // スティックをニュートラルに戻したらダッシュ解除。
        // ただし開始フレームと猶予時間中（A押下→移動待ち）はニュートラルでも維持する。
        if (isDashing_ && !hasStickInput && !dashStartedThisFrame_ && dashGraceTimer_ <= 0.0f) {
            isDashing_ = false;
            dashInputX_ = kInputZero;
            dashInputZ_ = kInputZero;
            dashDuration_ = 0.0f;
            dashGraceTimer_ = 0.0f;
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

    // ダメージリアクションの開始（点滅のみ）
    isDamageReact_ = true;
    damageReactTimer_ = kTimerReset;
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

    // ─── アニメーション制御 ───
    if (ImGui::CollapsingHeader("アニメーション制御")) {
        animationController_.DrawImGui();
    }

    // ─── 飛行リーン（体の傾き）───
    if (ImGui::CollapsingHeader("飛行リーン")) {
        ImGui::Checkbox("有効", &flyLeanEnabled_);
        ImGui::TextDisabled("ロックオン飛行移動中、顔は敵向きのまま体を進行方向へ倒します");
        ImGui::DragFloat("前傾の最大角(度)", &flyLeanMaxFwdPitchDeg_, 0.5f, 0.0f, 90.0f);
        ImGui::DragFloat("仰け反りの最大角(度)", &flyLeanMaxBackPitchDeg_, 0.5f, 0.0f, 90.0f);
        ImGui::DragFloat("横移動のヨー(Y回転)最大角(度・90で真横)", &flyLeanMaxSideDeg_, 0.5f, 0.0f, 90.0f);
        ImGui::DragFloat("基準速度", &flyLeanRefSpeed_, 0.1f, 0.1f, 30.0f);
        ImGui::DragFloat("追従速度", &flyLeanResponse_, 0.1f, 0.1f, 30.0f);
        ImGui::DragFloat3("回転中心(ピボット)", &flyLeanPivot_.x, 0.05f);
        Vector3 leanEuler = flyLeanRotation_.ToEulerDegrees();
        ImGui::Text("現在の傾き(オイラー換算): X %.1f / Y %.1f / Z %.1f度",
                    leanEuler.x, leanEuler.y, leanEuler.z);
        if (ImGui::Button("飛行リーン設定を保存")) {
            Save();
        }
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

    if (isDashing_) {
        dashDuration_ += dt_; // ダッシュ継続時間を更新（A＋スティックで維持）
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

    // エネルギーが被弾消費量未満ならガードに入れない（無くなった/支払えない場合は不可）
    if (!CanGuard()) {
        return;
    }

    // 近接コンボ中・チャージショット溜め中はガードに入れない（攻撃とガードは排他）
    if (punchCombo_.IsComboActive() || (chargeShot_ && chargeShot_->GetIsCharge())) {
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
                // 攻撃モーションはループ無し・短い補間でテンポよく切り替える
                animationController_.PlayFile(path, false, 1.0f, 0.1f);
            }
            // else: 該当段のアニメーションが未設定（空文字）なので何もしない
        }
        return; // 攻撃中は以降のステート判定をスキップ
    }

    // ──────────────────────────────────────────
    // 通常ステート：ステート名に応じてクリップを切り替え
    // ──────────────────────────────────────────
    const std::string stateName = GetCurrentStateName();

    if (stateName == "Idle") {
        // 地上待機
        animationController_.Play("Idle");

    } else if (stateName == "Move") {
        // 地上移動 ― 移動方向に応じて前後左右のクリップを使い分ける
        switch (moveDir_) {
        case MoveDirection::Behind:
            animationController_.Play("RunBack");
            break;
        case MoveDirection::Left:
            animationController_.Play("RunLeft");
            break;
        case MoveDirection::Right:
            animationController_.Play("RunRight");
            break;
        case MoveDirection::Forward:
        default:
            animationController_.Play("Run");
            break;
        }

    } else if (stateName == "Jump" || stateName == "Air") {
        // ジャンプ・空中（上昇/滞空/下降）
        animationController_.Play("Jump");

    } else if (stateName == "FlyIdle") {
        // 浮遊待機
        animationController_.Play("FlyIdle");

    } else if (stateName == "FlyMove") {
        // 浮遊移動 ― 後退・上昇・下降は Idle_Flying、前進・左右移動は Running_Fly を使う。
        // 縦移動(|velocity_.y| > 閾値)または後退(moveDir_ == Behind)は、仰向け気味の
        // Idle_Flying の方がモーションとして自然なため優先する
        bool verticalMove = std::abs(velocity_.y) > kFlyVerticalAnimThreshold;
        bool movingBackward = (moveDir_ == MoveDirection::Behind);
        if (verticalMove || movingBackward) {
            animationController_.Play("FlyIdle");
        } else {
            animationController_.Play("FlyMove");
        }

    } else if (stateName == "Rush") {
        // ダッシュ突進
        animationController_.Play("FlyMove");

    } else if (stateName == "Guard") {
        // ガード
        animationController_.Play("Guard");

    } else if (stateName == "EnergyCharge") {
        // エネルギーチャージ ― 専用モーションなし（待機で代用）
        animationController_.Play("Idle");
    }
}

void Player::UpdateFlyLean() {
    // 目標姿勢をクォータニオンで構築する。条件を満たさないときは無回転（直立）へ戻す
    Quaternion targetLean = Quaternion::IdentityQuaternion();

    // ロックオン飛行移動中のみ傾ける。
    // 非ロックオン時は体が進行方向を向くため傾け不要、地上やその他ステートでも適用しない
    bool active = flyLeanEnabled_ && isLockOn_ &&
                  currentState_ == states_["FlyMove"].get();

    if (active) {
        Vector3 horizontalVel = {velocity_.x, 0.0f, velocity_.z};
        float speed = horizontalVel.Length();

        // 体の「水平」な前方・右方向（ロックオンの上下ピッチを除いた安定フレーム）。
        // 傾きはこの体の実ワールド軸まわりに作ることで、向きが変わっても
        // 「後ろへ倒れる」方向が一定になる（ワールド固定軸だと向きでズレる）
        Vector3 fwdAxis = {GetForward().x, 0.0f, GetForward().z};
        Vector3 rightAxis = {GetRight().x, 0.0f, GetRight().z};
        float fwdLen = fwdAxis.Length();
        float rightLen = rightAxis.Length();

        if (speed > 0.001f && fwdLen > 0.001f && rightLen > 0.001f) {
            Vector3 dir = horizontalVel / speed; // 進行方向（ワールド・水平）
            fwdAxis = fwdAxis / fwdLen;
            rightAxis = rightAxis / rightLen;

            // 進行方向を体の水平フレームへ分解する
            float f = dir.Dot(fwdAxis);   // +で前進（敵方向） / -で後退
            float r = dir.Dot(rightAxis); // +で右 / -で左

            float refSpeed = (flyLeanRefSpeed_ > 0.001f) ? flyLeanRefSpeed_ : 1.0f;
            float speedFrac = std::clamp(speed / refSpeed, 0.0f, 1.0f);

            // 後退時は仰向け、前進時は前傾。左右はヨーのみ（ロールなし）
            float pitchMaxDeg = (f >= 0.0f) ? flyLeanMaxFwdPitchDeg_ : flyLeanMaxBackPitchDeg_;
            float pitch = degreesToRadians(-f * pitchMaxDeg) * speedFrac;
            float yaw = degreesToRadians(-r * flyLeanMaxSideDeg_) * speedFrac;

            // ピッチは体の水平右方向まわり（向きに追従＝向き非依存の仰向け）、
            // ヨーは鉛直まわり（向き非依存）。横移動のみのときはヨーだけになる
            Quaternion qPitch = Quaternion::FromAxisAngle(rightAxis, pitch);
            Quaternion qYaw = Quaternion::FromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), yaw);
            targetLean = qYaw * qPitch;
        }
    }

    // クォータニオンの球面補間（Slerp）で滑らかに追従させる（フレームレート非依存）
    float t = std::clamp(flyLeanResponse_ * dt_, 0.0f, 1.0f);
    flyLeanRotation_ = Quaternion::Slerp(flyLeanRotation_, targetLean, t);

    // ほぼ無回転（直立）に戻ったら描画オフセットを解除する
    if (flyLeanRotation_.w > 0.99999f) {
        flyLeanRotation_ = Quaternion::IdentityQuaternion();
        ClearRenderRotationOffset();
        return;
    }

    // 体のローカル空間の傾きとして適用。モデル中心が原点にないため flyLeanPivot_ で回転中心を補正する
    SetRenderRotationOffset(flyLeanRotation_, flyLeanPivot_);
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
    data_->Save("flyLeanEnabled", flyLeanEnabled_ ? 1 : 0);
    data_->Save("flyLeanMaxFwdPitchDeg", flyLeanMaxFwdPitchDeg_);
    data_->Save("flyLeanMaxBackPitchDeg", flyLeanMaxBackPitchDeg_);
    data_->Save("flyLeanMaxSideDeg", flyLeanMaxSideDeg_);
    data_->Save("flyLeanRefSpeed", flyLeanRefSpeed_);
    data_->Save("flyLeanResponse", flyLeanResponse_);
    data_->Save("flyLeanPivot", flyLeanPivot_);
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
    flyLeanEnabled_ = data_->Load<int>("flyLeanEnabled", flyLeanEnabled_ ? 1 : 0) != 0;
    flyLeanMaxFwdPitchDeg_ = data_->Load<float>("flyLeanMaxFwdPitchDeg", flyLeanMaxFwdPitchDeg_);
    flyLeanMaxBackPitchDeg_ = data_->Load<float>("flyLeanMaxBackPitchDeg", flyLeanMaxBackPitchDeg_);
    flyLeanMaxSideDeg_ = data_->Load<float>("flyLeanMaxSideDeg", flyLeanMaxSideDeg_);
    flyLeanRefSpeed_ = data_->Load<float>("flyLeanRefSpeed", flyLeanRefSpeed_);
    flyLeanResponse_ = data_->Load<float>("flyLeanResponse", flyLeanResponse_);
    flyLeanPivot_ = data_->Load<Hagine::Vector3>("flyLeanPivot", flyLeanPivot_);
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