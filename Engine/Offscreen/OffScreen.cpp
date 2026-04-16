#include "OffScreen.h"
#include "DirectXCommon.h"
#include <Frame.h>
#include <format>
#ifdef _DEBUG
#include "imgui.h"
#endif

void OffScreen::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    SrvManager *srvManager     = SrvManager::GetInstance();
    PipeLineManager *psoManager = PipeLineManager::GetInstance();

    renderer_.Initialize(dxCommon_, srvManager, psoManager);
    dataManager_.Initialize();

    // 保存データを読み込み（スロットへの復元）
    dataManager_.LoadData(effectChain_, dxCommon_);
}

void OffScreen::Draw() {
    renderer_.Draw(effectChain_, Frame::DeltaTime());
}

void OffScreen::SetProjection(Matrix4x4 projectionMatrix) {
    projectionMatrix_ = projectionMatrix;

    // 深度ベースアウトライン等、射影逆行列が必要なエフェクトに反映
    const auto &slots = effectChain_.GetSlots();
    for (int i = 0; i < PostEffectChain::kMaxSlots; ++i) {
        if (!slots[i].occupied) { continue; }
        if (auto *p = effectChain_.GetParams<OutlineDepthParams>(i)) {            p->SetProjectionInverse(projectionMatrix);
        }
    }
}

uint32_t OffScreen::GetFinalResultSrvIndex() const {
    return renderer_.GetFinalResultSrvIndex();
}

void OffScreen::CopyFinalResultToBackBuffer() {
    renderer_.CopyFinalResultToBackBuffer();
}

// -------------------------------------------------------
//  エフェクト管理API（OffScreenのラッパー）
// -------------------------------------------------------

int OffScreen::AddEffect(ShaderMode mode, const std::string &name, int slotIndex) {
    int result = effectChain_.AddEffect(mode, name, dxCommon_, slotIndex);

    // 射影逆行列が必要なエフェクトへの即時反映
    if (result != -1) {
        if (auto *p = effectChain_.GetParams<OutlineDepthParams>(result)) {
            p->SetProjectionInverse(projectionMatrix_);
        }
    }
    return result;
}

bool OffScreen::RemoveEffect(int slotIndex) {
    return effectChain_.RemoveEffect(slotIndex);
}

int OffScreen::RemoveEffectByName(const std::string &name) {
    return effectChain_.RemoveEffectByName(name);
}

int OffScreen::RemoveAllEffectsByName(const std::string &name) {
    return effectChain_.RemoveAllEffectsByName(name);
}

bool OffScreen::SetEffectEnabled(int slotIndex, bool enabled) {
    return effectChain_.SetEnabled(slotIndex, enabled);
}

bool OffScreen::MoveEffectUp(int slotIndex) {
    return effectChain_.MoveUp(slotIndex);
}

bool OffScreen::MoveEffectDown(int slotIndex) {
    return effectChain_.MoveDown(slotIndex);
}

// -------------------------------------------------------
//  ImGui
// -------------------------------------------------------

void OffScreen::Setting() {
#ifdef _DEBUG
    ImGui::Text("ポストエフェクト（空き: %d / %d）",
                effectChain_.GetFreeSlotCount(), PostEffectChain::kMaxSlots);

    // --- エフェクト追加UI ---
    const char *shaderModeItems[] = {
        "なし", "グレイ", "ビネット", "スムース", "ガウス",
        "アウトライン(エッジ検出)", "アウトライン(深度ベース)",
        "ブラー", "シネマティック", "ディゾルブ", "ランダム", "集中線", "ピクセル化", "ブルーム"};

    static int selectedMode = 0;
    static char effectName[64] = "";
    static int targetSlot = -1; // -1で自動

    ImGui::Combo("追加するエフェクト", &selectedMode, shaderModeItems, IM_ARRAYSIZE(shaderModeItems));
    ImGui::InputText("名前", effectName, sizeof(effectName));
    ImGui::SliderInt("スロット指定（-1で自動）", &targetSlot, -1, PostEffectChain::kMaxSlots - 1);

    if (ImGui::Button("エフェクトを追加")) {
        int result = AddEffect(static_cast<ShaderMode>(selectedMode), effectName, targetSlot);
        if (result == -1) {
            ImGui::OpenPopup("AddFailed");
        }
    }

    if (ImGui::BeginPopup("AddFailed")) {
        ImGui::Text("追加失敗: スロットが満杯または指定スロットが使用中です");
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // --- スロット一覧 ---
    const auto &slots = effectChain_.GetSlots();
    for (int i = 0; i < PostEffectChain::kMaxSlots; ++i) {
        ImGui::PushID(i);

        if (!slots[i].occupied) {
            // 空きスロットはグレー表示
            ImGui::TextDisabled("[%d] ---（空き）---", i);
            ImGui::PopID();
            continue;
        }

        const auto &slot = slots[i];
        ShaderMode mode  = slot.params->GetMode();

        ImGui::Text("[%d] %s (%s)", i, slot.name.c_str(), shaderModeItems[static_cast<int>(mode)]);

        ImGui::SameLine();
        bool enabled = slot.enabled;
        if (ImGui::Checkbox("有効", &enabled)) {
            effectChain_.SetEnabled(i, enabled);
        }

        ImGui::SameLine();
        if (ImGui::Button("↑") && i > 0) { effectChain_.MoveUp(i); }
        ImGui::SameLine();
        if (ImGui::Button("↓") && i < PostEffectChain::kMaxSlots - 1) { effectChain_.MoveDown(i); }
        ImGui::SameLine();
        if (ImGui::Button("削除")) {
            effectChain_.RemoveEffect(i);
            ImGui::PopID();
            continue;
        }

        // 有効時のみパラメータUI表示
        if (slot.enabled) {
            ImGui::Indent();
            slot.params->DrawUI();
            ImGui::Unindent();
        }

        ImGui::PopID();
        ImGui::Separator();
    }

    // --- セーブ ---
    if (ImGui::Button("セーブ")) {
        dataManager_.SaveData(effectChain_);
        std::string msg = std::format("OffScreen saved.");
        MessageBoxA(nullptr, msg.c_str(), "OffScreen", 0);
    }
#endif
}
