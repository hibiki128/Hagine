#define NOMINMAX
#include "PlayerCombat.h"
#include "Application/camera/follow/FollowCamera.h"
#include "Application/entity/enemy/Enemy.h"
#include "Application/entity/player/bullet/charge/ChargeShot.h"
#include "Application/entity/player/Player.h"
#include "object/base/BaseObjectManager.h"
#include <data/DataHandler.h>
#include <utility/debug/param/GameParamHub.h>

using namespace Hagine;

PlayerCombat::PlayerCombat()
{
}

PlayerCombat::~PlayerCombat()
{
}

void PlayerCombat::Init(Player *pOwner)
{
    pOwner_ = pOwner;

    chargeShot_ = std::make_unique<ChargeShot>();
    chargeShot_->SetPlayer(pOwner_);
    chargeShot_->Init("chargeShot");

    // 必殺技はBaseObjectManagerが所有し、こちらは生ポインタで操作する
    auto makanAttack = std::make_unique<MakanAttackSkill>();
    makanAttack->Init("makanAttack");
    pMakanAttack_ = makanAttack.get();
    BaseObjectManager::GetInstance()->AddObject(std::move(makanAttack));

    if (!comboInitialized_)
    {
        // 段の並び・ダメージ・ノックバック・判定タイミング・アニメーションはすべて
        // jsons/ComboDefinition/PunchCombo.json が持つ。
        // 見た目は本体アニメーションで再生するのでモーション用ターゲットは指定しない（nullptr）
        punchCombo_.LoadDefinition(kComboDefinitionName);
        comboInitialized_ = true;
    }

    attackCollider_ = std::make_unique<PlayerAttackCollider>();
    attackCollider_->Init(pOwner_);

    // コンボ派生技。連射フェーズはこちらの弾生成を借りる（エネルギーは発動時に一括で消費済み）
    finisher_.Init(pOwner_);
    finisher_.SetFireBulletCallback([this] { FireNormalBullet(true); });

    // 顔アップ＋発動遅延の合計を必殺技モーションの発射キーフレーム（30フレーム目≒1.0秒）に合わせる
    skillCutscene_.GetCloseUpDuration() = 0.6f;
    skillCutscene_.GetActivationDelay() = 0.4f;

    // コンボが攻撃を発火したとき attackCollider_ を有効化するコールバックを登録
    punchCombo_.SetOnAttackFired(
        [this](float damage, float knockback, float duration, float delay) {
            if (attackCollider_)
            {
                // 段番号（GetCurrentComboIndex() は実行中の段の0始まりインデックス）
                const int stage = punchCombo_.GetCurrentComboIndex() + 1;

                // 瞬間移動コンボ：特定段のヒット挙動を切り替える。
                // Launch/Chase の吹き飛ばし速度は TeleportAhead 側で設定するのでkbは使わない
                MeleeHitReaction reaction = MeleeHitReaction::Normal;
                float kb = knockback;
                if (teleportEnabled_)
                {
                    if (stage == teleportSlamStage_)
                    {
                        reaction = MeleeHitReaction::Slam; // 下へ叩きつけ＋追撃終了
                        kb = teleportSlamKnockback_;
                    }
                    else if (stage == teleportLaunchStage_ &&
                             pOwner_->GetEnergy() >= teleportEnergyCost_)
                    {
                        // エネルギーが足りるときだけ大吹き飛ばし＋瞬間移動開始
                        reaction = MeleeHitReaction::Launch;
                        kb = 0.0f;
                    }
                    else if (stage > teleportLaunchStage_ && stage < teleportSlamStage_)
                    {
                        reaction = MeleeHitReaction::Chase; // 吹き飛ばし＋瞬間移動継続
                        kb = 0.0f;
                    }
                }

                attackCollider_->Activate(damage, kb, duration, delay, reaction);
            }
            // 攻撃のたびに少し前へ踏み込む（その場で殴ると当てづらいため）
            pOwner_->Movement().StartMeleeLunge();

            // 入力表示UI用: 実際に発火した近接攻撃の段名を記録する
            // （先行入力バッファ経由の発火もここを通るため取りこぼしがない）
            meleeAttackFired_ = true;
            lastMeleeAttackName_ = punchCombo_.GetCurrentAttackName();
        });
}

