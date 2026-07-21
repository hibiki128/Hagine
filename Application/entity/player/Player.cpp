#define NOMINMAX
#include "Player.h"
#include "Application/staging/screen/ScreenFlash.h"
#include "frame/Frame.h"
#include "utility/debug/imgui/ImGuiNotification.h"

#include "collider/CollisionManager.h"
#include "object/base/BaseObjectManager.h"
#include "state/action/PlayerEnergyCharge.h"
#include "state/action/PlayerStateGuard.h"
#include "state/action/PlayerStateRush.h"
#include "state/air/PlayerStateAir.h"
#include "state/fly/PlayerStateFlyIdle.h"
#include "state/fly/PlayerStateFlyMove.h"
#include "state/ground/PlayerStateIdle.h"
#include "state/ground/PlayerStateJump.h"
#include "state/ground/PlayerStateMove.h"
#include <Application/camera/follow/FollowCamera.h>
#include <Application/entity/enemy/Enemy.h>
#include <particle/gpu/ParticleCSEditor.h>
#include <particle/ParticleEditor.h>
#include <utility/debug/param/GameParamHub.h>
#include <cmath>

using namespace Hagine;

Player::Player()
{
    movement_ = std::make_unique<PlayerMovement>();
    combat_ = std::make_unique<PlayerCombat>();
    status_ = std::make_unique<PlayerStatus>();
    visual_ = std::make_unique<PlayerVisual>();
}

Player::~Player()
{
    // 追加したポストエフェクトスロットを解放する（グローバルなオフスクリーンに残さない）
    if (screenFlash_)
    {
        screenFlash_->Finalize();
    }

    // ポインタ失効前にゲームパラメータHubから登録を解除する
    GameParamHub::GetInstance()->Unregister("Player");
    GameParamHub::GetInstance()->Unregister("必殺演出(Player)");
    GameParamHub::GetInstance()->Unregister("瞬間移動コンボ(Player)");
}

void Player::Init(const std::string objectName)
{
    BaseObject::Init(objectName);

    // ベースモデル（地上待機）を生成する。各 gltf はメッシュ＋スケルトン＋
    // アニメーションを内包しており、スケルトンを共有するクリップを切り替えて再生する
    BaseObject::CreateModel("animation/Player/Idle_Ground.gltf");
    BaseObject::SetOffset({0.0f, kModelOffsetY, 0.0f}); // 描画オフセット（地面に足がつくように）
    BaseObject::GetLocalScale() = {4.0f, 4.0f, 4.0f};
    BaseObject::SetAnimationSpeed(1.0f);
    BaseObject::SetAnimationBlendDuration(0.2f);

    // 体色を青系にする。モデル素体（マテリアル色）は赤なので、敵は赤のまま、
    // プレイヤーだけここで青へ上書きする（シェーダは material.color * texture）
    for (int i = 0; i < GetObject3d()->GetMaterialCount(); ++i)
    {
        BaseObject::SetColor({0.15f, 0.35f, 1.0f, 1.0f}, i);
    }

    pPlayerCollider_ = AddOBBCollider("player_Collider");
    pPlayerCollider_->SetTag("Player");
    pPlayerCollider_->AddCollisionMask("Enemy");
    pPlayerCollider_->AddCollisionMask("CylinderField");

    pPlayerWallCollider_ = AddAABBCollider("player_WallCollider");
    pPlayerWallCollider_->SetTag("PlayerWall");
    pPlayerWallCollider_->AddCollisionMask("EnemyWall");
    pPlayerWallCollider_->SetSize({2.75f, 1000.0f, 2.5f});

    pPlayerCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    pPlayerCollider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
    });

    pPlayerWallCollider_->SetOnCollision([this](ColliderBase *other) {
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
    pCurrentState_ = states_["Idle"].get();

    data_ = std::make_unique<DataHandler>("EntityData", "Player");

    // パーツより先に入力を用意する（パーツ初期化中の参照に備える）
    pInput_ = Input::GetInstance();
    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);

    // ─── 各パーツの初期化 ───
    movement_->Init(this);
    status_->Init(this);
    combat_->Init(this);
    visual_->Init(this);

    Load();

    shake_ = std::make_unique<Shake>();

    auraEmitter_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("playerAura");
    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("smokeEmitter");

    dashEffect_ = std::make_unique<DashEffect>();
    dashEffect_->Init();

    deathStaging_ = std::make_unique<DeathStaging>();

    // 必殺技の画面白黒フラッシュ演出（ポストエフェクトの Gray + Bloom を利用）
    screenFlash_ = std::make_unique<ScreenFlash>();
    screenFlash_->Initialize();

    pGeneratedField_ = ParticleCSFieldManager::GetInstance()->GetField(0); // 0番目のフィールドを使用

    // ─── 調整パラメータをゲームパラメータHubへ登録 ───
    movement_->RegisterParams();
    status_->RegisterParams();
    combat_->RegisterParams();
    visual_->RegisterParams();
    screenFlash_->RegisterParams("Player");
}

