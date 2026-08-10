#include "PlayModeManager.h"
#ifdef USE_IMGUI
#include "SpriteManager.h"
#include <imgui.h>
#include <object/base/BaseObjectManager.h>
#include <utility/debug/imgui/DebugUIHelper.h>
#include <utility/debug/imgui/ImGuiNotification.h>

namespace Hagine {

void PlayModeManager::CaptureBaseline()
{
    objectBaseline_ = BaseObjectManager::GetInstance()->CaptureUndoState();
    spriteBaseline_ = SpriteManager::GetInstance()->CaptureUndoState();
    hasBaseline_ = true;
}

void PlayModeManager::OnSceneChanged()
{
    // 前のシーンのスナップショットを持ち越さない。
    // 持ち越すと、停止したときに前シーンのオブジェクトが今のシーンに生成されてしまう。
    objectBaseline_ = nlohmann::json::object();
    spriteBaseline_ = nlohmann::json::object();
    hasBaseline_ = false;

    // シーンが変わったら再生中に戻す（切り替えた先が止まったままだと動かせないため）
    state_ = State::Playing;
    stepRequested_ = false;
}

void PlayModeManager::RestoreBaseline()
{
    if (!hasBaseline_)
    {
        return;
    }
    BaseObjectManager::GetInstance()->RestoreUndoState(objectBaseline_);
    SpriteManager::GetInstance()->RestoreUndoState(spriteBaseline_);
}

void PlayModeManager::Play()
{
    if (state_ == State::Playing)
    {
        return;
    }
    // 一時停止からの再開では控え直さない（停止で戻る先が「最初に再生した時点」のままになるように）
    if (state_ == State::Editing)
    {
        CaptureBaseline();
    }
    state_ = State::Playing;
    stepRequested_ = false;
    ImGuiNotification::Post("再生", {0.45f, 0.68f, 0.52f, 1.0f});
}

void PlayModeManager::Pause()
{
    if (state_ != State::Playing)
    {
        return;
    }
    state_ = State::Paused;
    stepRequested_ = false;
    ImGuiNotification::Post("一時停止", {0.80f, 0.72f, 0.42f, 1.0f});
}

void PlayModeManager::StepOnce()
{
    // 再生中に押されたら、まず止めてから1フレームだけ進める
    if (state_ == State::Playing)
    {
        state_ = State::Paused;
    }
    stepRequested_ = true;
}

void PlayModeManager::Stop()
{
    if (state_ == State::Editing)
    {
        return;
    }
    state_ = State::Editing;
    stepRequested_ = false;
    RestoreBaseline();
    ImGuiNotification::Post("停止（再生前の状態へ戻しました）", {0.42f, 0.66f, 0.68f, 1.0f});
}

void PlayModeManager::EndFrame()
{
    // ステップ実行はこのフレームで消費する
    stepRequested_ = false;
}

void PlayModeManager::DrawToolbar()
{
    const bool isPlaying = (state_ == State::Playing);
    const bool isEditing = (state_ == State::Editing);

    // 再生中は緑、それ以外は通常色。押しているモードが一目で分かるようにする。
    ImGui::PushStyleColor(ImGuiCol_Button,
                          isPlaying ? DebugTheme::kBgGreen : ImGui::GetStyle().Colors[ImGuiCol_Button]);
    if (ImGui::Button(isPlaying ? "再生中" : "再生"))
    {
        Play();
    }
    ImGui::PopStyleColor();
    ImGui::SetItemTooltip("ゲームの更新を開始します（停止から始めると、この時点の状態を控えます）");

    ImGui::SameLine();
    ImGui::BeginDisabled(!isPlaying);
    if (ImGui::Button("一時停止"))
    {
        Pause();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("ゲームの更新だけ止めます（描画とエディタは動いたままです）");

    ImGui::SameLine();
    ImGui::BeginDisabled(isPlaying);
    if (ImGui::Button("コマ送り"))
    {
        StepOnce();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("1フレームだけ進めます");

    ImGui::SameLine();
    ImGui::BeginDisabled(isEditing);
    if (ImGui::Button("停止"))
    {
        Stop();
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("更新を止め、再生を押した時点の配置へ戻します\n"
                          "（戻るのは配置オブジェクトとスプライト。プレイヤー等はシーン再読み込みで戻ります）");

    ImGui::SameLine();
    const char *label = isPlaying ? "再生中" : (state_ == State::Paused ? "一時停止" : "停止中");
    StatusBadge(label, isPlaying ? DebugTheme::kAccentGreen
                                 : (state_ == State::Paused ? DebugTheme::kAccentYellow
                                                            : DebugTheme::kTextDim));
}
} // namespace Hagine
#endif // USE_IMGUI