void PlayerCombat::UpdateCombo()
{
    ComboUpdate();
}

void PlayerCombat::UpdateAttackCollider()
{
    if (attackCollider_)
    {
        attackCollider_->Update(pOwner_->GetDt());
    }
}

void PlayerCombat::TeleportAhead()
{
    Enemy *pEnemy = pOwner_->GetEnemy();
    if (!pEnemy)
    {
        return;
    }

    // 吹き飛ばし方向 = プレイヤーが今向いている方向（水平）
    Hagine::Vector3 dir = pOwner_->GetForward();
    dir.y = 0.0f;
    if (dir.Length() > 0.001f)
    {
        dir = dir.Normalize();
    }
    else
    {
        dir = teleportDir_;
    }
    teleportDir_ = dir;

    const Hagine::Vector3 enemyPos = pEnemy->GetWorldPosition();

    // arrivalTime 秒で aheadDistance を進む速度にして、先回り地点へ丁度到達させる
    const float arrival = (teleportArrivalTime_ > 0.001f) ? teleportArrivalTime_ : 0.001f;
    const float speed = teleportAheadDistance_ / arrival;
    pEnemy->SetVelocity({dir.x * speed, teleportLaunchUp_, dir.z * speed});
    pEnemy->Movement().CancelVelocityEase(); // BTの速度イージングに吹き飛ばし速度を上書きされないように

    // プレイヤーは吹き飛ぶ方向の先へ先回り瞬間移動する。敵はこの地点へ吹き飛ばされてくる
    Hagine::Vector3 aheadPos = enemyPos + dir * teleportAheadDistance_;
    aheadPos.y = enemyPos.y;
    pOwner_->GetLocalPosition() = aheadPos;
    pOwner_->GetVelocity() = {0.0f, 0.0f, 0.0f};
    pOwner_->GetIsGrounded() = false;
    pOwner_->Movement().FaceTargetInstant(enemyPos); // 迎え撃つように敵の方を向く

    // 消える演出をやり直す
    teleportVanishTimer_ = 0.0f;

    // カメラを一瞬だけ旧位置に留めてから、瞬間移動先のプレイヤーへスナップさせる
    if (pOwner_->GetCamera())
    {
        pOwner_->GetCamera()->HoldThenSnap(teleportCameraHold_);
    }
}

void PlayerCombat::StartTeleportChase()
{
    if (!teleportEnabled_)
    {
        return;
    }
    Enemy *pEnemy = pOwner_->GetEnemy();
    if (!pEnemy || !pEnemy->GetAlive())
    {
        return;
    }
    // 瞬間移動には少量のエネルギーを消費する。足りなければ追撃しない
    if (!pOwner_->ConsumeEnergy(teleportEnergyCost_))
    {
        return;
    }

    teleportActive_ = true;
    teleportTimer_ = 0.0f;
    TeleportAhead(); // 吹き飛ばし＋先回り＋カメラのホールドスナップを行う
}

void PlayerCombat::RefreshTeleportChase()
{
    // 追撃段のヒットで再度大きく吹き飛ばし、その先へ先回りし直す
    if (!teleportActive_)
    {
        return;
    }
    teleportTimer_ = 0.0f;
    TeleportAhead();
}

void PlayerCombat::EndTeleportChase()
{
    if (!teleportActive_)
    {
        return;
    }
    teleportActive_ = false;
    pOwner_->SetAlpha(1.0f); // 不透明へ戻す（消える演出の解除）

    // 空中に取り残されたら落下ステートへ移し、棒立ちで浮かないようにする
    if (!pOwner_->GetIsGrounded())
    {
        pOwner_->ChangeState("Air");
    }
}

