#define NOMINMAX
#include "Enemy.h"
#include "Collider/CollisionManager.h"
#include "Utility/Debug/ImGui/ImGuiNotification.h"
#include "Particle/CSParticle/ParticleCSEditor.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChargeShot/ChargeShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include "Edit/MotionEditor/MotionEditor.h"
#include <Debug/Log/Logger.h>
#include <Utility/Debug/GameParam/GameParamHub.h>
#include <3d/Line/DrawLine3D.h>
#include <Frame.h>
#include <Object/Base/BaseObjectManager.h>
#include <cmath>
#include <numbers>

using namespace Hagine;
Enemy::Enemy() {}
Enemy::~Enemy() {
    // ポインタ失効前にゲームパラメータHubから登録を解除する
    GameParamHub::GetInstance()->Unregister("Enemy");
    GameParamHub::GetInstance()->Unregister("必殺演出(Enemy)");
}

void Enemy::Init(const std::string objectName) {
    BaseObject::Init(objectName);

    // プレイヤーと同じスケルトン付きモデルを使い、同一クリップでアニメーションさせる。
    // モデル素体のマテリアル色は赤なので、敵はそのまま赤で表示される
    BaseObject::CreateModel("animation/Player/Idle_Ground.gltf");
    BaseObject::SetOffset({0.0f, -0.75f, 0.0f}); // 描画オフセット（足が地面につくように）
    BaseObject::GetLocalScale() = {4.0f, 4.0f, 4.0f};
    BaseObject::SetAnimationSpeed(1.0f);
    BaseObject::SetAnimationBlendDuration(0.2f);

    // ───────────────────────────────────────────
    // アニメーションコントローラへクリップを登録（プレイヤーと同一構成）
    // ───────────────────────────────────────────
    animationController_.Initialize(GetObject3d());
    animationController_.RegisterClip("Idle", "animation/Player/Idle_Ground.gltf", true);
    animationController_.RegisterClip("FlyIdle", "animation/Player/Idle_Flying.gltf", true);
    animationController_.RegisterClip("Run", "animation/Player/Running.gltf", true);
    animationController_.RegisterClip("RunBack", "animation/Player/Running_Back.gltf", true);
    animationController_.RegisterClip("RunLeft", "animation/Player/Running_Left.gltf", true);
    animationController_.RegisterClip("RunRight", "animation/Player/Running_Right.gltf", true);
    animationController_.RegisterClip("FlyMove", "animation/Player/Running_Fly.gltf", true);
    animationController_.RegisterClip("Guard", "animation/Player/Block_Idle.gltf", true);
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
    // プレイヤー側で調整済みのクリップ設定（速度・補間）を流用する
    animationController_.LoadClips("AnimationController", "PlayerClips");

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

    BaseObject::SetColor(Vector4(kColorRed, kColorZero, kColorZero, kColorOpaque));

    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("smokeEmitter");
    chargeShake_ = std::make_unique<Shake>();
    isGuarding_ = false;

    // チャージ攻撃演出（enemyChargeAura: 既存の気弾チャージオーラ）
    chargeAura_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("enemyChargeAura");
    // ビームメイン演出（プレイヤーの MakanAttackSkill と同じテンプレートを流用）
    beamMainEffect_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("makan_main");
    // ビームらせん演出
    beamAroundEffect_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("makan_around");

    // ビーム判定コライダー（初期は無効化）
    beamCollider_ = AddOBBCollider("enemy_BeamCollider");
    beamCollider_->SetTag("EnemyBeam");
    beamCollider_->AddCollisionMask("Player");
    beamCollider_->SetEnabled(false);
    beamCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        // ビームアクティブ中かつ未ダメージ処理のとき一度だけダメージを与える
        if (other->GetTag() == "Player" && beamActive_ && !beamDamageDealt_ && target_) {
            target_->SetDamage(kBeamDamage);
            beamDamageDealt_ = true;
        }
    });

    // 移動パラメータ
    moveSpeed_ = 5.0f;
    jumpSpeed_ = 15.0f;
    maxSpeed_ = 10.0f;
    accelRate_ = 1.0f;

    // 前方攻撃判定コライダー（PlayerAttackColliderと対称の設計）
    attackCollider_ = std::make_unique<EnemyAttackCollider>();
    attackCollider_->Init(this);

    // コンボ登録（ダメージ・ノックバックはImGuiで調整・セーブ可能）
    if (!comboInitialized_) {
        punchCombo_.SetName("EnemyPunchCombo"); // DataHandlerのファイル名

        // 攻撃の見た目は本体アニメーション（comboAnimations_）で再生するため、
        // モーション再生用ターゲットは不要（nullptr）。ダメージ等のパラメータのみ指定する
        punchCombo_
            .Add(nullptr, "Jab", 8.0f, 2.0f, 0.25f, 0.08f)
            .Add(nullptr, "Hook", 10.0f, 3.0f, 0.25f, 0.08f)
            .Add(nullptr, "Cross", 10.0f, 3.0f, 0.25f, 0.08f)
            .Add(nullptr, "Uppercut", 12.0f, 5.0f, 0.30f, 0.10f)
            .Add(nullptr, "Overhand", 12.0f, 5.0f, 0.30f, 0.10f)
            .Add(nullptr, "Swing", 14.0f, 6.0f, 0.30f, 0.10f)
            .Add(nullptr, "Elbow", 16.0f, 7.0f, 0.25f, 0.06f)
            .Add(nullptr, "Slam", 20.0f, 10.0f, 0.35f, 0.12f);

        // JSONがあれば保存済みの値で上書き
        punchCombo_.LoadAttackParams();

        // 攻撃発火時にダメージ・ノックバックを保存し、前方コライダーを有効化する
        punchCombo_.SetOnAttackFired(
            [this](float damage, float knockback, float duration, float /*delay*/) {
                currentAttackDamage_ = damage;
                currentAttackKnockback_ = knockback;
                currentAttackDuration_ = duration;
                if (attackCollider_) {
                    attackCollider_->Activate(damage, knockback, duration);
                }
            });

        comboInitialized_ = true;
    }

    // コンボ段ごとの本体アニメーション（プレイヤーと同じ割り当て：パンチ4段＋キック3段＋叩きつけ）
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

    isGrounded_ = true;
    velocity_ = Vector3(0.0f, 0.0f, 0.0f);
    acceleration_ = Vector3(0.0f, 0.0f, 0.0f);

    transform_->SetRotationEuler({0.0f, degreesToRadians(180.0f), 0.0f});

    // ─── 調整パラメータをゲームパラメータHubへ登録 ───
    {
        auto *hub = GameParamHub::GetInstance();
        hub->Register("Enemy", "HP", static_cast<const float *>(&HP_));
        hub->Register("Enemy", "エネルギー", static_cast<const float *>(&energy_));
        hub->Register("Enemy", "移動速度", &moveSpeed_, {0.1f, 0.0f, 50.0f});
        hub->Register("Enemy", "最大速度", &maxSpeed_, {0.1f, 0.0f, 50.0f});
        hub->Register("Enemy", "加速率", &accelRate_, {0.1f, 0.0f, 50.0f});
        hub->Register("Enemy", "ジャンプ速度", &jumpSpeed_, {0.1f, 0.0f, 50.0f});
        hub->Register("Enemy", "エネルギー回復速度", &energyRecoveryRate_, {0.1f, 0.0f, 50.0f});
        hub->Register("Enemy", "被弾リアクション時間", &damageReactDuration_, {0.05f, 0.0f, 3.0f});
    }
    beamCutscene_.RegisterParams("必殺演出(Enemy)");
}