void Player::Update()
{

    // 入力表示UI用アクションイベントは毎フレーム先頭でクリアする
    actionEvents_.clear();

    if (activeDebugCamera_)
        return;

    dt_ = Frame::DeltaTime();

    // 必殺技の画面白黒フラッシュ演出は、必殺技のビーム発射中（下の早期リターン経路）でも
    // 進める必要があるため、ここで毎フレーム更新する
    if (screenFlash_)
    {
        screenFlash_->Update(dt_);
    }

    // ダッシュ演出は emit フラグ残留を防ぐため毎フレーム更新する。
    // 通常ダッシュに加えて急接近（Rush）中も dashWind を発生させる。
    const bool dashActive = movement_->GetIsDashing() || (GetCurrentStateName() == "Rush");
    dashEffect_->Update(GetWorldPosition(), movement_->GetVelocity(), GetForward(),
                        isAlive_ && started_ && !isPause_ && dashActive,
                        movement_->GetIsGrounded());

    if (combat_->IsSkillActive())
    {
        // ビーム発動中も必殺技モーション（発射→フレーム80の撃ち終わりまで）は進める
        visual_->UpdateAnimation();
        BaseObject::Update();
        return;
    }

    if (!isAlive_)
    {
        // 空中で死んだ場合は重力で地面まで落下させる
        // （die モーション・粒子化演出は地面に倒れる前提のため、浮いたまま倒れると回転が合わない）
        if (!movement_->GetIsGrounded())
        {
            movement_->GetVelocity().y += movement_->GetFallSpeed() * dt_;
        }
        movement_->CollisionGround();

        // 死亡時は行動処理なし。死亡アニメーションだけ再生して進める
        // （再生し終わった後の粒子化演出は DrawParticle 側で描画する）
        visual_->PlayDeathAnimation();
        BaseObject::Update();
    }
    else
    {
        pGeneratedField_->data.position = GetWorldPosition();
        gamePad_->Update();

        if (status_->IsInvincible())
        {
            status_->InvincibleUpdate();
        }

        status_->DamageUpdate();

        // ひるみ（ヒットスタン）残り時間を進める
        status_->UpdateHitStun();

        if (started_ && !isPause_)
        {
            status_->RecoverEnergy();

            // ─── 必殺技のカメラ演出中は行動不能にする ───
            // ・カメラワーク（顔アップ）中: 自分/相手どちらの必殺技でも移動・行動をロックする
            // ・自分の必殺技演出中（顔アップ後の発動遅延も含む）: 発動が終わるまで自分をロックする
            //   （相手はカメラワーク終了後に回避できるよう、ここではロックしない）
            const bool cameraCloseUp = FollowCamera_ && FollowCamera_->IsSkillCloseUpActive();
            const bool selfSkillStaging = combat_->IsSkillStaging();
            const bool skillLocked = cameraCloseUp || selfSkillStaging;

            // ロック中は溜め操作を受け付けない（溜め演出のエミッタ管理は継続する）
            combat_->SetChargeActionLocked(skillLocked);

            if (skillLocked)
            {
                // ロック開始の瞬間に一度だけ待機ステートへ移し、移動アニメの足踏みを防ぐ
                if (!wasSkillLocked_)
                {
                    ChangeState(GetIsGrounded() ? "Idle" : "FlyIdle");
                }

                // その場で停止させる（水平移動を止める）
                Hagine::Vector3 &vel = movement_->GetVelocity();
                vel.x = 0.0f;
                vel.z = 0.0f;

                // 発動前演出（顔アップ→遅延→発動）は進める必要がある。
                // 相手の必殺技でロックされている場合は自分の演出は非アクティブなので実質何もしない
                combat_->UpdateSkillCutscene();

                // 溜め演出のエミッタが emit フラグ残留で消えなくなるのを防ぐため、
                // ロック中も更新自体は継続する（入力は上のロックで無効化済み）
                combat_->UpdateChargeShot();

                // 必殺技モーションを反映
                visual_->UpdateAnimation();

                // 飛行リーンの傾きを直立へ滑らかに戻す（放置すると傾いたまま固まり、
                // 必殺技モーションや死亡時の倒れる向きがずれる）
                visual_->UpdateFlyLean();
            }
            else if (status_->IsHitStun())
            {
                // ─── ひるみ（ヒットスタン）中は行動不能 ───
                // 攻撃・移動入力・ステート更新は行わない。ノックバック（速度）と重力・
                // 被弾点滅は下の共通処理で継続するため、アニメーションだけ進めて硬直を表現する。
                // 入力減速が効かないので、横滑りが伸びすぎないよう水平速度を減衰させる
                Hagine::Vector3 &vel = movement_->GetVelocity();
                vel.x *= kHitStunHorizontalDamping;
                vel.z *= kHitStunHorizontalDamping;
                visual_->UpdateAnimation();
                visual_->UpdateFlyLean();
            }
            else if (combat_->IsTeleporting())
            {
                // ─── 瞬間移動追撃中 ───
                // 敵に貼り付いて追撃を継続する。移動入力・射撃・ガードは無効化し、
                // コンボと前方判定だけ進めてフィニッシュ（叩きつけ）まで繋ぐ
                combat_->UpdateTeleport(dt_);      // 位置固定・向き・消える演出・カメラ制御
                combat_->UpdateComboAndCollider(); // コンボ継続＋前方判定更新
                visual_->UpdateAnimation();
                visual_->UpdateFlyLean();
            }
            else
            {
                combat_->UpdateComboAndCollider();

                movement_->UpdateDashState();

                UpdateGuardInput();

                if (pCurrentState_)
                {
                    pCurrentState_->Update(*this);
                }

                visual_->UpdateAnimation(); // ステート・コンボに応じてアニメーションを切り替え

                // RotateUpdate はロックオン追従と手動スティック回転を行う
                // Rush ステートは自前で UpdateRotation() を持つため除外する
                if (pCurrentState_ != states_["Rush"].get())
                {
                    movement_->RotateUpdate();
                }

                // 飛行移動中の体の傾き（描画専用）を更新する。RotateUpdate 後の向きを基準にする
                visual_->UpdateFlyLean();

                combat_->UpdateChargeShot();

                // 必殺技の発動前演出（カメラ顔アップ→遅延→発動）の進行
                combat_->UpdateSkillCutscene();

                combat_->Shot();

                combat_->SkillShot();
            }

            wasSkillLocked_ = skillLocked;
        }

        // ダメージリアクション処理（高速点滅のみ）
        status_->UpdateDamageReact();

        if (movement_->GetIsDashing())
        {
            targetFov_ = kDashingFov;
        }
        else
        {
            targetFov_ = kNormalFov;
        }

        // 現在のFOVを滑らかに補間してカメラに適用
        currentFov_ += (targetFov_ - currentFov_) * fovLerpSpeed_ * dt_;
        FollowCamera_->SetCameraFov(currentFov_);

        // 落下速度の制限と接地判定・位置更新
        movement_->CollisionGround();

        BaseObject::Update();

        shake_->Update();

        auraEmitter_->SetTranslate({GetWorldPosition().x, GetWorldPosition().y + auraEmitter_->GetScale().y, GetWorldPosition().z});
        auraEmitter_->SetRotation(-GetWorldRotation());
        auraEmitter_->Update();
        auraEmitter_->SetAuto(combat_->IsCharging());

        if (status_->GetHP() <= 0.0f)
        {
            if (isAlive_)
            {
                // ダメージリアクション中なら即座に終了させる
                status_->StopDamageReact();

                // 飛行リーンの描画回転オフセットが残っていると、死亡アニメーションの
                // 倒れる向きが本来の向き（quateRotation_）からずれ、粒子化演出と合わなくなる
                ClearRenderRotationOffset();

                // 空中（飛行）中の死亡でピッチが残っていると、倒れるモーションや
                // die.obj の粒子化演出が地面と合わない。ヨーのみ残して直立へ戻す
                Vector3 euler = transform_->quateRotation_.ToEulerAngles();
                transform_->quateRotation_ = Quaternion::FromEulerAngles({0.0f, euler.y, 0.0f});

                // 死亡後は水平移動を止め、その場で真下に落下させる
                movement_->GetVelocity().x = 0.0f;
                movement_->GetVelocity().z = 0.0f;
            }
            isAlive_ = false;
        }
    }

    if (isPause_)
    {
        movement_->GetVelocity() = {0.0f, 0.0f, 0.0f};
    }
    else
    {
        combat_->SetSkillMenu(gamePad_->GetLeftTrigger() > 0.25f);
    }
}

