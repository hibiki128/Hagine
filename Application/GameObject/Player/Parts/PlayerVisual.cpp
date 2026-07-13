#define NOMINMAX
#include "PlayerVisual.h"
#include "Application/GameObject/Player/Parts/PlayerCombat.h"
#include "Application/GameObject/Player/Player.h"
#include <Data/DataHandler.h>
#include <Utility/Debug/GameParam/GameParamHub.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;

void PlayerVisual::Init(Player *owner)
{
    owner_ = owner;

    // ───────────────────────────────────────────
    // アニメーションコントローラへ全クリップを登録する
    // RegisterClip(識別名, ファイルパス, ループ, 速度, 補間時間)
    // ───────────────────────────────────────────
    animationController_.Initialize(owner_->GetObject3d());

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
}

void PlayerVisual::UpdateAnimation()
{
    ComboSystem &punchCombo = owner_->Combat().GetPunchCombo();
    const std::vector<std::string> &comboAnimations = owner_->Combat().GetComboAnimations();

    // ──────────────────────────────────────────
    // コンボ攻撃中：段数に対応したアニメーションを再生
    // ──────────────────────────────────────────
    if (punchCombo.IsComboActive())
    {
        // GetCurrentComboIndex() は「次に実行する」インデックスを返す。
        // ExecuteComboAttack() が呼ばれるとインクリメントされるため、
        // 現在再生中の段 = nextIdx - 1（0 のときは最終段 Slam の後待機中）
        int nextIdx = punchCombo.GetCurrentComboIndex();
        int comboLen = punchCombo.GetComboLength();
        int animIdx = (nextIdx == 0) ? (comboLen - 1) : (nextIdx - 1);

        if (animIdx >= 0 && animIdx < static_cast<int>(comboAnimations.size()))
        {
            const std::string &path = comboAnimations[animIdx];
            if (!path.empty())
            {
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
    const std::string stateName = owner_->GetCurrentStateName();

    if (stateName == "Idle")
    {
        // 地上待機
        animationController_.Play("Idle");
    }
    else if (stateName == "Move")
    {
        // 地上移動 ― 移動方向に応じて前後左右のクリップを使い分ける
        switch (owner_->GetMoveDirection())
        {
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
    }
    else if (stateName == "Jump" || stateName == "Air")
    {
        // ジャンプ・空中（上昇/滞空/下降）
        animationController_.Play("Jump");
    }
    else if (stateName == "FlyIdle")
    {
        // 浮遊待機
        animationController_.Play("FlyIdle");
    }
    else if (stateName == "FlyMove")
    {
        // 浮遊移動 ― 後退・上昇・下降は Idle_Flying、前進・左右移動は Running_Fly を使う。
        // 縦移動(|velocity.y| > 閾値)または後退(moveDir == Behind)は、仰向け気味の
        // Idle_Flying の方がモーションとして自然なため優先する
        bool verticalMove = std::abs(owner_->GetVelocity().y) > kFlyVerticalAnimThreshold;
        bool movingBackward = (owner_->GetMoveDirection() == MoveDirection::Behind);
        if (verticalMove || movingBackward)
        {
            animationController_.Play("FlyIdle");
        }
        else
        {
            animationController_.Play("FlyMove");
        }
    }
    else if (stateName == "Rush")
    {
        // ダッシュ突進
        animationController_.Play("FlyMove");
    }
    else if (stateName == "Guard")
    {
        // ガード
        animationController_.Play("Guard");
    }
    else if (stateName == "EnergyCharge")
    {
        // エネルギーチャージ ― 専用モーションなし（待機で代用）
        animationController_.Play("Idle");
    }
}

void PlayerVisual::UpdateFlyLean()
{
    // 目標姿勢をクォータニオンで構築する。条件を満たさないときは無回転（直立）へ戻す
    Quaternion targetLean = Quaternion::IdentityQuaternion();

    // ロックオン飛行移動中のみ傾ける。
    // 非ロックオン時は体が進行方向を向くため傾け不要、地上やその他ステートでも適用しない
    bool active = flyLeanEnabled_ && owner_->GetIsLockOn() &&
                  owner_->GetCurrentStateName() == "FlyMove";

    const Vector3 &velocity = owner_->GetVelocity();

    if (active)
    {
        Vector3 horizontalVel = {velocity.x, 0.0f, velocity.z};
        float speed = horizontalVel.Length();

        // 体の「水平」な前方・右方向（ロックオンの上下ピッチを除いた安定フレーム）。
        // 傾きはこの体の実ワールド軸まわりに作ることで、向きが変わっても
        // 「後ろへ倒れる」方向が一定になる（ワールド固定軸だと向きでズレる）
        Vector3 fwdAxis = {owner_->GetForward().x, 0.0f, owner_->GetForward().z};
        Vector3 rightAxis = {owner_->GetRight().x, 0.0f, owner_->GetRight().z};
        float fwdLen = fwdAxis.Length();
        float rightLen = rightAxis.Length();

        if (speed > 0.001f && fwdLen > 0.001f && rightLen > 0.001f)
        {
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
    float t = std::clamp(flyLeanResponse_ * owner_->GetDt(), 0.0f, 1.0f);
    flyLeanRotation_ = Quaternion::Slerp(flyLeanRotation_, targetLean, t);

    // ほぼ無回転（直立）に戻ったら描画オフセットを解除する
    if (flyLeanRotation_.w > 0.99999f)
    {
        flyLeanRotation_ = Quaternion::IdentityQuaternion();
        owner_->ClearRenderRotationOffset();
        return;
    }

    // 体のローカル空間の傾きとして適用。モデル中心が原点にないため flyLeanPivot_ で回転中心を補正する
    owner_->SetRenderRotationOffset(flyLeanRotation_, flyLeanPivot_);
}

void PlayerVisual::Save(DataHandler *data)
{
    data->Save("flyLeanEnabled", flyLeanEnabled_ ? 1 : 0);
    data->Save("flyLeanMaxFwdPitchDeg", flyLeanMaxFwdPitchDeg_);
    data->Save("flyLeanMaxBackPitchDeg", flyLeanMaxBackPitchDeg_);
    data->Save("flyLeanMaxSideDeg", flyLeanMaxSideDeg_);
    data->Save("flyLeanRefSpeed", flyLeanRefSpeed_);
    data->Save("flyLeanResponse", flyLeanResponse_);
    data->Save("flyLeanPivot", flyLeanPivot_);
}

void PlayerVisual::Load(DataHandler *data)
{
    flyLeanEnabled_ = data->Load<int>("flyLeanEnabled", flyLeanEnabled_ ? 1 : 0) != 0;
    flyLeanMaxFwdPitchDeg_ = data->Load<float>("flyLeanMaxFwdPitchDeg", flyLeanMaxFwdPitchDeg_);
    flyLeanMaxBackPitchDeg_ = data->Load<float>("flyLeanMaxBackPitchDeg", flyLeanMaxBackPitchDeg_);
    flyLeanMaxSideDeg_ = data->Load<float>("flyLeanMaxSideDeg", flyLeanMaxSideDeg_);
    flyLeanRefSpeed_ = data->Load<float>("flyLeanRefSpeed", flyLeanRefSpeed_);
    flyLeanResponse_ = data->Load<float>("flyLeanResponse", flyLeanResponse_);
    flyLeanPivot_ = data->Load<Hagine::Vector3>("flyLeanPivot", flyLeanPivot_);
}

void PlayerVisual::DrawImGui(const std::function<void()> &onSave)
{
#ifdef USE_IMGUI
    // ─── アニメーション制御 ───
    if (ImGui::CollapsingHeader("アニメーション制御"))
    {
        animationController_.DrawImGui();
    }

    // ─── 飛行リーン（体の傾き）───
    if (ImGui::CollapsingHeader("飛行リーン"))
    {
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
        if (ImGui::Button("飛行リーン設定を保存") && onSave)
        {
            onSave();
        }
    }
#else
    (void)onSave;
#endif // USE_IMGUI
}

void PlayerVisual::RegisterParams()
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register("Player", "飛行リーン有効", &flyLeanEnabled_);
    hub->Register("Player", "リーン前傾最大角(度)", &flyLeanMaxFwdPitchDeg_, {0.5f, 0.0f, 90.0f});
    hub->Register("Player", "リーン仰け反り最大角(度)", &flyLeanMaxBackPitchDeg_, {0.5f, 0.0f, 90.0f});
    hub->Register("Player", "リーン横ヨー最大角(度)", &flyLeanMaxSideDeg_, {0.5f, 0.0f, 90.0f});
    hub->Register("Player", "リーン基準速度", &flyLeanRefSpeed_, {0.1f, 0.1f, 30.0f});
    hub->Register("Player", "リーン追従速度", &flyLeanResponse_, {0.1f, 0.1f, 30.0f});
    hub->Register("Player", "リーン回転中心", &flyLeanPivot_, {0.05f});
}