void PlayerCombat::UpdateTeleport(float deltaTime)
{
    if (!teleportActive_)
    {
        return;
    }

    Enemy *pEnemy = pOwner_->GetEnemy();
    // 敵が死亡/消滅、またはコンボが途切れたら追撃を終了する
    if (!pEnemy || !pEnemy->GetAlive() || !punchCombo_.IsComboActive())
    {
        EndTeleportChase();
        return;
    }

    teleportTimer_ += deltaTime;
    if (teleportTimer_ >= teleportPinDuration_)
    {
        EndTeleportChase();
        return;
    }

    // 先回り地点で待機（動かない）。迎え撃つように敵の方を向き続ける
    pOwner_->GetVelocity() = {0.0f, 0.0f, 0.0f};
    pOwner_->GetIsGrounded() = false;
    pOwner_->Movement().FaceTargetInstant(pEnemy->GetWorldPosition());

    // 敵が攻撃間合いまで到達したら、通り過ぎないよう滑走を止めてその場に留める
    Hagine::Vector3 toEnemy = pEnemy->GetWorldPosition() - pOwner_->GetWorldPosition();
    toEnemy.y = 0.0f;
    if (toEnemy.Length() <= teleportFollowDistance_)
    {
        Hagine::Vector3 vel = pEnemy->GetVelocity();
        pEnemy->SetVelocity({0.0f, vel.y, 0.0f});
    }

    // 消える演出：瞬間移動直後だけ薄く表示→通常へ戻す
    teleportVanishTimer_ += deltaTime;
    if (teleportVanishTimer_ < teleportVanishDuration_)
    {
        const float t = teleportVanishTimer_ / teleportVanishDuration_;
        pOwner_->SetAlpha(0.1f + 0.9f * t);
    }
    else
    {
        pOwner_->SetAlpha(1.0f);
    }
}

void PlayerCombat::ComboUpdate()
{
    // ガード中・必殺技演出中・射撃モーション中は近接コンボを実行できない（射撃との排他）
    if (pOwner_->GetCurrentStateName() != "EnergyCharge" && !IsCharging() &&
        !pOwner_->IsGuarding() && !IsSkillStaging() &&
        !pOwner_->Visual().IsShotAnimationPlaying())
    {
        punchCombo_.Update(pOwner_->GetDt());

        if (!pOwner_->GetGamePad()->IsConnected())
        {
            // キーボード入力
            if (pOwner_->GetInput()->TriggerKey(DIK_H))
            {
                punchCombo_.TryExecuteCombo();
            }
        }
        else
        {
            // ゲームパッド入力
            if (pOwner_->GetGamePad()->IsTrigger(XINPUT_GAMEPAD_X))
            {
                punchCombo_.TryExecuteCombo();
            }
        }
    }
}

void PlayerCombat::UpdateChargeShot()
{
    if (!chargeShot_)
    {
        return;
    }
    chargeShot_->SetIsSkillMenu(isSkillMenu_);
    chargeShot_->Update();

    // 入力表示UI用：チャージ開始（溜め始め）とチャージ弾発射を通知
    bool nowCharge = chargeShot_->GetIsCharge();
    if (nowCharge && !prevChargeState_)
    {
        pOwner_->EmitAction(Player::ActionKind::ChargeStart);
    }
    prevChargeState_ = nowCharge;
    if (chargeShot_->ConsumeFired())
    {
        pOwner_->EmitAction(Player::ActionKind::ChargeShot);
    }
}

void PlayerCombat::UpdateSkillCutscene()
{
    skillCutscene_.Update(pOwner_->GetDt());
}

bool PlayerCombat::IsCharging() const
{
    return chargeShot_ && chargeShot_->GetIsCharge();
}

void PlayerCombat::SetChargeActionLocked(bool locked)
{
    if (chargeShot_)
    {
        chargeShot_->SetActionLocked(locked);
    }
}

void PlayerCombat::FireNormalBullet(bool forceHoming)
{
    std::string bulletName = "PlayerBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<PlayerBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(pOwner_, forceHoming);
    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);
    bullets_.push_back(std::move(bullet));

    pOwner_->Visual().PlayShotAnimation(); // 発射モーション（連射時は毎回先頭から再生し直す）

    pOwner_->EmitAction(Player::ActionKind::NormalShot); // 入力表示UI用：通常射撃を通知
}

