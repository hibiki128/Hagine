#define NOMINMAX
#include "PlayerCombat.h"
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

void PlayerCombat::Init(Player *owner)
{
    pOwner_ = owner;

    chargeShot_ = std::make_unique<ChargeShot>();
    chargeShot_->SetPlayer(pOwner_);
    chargeShot_->Init("chargeShot");

    // 必殺技はBaseObjectManagerが所有し、こちらは生ポインタで操作する
    auto makanAttack = std::make_unique<MakanAttackSkill>();
    makanAttack->Init("makanAttack");
    pMakanAttack_ptr_ = makanAttack.get();
    BaseObjectManager::GetInstance()->AddObject(std::move(makanAttack));

    if (!comboInitialized_)
    {
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
        comboInitialized_ = true;
    }

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

    attackCollider_ = std::make_unique<PlayerAttackCollider>();
    attackCollider_->Init(pOwner_);

    // 必殺技モーション（MakanSkill.gltf・30fps）に演出の長さを合わせる。
    // 顔アップ＋発動遅延の合計 = 発射キーフレーム（30フレーム目 ≒ 1.0秒）
    skillCutscene_.GetCloseUpDuration() = 0.6f;
    skillCutscene_.GetActivationDelay() = 0.4f;

    // コンボが攻撃を発火したとき attackCollider_ を有効化するコールバックを登録
    punchCombo_.SetOnAttackFired(
        [this](float damage, float knockback, float duration, float delay) {
            if (attackCollider_)
            {
                attackCollider_->Activate(damage, knockback, duration, delay);
            }
            // 入力表示UI用: 実際に発火した近接攻撃の段名を記録する
            // （先行入力バッファ経由の発火もここを通るため取りこぼしがない）
            meleeAttackFired_ = true;
            lastMeleeAttackName_ = punchCombo_.GetCurrentAttackName();
        });
}

void PlayerCombat::UpdateComboAndCollider()
{
    ComboUpdate();
    if (attackCollider_)
    {
        attackCollider_->Update(pOwner_->GetDt());
    }
}