void Enemy::Update() {
    // 開始フラグが立っており、ポーズ中でなく、ターゲットが生きている場合に更新
    if (started_ && !isPause_ && target_->GetIsAlive()) {
        DamageUpdate();
        RecoverEnergy();

        // 死亡判定
        if (HP_ <= kMinHP) {
            if (dummyMode_) {
                // ダミーはHPが尽きたら即復活する（満タン・初期位置へ）
                Revive();
            } else {
                isAlive_ = false;
                HP_ = kMinHP;
            }
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

        // 回転を更新（敵をプレイヤーへ向ける）
        RotateUpdate();

        // ダミーモードでは自身の攻撃(コンボ)は行わない
        if (!dummyMode_) {
            ConboUpdate();
        }
        chargeShake_->Update();

        // 大技演出エミッタの追従＆更新
        {
            Vector3 selfPos = GetWorldPosition();
            Quaternion selfRot = GetLocalRotation();
            if (chargeAura_) {
                chargeAura_->SetTranslate(selfPos);
                chargeAura_->SetRotation(-selfRot);
                chargeAura_->Update();
            }
            if (beamMainEffect_) {
                beamMainEffect_->Update();
            }
            if (beamAroundEffect_) {
                beamAroundEffect_->Update();
            }
        }

        // ビーム発動前演出（カメラ顔アップ→遅延→発動）の進行
        beamCutscene_.Update(Frame::DeltaTime());

        // ビーム必殺技のフレーム更新
        UpdateBeam();

        // ダメージリアクション処理（高速点滅のみ。傾き(のけぞり)演出は廃止）
        if (isDamageReact_) {
            damageReactTimer_ += Frame::DeltaTime();

            float blinkInterval = kDamageBlinkInterval;
            int blink = static_cast<int>(damageReactTimer_ / blinkInterval);
            SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kAlphaTransparent : kAlphaOpaque);

            if (damageReactTimer_ >= damageReactDuration_) {
                isDamageReact_ = false;
                SetAlpha(kAlphaOpaque);
            }
        }

        // ビヘイビアツリーの更新（ダミーモードではAIを動かさない）
        if (rootNode_ && !dummyMode_) {
            rootNode_->SetContext(this, target_);
            rootNode_->Tick();

            // ガード中は移動させない（EnemyGuardNode が毎フレーム速度を0にしているため、
            // ここで移動イージングを適用すると追跡速度で上書きされて動いてしまう）
            if (isGuarding_) {
                velocity_.x = 0.0f;
                velocity_.z = 0.0f;
            }
            // 速度イージングの更新（ガード中は適用しない）
            else if (velocityEase_.isActive) {
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
        } else if (dummyMode_) {
            // ダミー: AIは動かさないが、被弾ノックバックは残す。
            // 水平速度に摩擦をかけて徐々に停止させ、重力だけ適用する。
            velocity_.x *= kDummyGroundFriction;
            velocity_.z *= kDummyGroundFriction;
            if (!isGrounded_) {
                velocity_.y += acceleration_.y * Frame::DeltaTime();
            } else {
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

        // 移動・コンボ・ガード状態に応じてアニメーションクリップを切り替える
        UpdateAnimation();

        // 接地判定
        CollisionGround();
        UpdateFrustumLockOn();

        // 弾の更新（ダミーモードでは弾を撃たないためスキップ）
        if (!dummyMode_) {
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

    // ワールドトランスフォームの行列更新は started_ に関わらず毎フレーム実行する。
    // StartCamera演出中は started_ が false のためゲームロジックはスキップされるが、
    // 行列が未更新のままだと Draw() に正しい行列が渡らず描画されなくなる。
    BaseObject::Update();
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

void Enemy::UpdateAnimation() {
    // ──────────────────────────────────────────
    // コンボ攻撃中：段数に対応したアニメーションを再生（プレイヤーと同じロジック）
    // GetCurrentComboIndex() は「次に実行する」インデックスなので、
    // 現在再生中の段 = nextIdx - 1（0 のときは最終段の後）
    // ──────────────────────────────────────────
    if (punchCombo_.IsComboActive()) {
        int nextIdx = punchCombo_.GetCurrentComboIndex();
        int comboLen = punchCombo_.GetComboLength();
        int animIdx = (nextIdx == 0) ? (comboLen - 1) : (nextIdx - 1);

        if (animIdx >= 0 && animIdx < static_cast<int>(comboAnimations_.size())) {
            const std::string &path = comboAnimations_[animIdx];
            if (!path.empty()) {
                animationController_.PlayFile(path, false, 1.0f, 0.1f);
            }
        }
        return; // 攻撃中は以降の判定をスキップ
    }

    // ガード中
    if (isGuarding_) {
        animationController_.Play("Guard");
        return;
    }

    // ──────────────────────────────────────────
    // 移動状態でクリップを選択する。
    // 敵はBT駆動で moveDir_ を持たないため、velocity と向きから進行方向を判定する
    // ──────────────────────────────────────────
    Vector3 horizontalVel = {velocity_.x, 0.0f, velocity_.z};
    float hSpeed = horizontalVel.Length();
    bool verticalMove = std::abs(velocity_.y) > kFlyVerticalAnimThreshold;

    if (isFlying_ || !isGrounded_) {
        // 飛行中 ― プレイヤーと同じ規則：
        // 後退・上昇・下降は Idle_Flying、前進・左右移動は Running_Fly
        if (hSpeed < kMoveAnimMinSpeed && !verticalMove) {
            animationController_.Play("FlyIdle");
            return;
        }

        // 「前進／後退」はプレイヤーの位置を基準に判定する。
        // 敵は常にプレイヤーへ向くため、プレイヤーへ近づく成分が＋＝前進、離れる成分が－＝後退。
        // 向き(GetForward)はクォータニオン規約の影響でプレイヤーと逆を指すことがあるため、
        // 実ワールド位置から求めたベクトルで判定する方が確実
        bool movingBackward = false;
        if (target_ && hSpeed > kMinRotationDistance) {
            Vector3 toPlayer = target_->GetWorldPosition() - GetWorldPosition();
            toPlayer.y = 0.0f;
            float toLen = toPlayer.Length();
            if (toLen > kMinRotationDistance) {
                float f = (horizontalVel / hSpeed).Dot(toPlayer / toLen); // +接近(前進) / -後退
                movingBackward = (f < -kBackwardDotThreshold);
            }
        }

        if (verticalMove || movingBackward) {
            animationController_.Play("FlyIdle");
        } else {
            animationController_.Play("FlyMove");
        }
        return;
    }

    // 地上 ― 待機 / 移動
    if (hSpeed < kMoveAnimMinSpeed) {
        animationController_.Play("Idle");
    } else {
        animationController_.Play("Run");
    }
}

void Enemy::Draw(const ViewProjection &viewProjection) {
    if (!isAlive_) {
        enemyCollider_->SetEnabled(false);
        return;
    }
    BaseObject::Draw(viewProjection);
    if (transform_->translation_.y < kGroundLevel)
        return;
}

void Enemy::DrawParticleCompute(const ViewProjection &viewProjection) {
    if (chargeAura_)
        chargeAura_->DrawCompute(viewProjection);
    if (beamMainEffect_)
        beamMainEffect_->DrawCompute(viewProjection);
    if (beamAroundEffect_)
        beamAroundEffect_->DrawCompute(viewProjection);
}

void Enemy::DrawParticle(const ViewProjection &viewProjection) {
    hitEmitter_->Draw(viewProjection); // CPU emitter
    // 大技演出（Graphics フェーズ）
    if (chargeAura_)
        chargeAura_->DrawGraphics(viewProjection);
    if (beamMainEffect_)
        beamMainEffect_->DrawGraphics(viewProjection);
    if (beamAroundEffect_)
        beamAroundEffect_->DrawGraphics(viewProjection);
    // 前方攻撃判定コライダーのヒットエフェクト
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

    // 前方攻撃判定（PlayerHand）ヒット時のパーティクル（ダメージ計算はコライダー側）
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

void Enemy::ConboUpdate() {
    punchCombo_.Update(Frame::DeltaTime());

    if (isComboAttack_) {
        punchCombo_.TryExecuteCombo();
        isComboAttack_ = false;
    }

    // 前方攻撃判定コライダーの遅延・有効時間タイマーを進める
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
void Enemy::Save() {
    ImGuiNotification::Post("エネミー設定を保存しました", {0.2f, 0.8f, 0.2f, 1.0f});
}
void Enemy::Load() {
    ImGuiNotification::Post("エネミー設定を読み込みました", {0.2f, 0.8f, 0.8f, 1.0f});
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

    // ダメージ計算（ガード時は軽減し、エネルギーを消費する：プレイヤーと同様）
    float actualDamage = damage_;
    if (isGuarding_) {
        actualDamage *= kGuardDamageMultiplier;
        ConsumeEnergy(kGuardEnergyCost);
    }
    HP_ -= actualDamage;
    damage_ = kNoDamage;

    // ノックバック適用（ガード中は軽減）
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
}

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

void Enemy::SetDummy(bool enable) {
    dummyMode_ = enable;
    if (enable && transform_) {
        // 呼び出し時点の位置を復活位置として記録する
        spawnPosition_ = transform_->translation_;
    }
}

void Enemy::Revive() {
    HP_ = maxHP_;
    energy_ = maxEnergy_;
    isAlive_ = true;

    // 速度・ノックバック・被弾リアクションをクリア
    velocity_ = {0.0f, 0.0f, 0.0f};
    acceleration_ = {0.0f, 0.0f, 0.0f};
    hasKnockback_ = false;
    pendingKnockback_ = {0.0f, 0.0f, 0.0f};
    isDamageReact_ = false;
    isGrounded_ = true;
    SetAlpha(kAlphaOpaque);

    // 初期位置へ戻す
    if (transform_) {
        transform_->translation_ = spawnPosition_;
    }
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

void Enemy::StartChargeAura() {
    if (chargeAura_) {
        chargeAura_->SetTranslate(GetWorldPosition());
        chargeAura_->SetAuto(true);
    }
    if (chargeShake_) {
        chargeShake_->StartShake();
    }
}

void Enemy::StopChargeAura() {
    if (chargeAura_)
        chargeAura_->SetAuto(false);
}

void Enemy::FireChargeBlast() {
    StopChargeAura();

    if (!target_)
        return;

    // エネルギーが足りなければ通常弾にフォールバック
    if (!ConsumeEnergy(kChargeAttackEnergyCost)) {
        Shot();
        return;
    }

    // 強化ホーミング弾を1発撃つ
    std::string bulletName = "EnemyChargeBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<EnemyBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(this);
    bullet->GetLocalScale() = {kChargeBlastScale, kChargeBlastScale, kChargeBlastScale};
    bullet->SetColliderRadius(kBulletColliderRadius * kChargeBlastScale);
    bullet->SetIsLockOnBullet(true);
    bullet->SetSpeed(kChargeBlastSpeed);
    bullet->SetDamage(kChargeBlastDamage);

    Vector3 toTarget = target_->GetLocalPosition() - GetLocalPosition();
    float len = toTarget.Length();
    toTarget = (len > kMinRotationDistance) ? toTarget / len : GetForward();
    bullet->SetVelocity(toTarget * kChargeBlastSpeed);

    bullets_.push_back(std::move(bullet));
}

void Enemy::ActivateBeam() {
    if (beamActive_)
        return;

    ConsumeEnergy(kUltimateEnergyCost);
    beamActive_ = true;
    beamDamageDealt_ = false;
    beamLength_ = 0.0f;
    beamActiveTime_ = 0.0f;
    beamSpiralTime_ = 0.0f;
    // 発射時点の向きを固定する。ビーム持続中はこの向きを使い続けることで
    // ターゲット追従（ホーミング）を防ぐ。
    beamLockedRotation_ = transform_->quateRotation_;

    if (beamMainEffect_)
        beamMainEffect_->SetAuto(true);
    if (beamAroundEffect_)
        beamAroundEffect_->SetAuto(true);
    if (chargeShake_)
        chargeShake_->StartShake();
}

void Enemy::StartBeamStaging() {
    if (beamActive_ || beamCutscene_.IsActive())
        return;

    // カメラはプレイヤーのフォローカメラを借りて敵の顔に寄せる
    FollowCamera *camera = target_ ? target_->GetCamera() : nullptr;

    // 演出・遅延中はロックオン（照準追従）を維持し、発動の瞬間に固定する。
    // ActivateBeam() 内で発射時の quateRotation_ が beamLockedRotation_ に
    // スナップショットされるため、以降は向きが固定され回避が可能になる
    beamCutscene_.Start(this, camera, [this] {
        SetIsLockOn(false);
        StopChargeAura();
        ActivateBeam();
    });
}

void Enemy::CancelBeamStaging() {
    beamCutscene_.Cancel();
}

void Enemy::DeactivateBeam() {
    beamActive_ = false;
    beamLength_ = 0.0f;
    beamActiveTime_ = 0.0f;
    beamSpiralTime_ = 0.0f;

    if (beamMainEffect_)
        beamMainEffect_->SetAuto(false);
    if (beamAroundEffect_)
        beamAroundEffect_->SetAuto(false); // 新規発生を止める（既存パーティクルは自然消滅）
    if (beamCollider_)
        beamCollider_->SetEnabled(false); // 判定を即無効化（ダメージは止める）
}

void Enemy::UpdateBeam() {
    if (!beamActive_)
        return;

    float dt = Frame::DeltaTime();
    beamActiveTime_ += dt;

    // ビーム発射中は transform_->quateRotation_ を発射時の固定向きで上書きする。
    // OBBコライダーは transform_->quateRotation_ から向きを取るため、
    // ここで上書きしないと RotateUpdate() によりコライダーがプレイヤーを追従し続ける。
    transform_->quateRotation_ = beamLockedRotation_;

    // ビーム長を前方に伸ばす
    beamLength_ += kBeamExtendSpeed * dt;
    if (beamLength_ > kBeamMaxLength)
        beamLength_ = kBeamMaxLength;

    Vector3 selfPos = GetWorldPosition();
    Quaternion selfRot = beamLockedRotation_;

    // メインビームエミッタの設定（MakanAttackSkill と同じアプローチ）
    // SetScale の Z 成分でビームの長さを制御し、SetAnchorPoint でビームの起点を調整する
    if (beamMainEffect_) {
        beamMainEffect_->SetAuto(true);
        beamMainEffect_->SetScale(Vector3(0.0f, 0.0f, beamLength_));
        beamMainEffect_->SetAnchorPoint(Vector3(0.5f, 0.5f, 0.75f));
        beamMainEffect_->SetTranslate(selfPos);
        beamMainEffect_->SetRotation(selfRot);
    }

    // らせん状エミッタの設定（MakanAttackSkill と同じアプローチ）
    if (beamAroundEffect_) {
        beamAroundEffect_->SetAuto(true);
        beamSpiralTime_ += dt;

        // クォータニオンからローカル座標系の基底ベクトルを計算する
        // RotateUpdate が +Z をプレイヤー方向に向けるため、
        // localForward（+Z 基底）はそのままプレイヤー方向を向く
        Quaternion q = selfRot;
        Vector3 localRight(
            1.0f - 2.0f * (q.y * q.y + q.z * q.z),
            2.0f * (q.x * q.y - q.w * q.z),
            2.0f * (q.x * q.z + q.w * q.y));
        Vector3 localUp(
            2.0f * (q.x * q.y + q.w * q.z),
            1.0f - 2.0f * (q.x * q.x + q.z * q.z),
            2.0f * (q.y * q.z - q.w * q.x));
        Vector3 localForward(
            2.0f * (q.x * q.z - q.w * q.y),
            2.0f * (q.y * q.z + q.w * q.x),
            1.0f - 2.0f * (q.x * q.x + q.y * q.y));

        // らせんパーティクルの前進距離
        float forwardDistance = (beamSpiralTime_ * beamSpiralForwardSpeed_) * 2.0f;
        if (forwardDistance > beamLength_ * 2.0f)
            forwardDistance = beamLength_ * 2.0f;

        // ビーム長に対する進捗に応じて巻き角度を決定する
        float t = forwardDistance / kBeamMaxLength;
        float angle = t * beamSpiralRevolution_ * (2.0f * std::numbers::pi_v<float>);

        // らせんオフセット（ローカル座標系で計算）
        Vector3 spiralOffset = localRight * (std::cos(angle) * beamSpiralRadius_) +
                               localUp * (std::sin(angle) * beamSpiralRadius_);

        Vector3 emitterPos = selfPos + localForward * forwardDistance + spiralOffset;
        beamAroundEffect_->SetTranslate(emitterPos);
        beamAroundEffect_->SetScale(Vector3(0.3f, 0.3f, 0.3f));
    }

    // OBBコライダーをビーム形状に更新する
    if (beamCollider_) {
        beamCollider_->SetEnabled(true);
        beamCollider_->SetSize(Vector3(kBeamWidth, kBeamWidth, beamLength_));
        beamCollider_->SetAnchorPoint(Vector3(0.5f, 0.5f, 1.0f));
    }

    // 持続時間が過ぎたら自動停止
    if (beamActiveTime_ >= kBeamDuration) {
        DeactivateBeam();
    }
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