void Player::Draw(const ViewProjection &viewProjection)
{
    if (deathStaging_->GetIsStart())
    {
        return;
    }
    BaseObject::Draw(viewProjection);
    combat_->Draw(viewProjection);
}

void Player::DrawParticleCompute(const ViewProjection &viewProjection)
{
    auraEmitter_->DrawCompute(viewProjection);
    dashEffect_->DrawCompute(viewProjection);
    combat_->DrawParticleCompute(viewProjection);
}

void Player::DrawParticle(const ViewProjection &viewProjection)
{

    // 死亡アニメーションを再生し終わったら、死亡ポーズメッシュ(die.obj)の
    // 表面からパーティクルを発生させて粒子化して消える演出を行う
    if (!isAlive_ && isDeathStaging_ && visual_->IsDeathAnimationFinished())
    {
        deathStaging_->Initialize(
            GetWorldPosition() + Vector3(0.0f, kModelOffsetY, 0.0f),
            GetWorldRotation(), GetWorldScale(), GetColor());
        deathStaging_->Update();
        deathStaging_->Draw(viewProjection);
    }

    combat_->DrawChargeParticle(viewProjection);
    auraEmitter_->DrawGraphics(viewProjection);
    dashEffect_->DrawGraphics(viewProjection);
    hitEmitter_->Draw(viewProjection); // CPU emitter

    combat_->DrawAttackParticles(viewProjection);

    // 全ステートのパーティクル描画を呼び出す
    for (const auto &[name, state] : states_)
    {
        if (state)
        {
            state->DrawParticle(*this, viewProjection);
        }
    }
}