void PlayerCombat::ComboUpdate()
{
    // ガード中・必殺技演出中は近接コンボを実行できない
    if (pOwner_->GetCurrentStateName() != "EnergyCharge" && !IsCharging() &&
        !pOwner_->IsGuarding() && !IsSkillStaging())
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

void PlayerCombat::FireNormalBullet()
{
    std::string bulletName = "PlayerBullet_" + std::to_string(bullets_.size());
    auto bullet = std::make_unique<PlayerBullet>();
    bullet->Init(bulletName);
    bullet->InitTransform(pOwner_);
    bullet->GetLocalScale() = {kBulletScale, kBulletScale, kBulletScale};
    bullet->SetColliderRadius(kBulletColliderRadius);
    bullets_.push_back(std::move(bullet));

    pOwner_->Visual().PlayShotAnimation(); // 発射モーション（連射時は毎回先頭から再生し直す）

    pOwner_->EmitAction(Player::ActionKind::NormalShot); // 入力表示UI用：通常射撃を通知
}

void PlayerCombat::Shot()
{
    // ガード中・必殺技演出中は遠距離射撃を発射できない（既存弾の更新は下で継続する）
    if (pOwner_->GetCurrentStateName() != "EnergyCharge" && !isSkillMenu_ &&
        !pOwner_->IsGuarding() && !IsSkillStaging())
    {
        if (!pOwner_->GetGamePad()->IsConnected())
        {
            // キーボード入力
            if (pOwner_->GetInput()->TriggerKey(DIK_J))
            {
                if (pOwner_->ConsumeEnergy(kNormalShotEnergyCost))
                {
                    FireNormalBullet();
                }
            }
        }
        else
        {
            // ゲームパッド入力 - Yボタンの押下時間を計測
            if (pOwner_->GetGamePad()->IsPress(XINPUT_GAMEPAD_Y) && !isSkillMenu_)
            {
                yButtonHoldTime_ += pOwner_->GetDt();
            }

            // Yボタンが離された瞬間、長押し判定閾値未満なら通常弾を発射
            if (pOwner_->GetGamePad()->IsRelease(XINPUT_GAMEPAD_Y) && !isSkillMenu_)
            {
                if (yButtonHoldTime_ < kYButtonChargeThreshold)
                {
                    if (pOwner_->ConsumeEnergy(kNormalShotEnergyCost))
                    {
                        FireNormalBullet();
                    }
                }

                yButtonHoldTime_ = 0.0f; // 押下時間をリセット
            }

            // Yボタンが押されていない時はタイマーをリセット
            if (!pOwner_->GetGamePad()->IsPress(XINPUT_GAMEPAD_Y))
            {
                yButtonHoldTime_ = 0.0f;
            }
        }
    }

    // 弾の更新と生存チェック
    for (auto it = bullets_.begin(); it != bullets_.end();)
    {
        (*it)->Update();
        (*it)->SetSpeed(B_speed_);
        (*it)->SetAcce(B_acce_);
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
    if (!pMakanAttack_ptr_ || pMakanAttack_ptr_->IsActive() || skillCutscene_.IsActive())
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
    pMakanAttack_ptr_->SetPlayer(pOwner_);

    // 入力の時点では撃たず、カメラ顔アップ演出→通常カメラ復帰→遅延の後に発動する。
    // 遅延中はロックオンによる照準追従が生きており、発動の瞬間の向きで固定される
    // （MakanAttackSkill::Activate が向きをスナップショットする）
    skillCutscene_.Start(pOwner_, pOwner_->GetCamera(), [this] {
        pMakanAttack_ptr_->Activate(pOwner_->GetTransformPtr());
        pOwner_->EmitAction(Player::ActionKind::Special); // 入力表示UI用：必殺技を通知
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

    if (pMakanAttack_ptr_)
    {
        pMakanAttack_ptr_->DrawParticle(viewProjection);
    }
}

void PlayerCombat::DrawParticleCompute(const ViewProjection &viewProjection)
{
    chargeShot_->DrawParticleCompute(viewProjection);
}

void PlayerCombat::Save(DataHandler *data)
{
    data->Save("bulletSpeed", B_speed_);
    data->Save("bulletAcce", B_acce_);
}

void PlayerCombat::Load(DataHandler *data)
{
    B_speed_ = data->Load<float>("bulletSpeed", 60.0f);
    B_acce_ = data->Load<float>("bulletAcce", 5.0f);
}

void PlayerCombat::DrawBulletImGui()
{
#ifdef USE_IMGUI
    ImGui::DragFloat("弾の速度", &B_speed_, 0.1f);
    ImGui::DragFloat("弾の加速度", &B_acce_, 0.1f);
#endif // USE_IMGUI
}

void PlayerCombat::DrawImGui()
{
#ifdef USE_IMGUI
    if (pMakanAttack_ptr_)
    {
        pMakanAttack_ptr_->DebugImGui();
    }

    if (ImGui::CollapsingHeader("コンボパラメータ"))
    {
        punchCombo_.DrawImGui();
    }

    // ─── コンボアニメーション割り当て ───
    if (ImGui::CollapsingHeader("コンボアニメーション割り当て"))
    {
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

        for (int i = 0; i < static_cast<int>(comboAnimations_.size()); ++i)
        {
            ImGui::PushID(i);

            // 現在実行中の段をハイライト
            bool isCurrentStage = punchCombo_.IsComboActive() &&
                                  ((punchCombo_.GetCurrentComboIndex() == 0
                                        ? punchCombo_.GetComboLength() - 1
                                        : punchCombo_.GetCurrentComboIndex() - 1) == i);
            if (isCurrentStage)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), ">>> %s", kComboLabels[i]);
            }
            else
            {
                ImGui::Text("%s", kComboLabels[i]);
            }

            char buf[256] = {};
            snprintf(buf, sizeof(buf), "%s", comboAnimations_[i].c_str());
            ImGui::SetNextItemWidth(380.0f);
            if (ImGui::InputText("##animPath", buf, sizeof(buf)))
            {
                comboAnimations_[i] = buf;
            }

            ImGui::PopID();
            ImGui::Spacing();
        }
    }
#endif // USE_IMGUI
}

void PlayerCombat::RegisterParams()
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Player", "弾の速度", &B_speed_, {0.1f});
    hub->Register("Player", "弾の加速度", &B_acce_, {0.1f});
    skillCutscene_.RegisterParams("必殺演出(Player)");
}