bool PlayerCombat::IsShotTriggered()
{
    if (!pOwner_->GetGamePad()->IsConnected())
    {
        // キーボード入力
        return pOwner_->GetInput()->TriggerKey(DIK_J);
    }

    // ゲームパッド入力 - Yボタンの押下時間を計測
    if (pOwner_->GetGamePad()->IsPress(XINPUT_GAMEPAD_Y) && !isSkillMenu_)
    {
        yButtonHoldTime_ += pOwner_->GetDt();
    }

    bool triggered = false;

    // Yボタンが離された瞬間、長押し判定閾値未満なら射撃入力として扱う
    if (pOwner_->GetGamePad()->IsRelease(XINPUT_GAMEPAD_Y) && !isSkillMenu_)
    {
        triggered = (yButtonHoldTime_ < kYButtonChargeThreshold);
        yButtonHoldTime_ = 0.0f; // 押下時間をリセット
    }

    // Yボタンが押されていない時はタイマーをリセット
    if (!pOwner_->GetGamePad()->IsPress(XINPUT_GAMEPAD_Y))
    {
        yButtonHoldTime_ = 0.0f;
    }

    return triggered;
}

void PlayerCombat::HandleShotInput()
{
    // 近接コンボ中の射撃入力は、通常弾ではなく段数に応じたコンボ派生技へ振り分ける。
    // 段数不足・エネルギー不足・クールダウン中などで出せなければ何も起きない
    // （コンボ中に通常弾が漏れて出ると、近接と射撃の排他が崩れるため）
    if (punchCombo_.IsComboActive())
    {
        if (finisher_.TryStart(punchCombo_.GetExecutedStepCount()))
        {
            // 派生技へ移行するので、コンボ・出しかけの攻撃判定・瞬間移動追撃は畳んでおく
            punchCombo_.Interrupt();
            if (attackCollider_)
            {
                attackCollider_->Deactivate();
            }
            EndTeleportChase();
        }
        return;
    }

    if (pOwner_->ConsumeEnergy(kNormalShotEnergyCost))
    {
        FireNormalBullet();
    }
}

void PlayerCombat::Shot()
{
    // ガード中・必殺技演出中は発射できない（既存弾の更新は下で継続する）。
    // 近接コンボ中はコンボ派生技の入力として受け付ける
    if (pOwner_->GetCurrentStateName() != "EnergyCharge" && !isSkillMenu_ &&
        !pOwner_->IsGuarding() && !IsSkillStaging())
    {
        if (IsShotTriggered())
        {
            HandleShotInput();
        }
    }
}