void Player::ChangeState(const std::string &stateName)
{
    auto it = states_.find(stateName);
    if (it != states_.end())
    {
        previousStateName_ = GetCurrentStateName();
        if (pCurrentState_)
        {
            pCurrentState_->Exit(*this);
        }
        pCurrentState_ = it->second.get();
        pCurrentState_->Enter(*this);

        // 入力表示UI用：ステート遷移で実際に発動したアクションを通知
        if (stateName == "Rush")
        {
            EmitAction(ActionKind::Rush);
        }
        else if (stateName == "Guard")
        {
            EmitAction(ActionKind::Guard);
        }
        else if (stateName == "EnergyCharge")
        {
            EmitAction(ActionKind::EnergyCharge);
        }
    }
}

void Player::OnCollision(ColliderBase *other)
{
    Vector3 &velocity = movement_->GetVelocity();

    // フィールド円柱との押し戻し
    if (other->GetTag() == "CylinderField")
    {
        if (other->GetType() != ColliderType::Cylinder)
        {
            return;
        }
        auto *cyl = static_cast<CylinderCollider *>(other);
        Vector3 mtv;
        if (CollisionManager::GetInstance()->CalculateDepenetrationOBBCylinder(pPlayerCollider_, cyl, mtv))
        {
            transform_->translation_ += mtv;
            Vector3 mtvDir = mtv.Normalize();
            float dot = velocity.Dot(mtvDir);
            if (dot < 0.0f)
            {
                velocity -= mtvDir * dot;
            }
        }
        return;
    }

    // PlayerWall との押し戻し（AABB）
    if (other->GetType() != ColliderType::AABB)
    {
        return;
    }
    auto *otherAABB = static_cast<AABBCollider *>(other);
    Vector3 mtv;
    if (CollisionManager::GetInstance()->CalculateDepenetration(pPlayerWallCollider_, otherAABB, mtv))
    {
        if (mtv.Length() < 0.0001f)
        {
            return;
        }
        transform_->translation_ += mtv;
        Vector3 mtvDir = mtv.Normalize();
        float dot = velocity.Dot(mtvDir);
        if (dot < 0.0f)
        {
            velocity -= mtvDir * dot;
        }
    }
}

