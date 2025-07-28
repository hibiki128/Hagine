#include "OffScreen.h"
#include "DirectXCommon.h"
#include <Frame.h>
#include <format>
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG

void OffScreen::Initialize() {
    DirectXCommon *dxCommon = DirectXCommon::GetInstance();
    SrvManager *srvManager = SrvManager::GetInstance();
    PipeLineManager *psoManager = PipeLineManager::GetInstance();

    // 各コンポーネントを初期化
    renderer_.Initialize(dxCommon, srvManager, psoManager);
    parameters_.Initialize(dxCommon);
    dataManager_.Initialize();

    // 保存されたデータを読み込み
    dataManager_.LoadData(effectChain_, parameters_);
}

void OffScreen::Draw() {
    parameters_.UpdateTimeParameters(Frame::DeltaTime());
    renderer_.Draw(effectChain_, parameters_);
}

void OffScreen::SetProjection(Matrix4x4 projectionMatrix) {
    projectionMatrix_ = projectionMatrix;
    parameters_.SetProjection(projectionMatrix);
}

uint32_t OffScreen::GetFinalResultSrvIndex() const {
    return renderer_.GetFinalResultSrvIndex();
}

void OffScreen::CopyFinalResultToBackBuffer() {
    renderer_.CopyFinalResultToBackBuffer();
}

void OffScreen::Setting() {
#ifdef _DEBUG
    ImGui::Text("ポストエフェクト");

    // エフェクト追加ボタン
    const char *shaderModeItems[] = {
        "なし", "グレイ", "ビネット", "スムース", "ガウス",
        "アウトライン(エッジ検出)", "アウトライン(深度ベース)",
        "ブラー", "シネマティック", "ディゾルブ", "ランダム", "集中線"};
    static int selectedEffect = 0;

    ImGui::Combo("追加するエフェクト", &selectedEffect, shaderModeItems, IM_ARRAYSIZE(shaderModeItems));

    if (ImGui::Button("エフェクトを追加")) {
        effectChain_.AddEffect(static_cast<ShaderMode>(selectedEffect));
    }

    ImGui::Separator();

    // エフェクトチェーン表示・編集
    const auto &effects = effectChain_.GetEffects();
    for (int i = 0; i < static_cast<int>(effects.size()); ++i) {
        ImGui::PushID(i);

        ImGui::Text("エフェクト %d: %s", i + 1, shaderModeItems[static_cast<int>(effects[i].shaderMode)]);

        ImGui::SameLine();
        bool enabled = effects[i].enabled;
        if (ImGui::Checkbox("有効", &enabled)) {
            effectChain_.SetEffectEnabled(i, enabled);
        }

        ImGui::SameLine();
        if (ImGui::Button("上へ") && i > 0) {
            effectChain_.MoveEffectUp(i);
        }

        ImGui::SameLine();
        if (ImGui::Button("下へ") && i < static_cast<int>(effects.size()) - 1) {
            effectChain_.MoveEffectDown(i);
        }

        ImGui::SameLine();
        if (ImGui::Button("削除")) {
            effectChain_.RemoveEffect(i);
            ImGui::PopID();
            break; // インデックスが変わるのでbreak
        }

        // 各エフェクトの個別設定
        if (effects[i].enabled) {
            parameters_.DrawParameterUI(effects[i].shaderMode);
        }

        ImGui::PopID();
        ImGui::Separator();
    }

    if (ImGui::Button("セーブ")) {
        dataManager_.SaveData(effectChain_, parameters_);
        std::string message = std::format("OffScreen saved.");
        MessageBoxA(nullptr, message.c_str(), "OffScreen", 0);
    }
#endif // _DEBUG
}