void PlayerCombat::UpdateBullets()
{
    // 弾の更新と生存チェック。
    // 発射できない状況（被弾リアクション中・派生技の演出中など）でも飛んでいる弾は
    // 進み続ける必要があるため、射撃入力の処理とは切り離して毎フレーム呼ぶ
    for (auto it = bullets_.begin(); it != bullets_.end();)
    {
        (*it)->Update();
        (*it)->SetSpeed(bulletSpeed_);
        (*it)->SetAcce(bulletAcceleration_);
        (*it)->UpdateWorldTransformHierarchy();

        if (!(*it)->IsAlive())
        {
            it = bullets_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void PlayerCombat::SkillShot()
{
    // ガード中は必殺技を発動できない
    if (pOwner_->IsGuarding())
    {
        return;
    }
    // チャージショット溜め中は発動できない
    if (IsCharging())
    {
        return;
    }

    // 発動入力の判定
    bool triggered = false;
    if (!pOwner_->GetGamePad()->IsConnected())
    {
        // キーボード入力
        triggered = pOwner_->GetInput()->TriggerKey(DIK_G);
    }
    else
    {
        // ゲームパッド入力（スキルメニュー中のYボタン）
        triggered = isSkillMenu_ && pOwner_->GetGamePad()->IsTrigger(XINPUT_GAMEPAD_Y);
    }
    if (!triggered)
    {
        return;
    }

    // 既に発動中・演出中なら何もしない
    if (!pMakanAttack_ || pMakanAttack_->IsActive() || skillCutscene_.IsActive())
    {
        return;
    }
    if (!pOwner_->ConsumeEnergy(kSkillShotEnergyCost))
    {
        return; // エネルギー不足なら発射しない
    }

    StartSkillStaging();
}

void PlayerCombat::StartSkillStaging()
{
    pMakanAttack_->SetPlayer(pOwner_);

    // 入力の時点では撃たず、カメラ顔アップ演出→通常カメラ復帰→遅延の後に発動する。
    // 遅延中も照準追従は生きており、発動の瞬間の向きで固定される
    skillCutscene_.Start(pOwner_, pOwner_->GetCamera(), [this] {
        pMakanAttack_->Activate(pOwner_->GetTransformPtr());
        pOwner_->EmitAction(Player::ActionKind::Special); // 入力表示UI用：必殺技を通知
        pOwner_->TriggerScreenFlash();                    // 画面白黒＆ブルームのフラッシュ演出
    });
}

void PlayerCombat::Draw(const ViewProjection &viewProjection)
{
    for (auto &bullet : bullets_)
    {
        bullet->Draw(viewProjection);
    }
    chargeShot_->Draw(viewProjection);
}

void PlayerCombat::DrawChargeParticle(const ViewProjection &viewProjection)
{
    chargeShot_->DrawParticle(viewProjection);
}

void PlayerCombat::DrawAttackParticles(const ViewProjection &viewProjection)
{
    for (auto &bullet : bullets_)
    {
        bullet->DrawParticle(viewProjection);
    }

    if (attackCollider_)
    {
        attackCollider_->DrawParticle(viewProjection);
    }

    // 必殺技ビーム（pMakanAttack_ptr_）は GPU パーティクルで、描画はエンジンが自動で行う。
}

void PlayerCombat::Save(DataHandler *pData)
{
    pData->Save("bulletSpeed", bulletSpeed_);
    pData->Save("bulletAcce", bulletAcceleration_);

    // 瞬間移動コンボ
    pData->Save("tpEnabled", teleportEnabled_);
    pData->Save("tpLaunchStage", teleportLaunchStage_);
    pData->Save("tpSlamStage", teleportSlamStage_);
    pData->Save("tpEnergyCost", teleportEnergyCost_);
    pData->Save("tpAheadDistance", teleportAheadDistance_);
    pData->Save("tpArrivalTime", teleportArrivalTime_);
    pData->Save("tpLaunchUp", teleportLaunchUp_);
    pData->Save("tpSlamKnockback", teleportSlamKnockback_);
    pData->Save("tpFollowDistance", teleportFollowDistance_);
    pData->Save("tpPinDuration", teleportPinDuration_);
    pData->Save("tpVanishDuration", teleportVanishDuration_);
    pData->Save("tpCameraHold", teleportCameraHold_);

    // コンボ派生技
    finisher_.Save(pData);
}

void PlayerCombat::Load(DataHandler *pData)
{
    bulletSpeed_ = pData->Load<float>("bulletSpeed", 60.0f);
    bulletAcceleration_ = pData->Load<float>("bulletAcce", 5.0f);

    // 瞬間移動コンボ
    teleportEnabled_ = pData->Load<bool>("tpEnabled", true);
    teleportLaunchStage_ = pData->Load<int>("tpLaunchStage", 5);
    teleportSlamStage_ = pData->Load<int>("tpSlamStage", 8);
    teleportEnergyCost_ = pData->Load<float>("tpEnergyCost", 8.0f);
    teleportAheadDistance_ = pData->Load<float>("tpAheadDistance", 12.0f);
    teleportArrivalTime_ = pData->Load<float>("tpArrivalTime", 0.4f);
    teleportLaunchUp_ = pData->Load<float>("tpLaunchUp", 2.0f);
    teleportSlamKnockback_ = pData->Load<float>("tpSlamKnockback", 45.0f);
    teleportFollowDistance_ = pData->Load<float>("tpFollowDistance", 3.0f);
    teleportPinDuration_ = pData->Load<float>("tpPinDuration", 0.9f);
    teleportVanishDuration_ = pData->Load<float>("tpVanishDuration", 0.15f);
    teleportCameraHold_ = pData->Load<float>("tpCameraHold", 0.3f);

    // コンボ派生技
    finisher_.Load(pData);
}

void PlayerCombat::DrawBulletImGui()
{
#ifdef USE_IMGUI
    ImGui::DragFloat("弾の速度", &bulletSpeed_, 0.1f);
    ImGui::DragFloat("弾の加速度", &bulletAcceleration_, 0.1f);
#endif // USE_IMGUI
}

void PlayerCombat::DrawImGui()
{
#ifdef USE_IMGUI
    if (pMakanAttack_)
    {
        pMakanAttack_->DrawImGui();
    }

    if (ImGui::CollapsingHeader("コンボパラメータ"))
    {
        punchCombo_.DrawImGui();
    }

    // ─── コンボ派生技（近接コンボ中の射撃入力で出る技）───
    finisher_.DrawImGui();

    // ─── コンボアニメーション割り当て ───
    // 段のラベル・パスはコンボ定義（JSON）から取得する。編集結果はコンボパラメータの
    // 「コンボ定義を保存」で同じJSONへ書き戻される
    if (ImGui::CollapsingHeader("コンボアニメーション割り当て"))
    {
        ImGui::TextDisabled("空白のままにすると、その段はアニメーションを変更しません");
        ImGui::Spacing();

        for (int i = 0; i < punchCombo_.GetComboLength(); ++i)
        {
            ImGui::PushID(i);

            const std::string stageLabel = std::to_string(i + 1) + "段目: " + punchCombo_.GetAttackName(i);

            // 現在実行中の段をハイライト
            bool isCurrentStage = punchCombo_.IsComboActive() &&
                                  ((punchCombo_.GetCurrentComboIndex() == 0
                                        ? punchCombo_.GetComboLength() - 1
                                        : punchCombo_.GetCurrentComboIndex() - 1) == i);
            if (isCurrentStage)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), ">>> %s", stageLabel.c_str());
            }
            else
            {
                ImGui::Text("%s", stageLabel.c_str());
            }

            char animationPathBuffer[kAnimationPathBufferSize] = {};
            snprintf(animationPathBuffer, sizeof(animationPathBuffer), "%s",
                     punchCombo_.GetAnimationPath(i).c_str());
            ImGui::SetNextItemWidth(380.0f);
            if (ImGui::InputText("##animPath", animationPathBuffer, sizeof(animationPathBuffer)))
            {
                punchCombo_.SetAnimationPath(i, animationPathBuffer);
            }

            ImGui::PopID();
            ImGui::Spacing();
        }
    }
#endif // USE_IMGUI
}