void Player::OnCollisionEnter(ColliderBase *other)
{
    if (other->GetTag() == "EnemyBullet")
    {
        hitEmitter_->SetPosition(transform_->translation_);
        hitEmitter_->UpdateOnce();
    }
}

void Player::Debug()
{
#ifdef USE_IMGUI
    if (ImGui::BeginTabBar("プレイヤー"))
    {
        if (ImGui::BeginTabItem("プレイヤー"))
        {

            status_->DrawImGui();

            ImGui::Separator();
            ImGui::Text("Current State: %s", GetCurrentStateName().c_str());
            ImGui::Text("IsLockOn: %s", isLockOn_ ? "True" : "False");
            movement_->DrawImGui();

            ImGui::Text("現在位置: X=%.2f, Y=%.2f, Z=%.2f",
                        GetLocalPosition().x, GetLocalPosition().y, GetLocalPosition().z);
            ImGui::Text("現在角度(クォータニオン): X=%.2f, Y=%.2f, Z=%.2f, W=%.2f",
                        GetLocalRotation().x, GetLocalRotation().y, GetLocalRotation().z, GetLocalRotation().w);
            ImGui::Text("現在角度(オイラー): X=%.2f, Y=%.2f, Z=%.2f",
                        GetLocalRotation().ToEulerAngles().x, GetLocalRotation().ToEulerAngles().y, GetLocalRotation().ToEulerAngles().z);

            combat_->DrawBulletImGui();

            if (ImGui::Button("セーブ"))
            {
                Save();
            }

            if (ImGui::TreeNode("操作説明"))
            {
                ImGui::Text("WASD : 移動");
                if (pCurrentState_ != states_["FlyMove"].get() || pCurrentState_ != states_["FlyIdle"].get())
                {
                    ImGui::Text("SPACE : ジャンプ");
                    ImGui::Text("空中でSPACE : 浮遊");
                }
                else
                {
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
    if (!isAlive_)
    {
        deathStaging_->imgui();
    }

    // コンボ・必殺技関連
    combat_->DrawImGui();

    // アニメーション・飛行リーン関連
    visual_->DrawImGui([this] { Save(); });

#endif // USE_IMGUI
}

void Player::TriggerScreenFlash()
{
    if (screenFlash_)
    {
        screenFlash_->Trigger();
    }
}

void Player::ChangeEnergyCharge()
{
    // EnergyCharge ステート内からは呼ばれないため、二重遷移防止のみチェック
    if (pCurrentState_ == states_["EnergyCharge"].get())
    {
        return;
    }
    if (status_->GetEnergy() >= status_->GetMaxEnergy())
    {
        return;
    }

    if (!gamePad_->IsConnected())
    {
        // キーボード入力
        if (pInput_->TriggerKey(DIK_C))
        {
            ChangeState("EnergyCharge");
        }
    }
    else
    {
        // ゲームパッド入力 - LT のエッジ検出でチャージ状態に遷移
        bool isRTPressed = gamePad_->GetLeftTrigger() > 0.25f;

        if (isRTPressed && !wasRTPressed_ &&
            !gamePad_->IsPress(XINPUT_GAMEPAD_Y) &&
            !gamePad_->IsPress(XINPUT_GAMEPAD_A))
        {
            ChangeState("EnergyCharge");
        }

        wasRTPressed_ = isRTPressed;
    }
}

void Player::UpdateGuardInput()
{
    // すでにガード中なら遷移判定は Guard ステート側に任せる
    if (pCurrentState_ == states_["Guard"].get())
    {
        return;
    }

    // スキル発動中・発動前演出中・スキルメニュー中はガード不可
    if (combat_->GetIsSkillMenu() || combat_->IsSkillActive() || combat_->IsSkillStaging())
    {
        return;
    }

    // 地上(Idle/Move)・飛行(FlyIdle/FlyMove)からのみガードへ入れる
    bool canGuard =
        pCurrentState_ == states_["Idle"].get() ||
        pCurrentState_ == states_["Move"].get() ||
        pCurrentState_ == states_["FlyIdle"].get() ||
        pCurrentState_ == states_["FlyMove"].get();
    if (!canGuard)
    {
        return;
    }

    // エネルギーが被弾消費量未満ならガードに入れない（無くなった/支払えない場合は不可）
    if (!CanGuard())
    {
        return;
    }

    // 近接コンボ中・チャージショット溜め中はガードに入れない（攻撃とガードは排他）
    if (combat_->GetPunchCombo().IsComboActive() || combat_->IsCharging())
    {
        return;
    }

    bool guardHeld = gamePad_->IsPress(XINPUT_GAMEPAD_B) || pInput_->PushKey(DIK_RSHIFT);
    if (guardHeld)
    {
        ChangeState("Guard");
    }
}

std::string Player::GetCurrentStateName() const
{
    for (const auto &pair : states_)
    {
        if (pair.second.get() == pCurrentState_)
        {
            return pair.first;
        }
    }
    return "Unknown";
}

void Player::Save()
{
    movement_->Save(data_.get());
    combat_->Save(data_.get());
    status_->Save(data_.get());
    visual_->Save(data_.get());
    ImGuiNotification::Post("プレイヤー設定を保存しました", {0.2f, 0.8f, 0.2f, 1.0f});
}

void Player::Load()
{
    movement_->Load(data_.get());
    combat_->Load(data_.get());
    status_->Load(data_.get());
    visual_->Load(data_.get());
    ImGuiNotification::Post("プレイヤー設定を読み込みました", {0.2f, 0.8f, 0.8f, 1.0f});
}

Vector3 Player::GetForward() const { return -GetBackward(); }

Vector3 Player::GetBackward() const
{
    // クォータニオンから前方向ベクトルを計算（Z軸の負方向が前方向）
    return TransformNormal(Vector3(kForwardVectorX, kForwardVectorY, kForwardVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Player::GetRight() const
{
    // クォータニオンから右方向ベクトルを計算（X軸の正方向が右方向）
    return TransformNormal(Vector3(kRightVectorX, kRightVectorY, kRightVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Player::GetLeft() const { return -GetRight(); }

Vector3 Player::GetUp() const
{
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

void Player::SetVp(ViewProjection *vp)
{
    pVp_ = vp;
    shake_->Initialize(pVp_);
}

void Player::SetEnemy(Enemy *enemy)
{
    pEnemy_ = enemy;
    if (combat_->GetAttackCollider())
    {
        combat_->GetAttackCollider()->SetEnemy(enemy);
    }
}

void Player::SetTutorialStep(TutorialStep step)
{
    // EnergyCharge ステップに切り替わった瞬間だけリセット処理を行う
    if (step == TutorialStep::EnergyCharge &&
        tutorialStep_ != TutorialStep::EnergyCharge)
    {
        status_->ResetEnergyForTutorial();
    }

    tutorialStep_ = step;
}