void PlayerCombat::RegisterParams()
{
    auto *pHub = GameParamHub::GetInstance();
    pHub->Register("Player", "弾の速度", &bulletSpeed_, {0.1f});
    pHub->Register("Player", "弾の加速度", &bulletAcceleration_, {0.1f});
    pHub->Register("Player", "コンボ出し切り後の硬直", &punchCombo_.GetFinishRecovery(), {0.05f, 0.0f, 3.0f});
    skillCutscene_.RegisterParams("必殺演出(Player)");

    // 瞬間移動コンボ（横吹き飛ばし→先回り瞬間移動→叩きつけ）
    const char *tp = "瞬間移動コンボ(Player)";
    pHub->Register(tp, "有効", &teleportEnabled_);
    pHub->Register(tp, "瞬間移動を始める段", &teleportLaunchStage_, {1.0f, 1.0f, 8.0f});
    pHub->Register(tp, "叩きつける最終段", &teleportSlamStage_, {1.0f, 1.0f, 8.0f});
    pHub->Register(tp, "消費エネルギー", &teleportEnergyCost_, {0.5f, 0.0f, 100.0f});
    pHub->Register(tp, "吹き飛ばし/先回り距離", &teleportAheadDistance_, {0.5f, 1.0f, 60.0f});
    pHub->Register(tp, "到達時間", &teleportArrivalTime_, {0.01f, 0.05f, 1.5f});
    pHub->Register(tp, "吹き飛ばし上方向成分", &teleportLaunchUp_, {0.1f, 0.0f, 30.0f});
    pHub->Register(tp, "叩きつけ威力", &teleportSlamKnockback_, {0.5f, 0.0f, 200.0f});
    pHub->Register(tp, "攻撃間合い(滑走停止距離)", &teleportFollowDistance_, {0.1f, 0.5f, 15.0f});
    pHub->Register(tp, "追撃保持時間(安全弁)", &teleportPinDuration_, {0.05f, 0.1f, 3.0f});
    pHub->Register(tp, "消える演出時間", &teleportVanishDuration_, {0.01f, 0.0f, 1.0f});
    pHub->Register(tp, "カメラ待機時間", &teleportCameraHold_, {0.01f, 0.0f, 2.0f});

    // コンボ派生技（近接コンボ中の射撃入力で出る技）
    finisher_.RegisterParams();
}
