#include "DrawSystem.h"
#include "DirectXCommon.h"
#include "Collider/CollisionManager.h"
#include "Data/DataHandler.h"
#include "Engine/Utility/Debug/ImGui/ImGuiNotification.h"
#include "Graphics/Srv/SrvManager.h"
#include "Particle/ParticleEditor.h"
#include "Scene/SceneManager.h"
#include <Shadow/ShadowMap.h>
#include <algorithm>
#ifdef _DEBUG
#include "Particle/CSParticle/ParticleCSEditor.h"
#endif
#ifdef _DEBUG
#include "imgui.h"
#include "line/DrawLine3D.h"
#endif

void DrawSystem::Initialize(DirectXCommon *dxCommon, SrvManager *srvManager,
                             OffScreen *offscreen, SceneManager *sceneManager,
                             CollisionManager *collision) {
    dxCommon_    = dxCommon;
    srvManager_  = srvManager;
    sceneManager_ = sceneManager;
    collision_   = collision;

    stageOffScreens_[0] = offscreen;
    nextStageIndex_ = 1;
}

// -------------------------------------------------------
// 描画エントリ登録
// -------------------------------------------------------

void DrawSystem::RegisterImpl(std::string name, int stageIndex,
                               std::function<void(const ViewProjection &)> drawFunc) {
    for (auto &e : entries_) {
        if (e.name == name) {
            e.stageIndex = stageIndex;
            e.draw = std::move(drawFunc);
            return;
        }
    }
    entries_.push_back({std::move(name), stageIndex, std::move(drawFunc), true});
}

void DrawSystem::Register(std::string name, DrawLayer layer,
                           std::function<void(const ViewProjection &)> drawFunc) {
    int stage = (layer == DrawLayer::kPostEffect) ? kUILayer : static_cast<int>(layer);
    RegisterImpl(std::move(name), stage, std::move(drawFunc));
}

void DrawSystem::Register(std::string name, int stageIndex,
                           std::function<void(const ViewProjection &)> drawFunc) {
    RegisterImpl(std::move(name), stageIndex, std::move(drawFunc));
}

void DrawSystem::Unregister(const std::string &name) {
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
                       [&name](const DrawEntry &e) { return e.name == name; }),
        entries_.end());
}

void DrawSystem::Clear() {
    entries_.clear();
}

// -------------------------------------------------------
// マルチステージ管理
// -------------------------------------------------------

int DrawSystem::CreateStage() {
    int idx = nextStageIndex_++;
    auto owned = std::make_unique<OffScreen>();
    owned->Initialize();
    stageOffScreens_[idx] = owned.get();
    ownedOffScreens_.push_back(std::move(owned));
    return idx;
}

OffScreen *DrawSystem::GetStageOffScreen(int stageIndex) {
    auto it = stageOffScreens_.find(stageIndex);
    return (it != stageOffScreens_.end()) ? it->second : nullptr;
}

// -------------------------------------------------------
// メイン描画パイプライン
// -------------------------------------------------------

void DrawSystem::Draw(const ViewProjection &vp) {
    // ─── GPU パーティクル Compute フェーズ（全エミッターを一括実行して Direct Queue に Wait 挿入）───
    {
        for (auto &entry : entries_) {
            if (entry.enabled && entry.stageIndex == kGPUParticleCompute) {
                entry.draw(vp);
            }
        }
        // GPUパーティクルエディタのエミッターをシーン非依存で常時シミュレートする。
        // （プレビュー窓・各シーンでの確認のため。各シーンでの Register に依存しない全体駆動）
        ParticleCSEditor::GetInstance()->DrawAllCompute(vp);

        // 記録が無ければ ExecuteComputeCommands は自己ガードで no-op、Wait も signaled 済み値への待ちで無害。
        dxCommon_->ExecuteComputeCommands();
        dxCommon_->WaitForComputeOnDirectQueue();
    }

#ifdef _DEBUG
    // ─── GPUパーティクル プレビュー窓を描画（Compute 完了後・ステージ束ね前）───
    // Compute 済みの生存バッファを VS 読み取り可能な状態のままプレビューVPで再描画する。
    // 後段のステージループ(PreRenderTexture)がオフスクリーンRTと全画面ビューポートを束ね直すため復元不要。
    ParticleCSEditor::GetInstance()->RenderPreview();
#endif

    // ─── シャドウプレパス ───
    ShadowMap *shadowMap = ShadowMap::GetInstance();
    shadowMap->Update(); // 有効フラグをGPUバッファに反映（無効時もenabledを0にするため毎フレーム呼ぶ）
    if (shadowMap->IsEnabled()) {
        shadowMap->BeginShadowPass();
        srvManager_->SetDescriptorHeap();
        shadowMap->SetShadowPassActive(true);
        for (auto &entry : entries_) {
            if (entry.enabled && entry.stageIndex == 0) {
                entry.draw(vp);
            }
        }
        shadowMap->SetShadowPassActive(false);
        shadowMap->EndShadowPass();
    }

    // 登録済みステージ（kUILayer を除く）を昇順で処理
    std::vector<int> sortedStages;
    for (auto &[idx, _] : stageOffScreens_) {
        sortedStages.push_back(idx);
    }
    std::sort(sortedStages.begin(), sortedStages.end());

    OffScreen *lastOffScreen = nullptr;

    for (size_t si = 0; si < sortedStages.size(); ++si) {
        int stageIdx = sortedStages[si];
        OffScreen *stageOS = stageOffScreens_.at(stageIdx);

        // ─── オフスクリーンテクスチャへ描画 ───
        dxCommon_->PreRenderTexture();
        srvManager_->SetDescriptorHeap();

        // 前ステージの結果を背景として合成
        if (lastOffScreen) {
            stageOS->BlitToOffScreen(lastOffScreen->GetFinalResultSrvIndex());
        }

        for (auto &entry : entries_) {
            if (entry.enabled && entry.stageIndex == stageIdx) {
                entry.draw(vp);
            }
        }

        // 注: GPUパーティクルエディタのエミッターは「プレビュー窓のみ」で確認する。
        // 以前はここで DrawAllGraphics(vp) を呼び現在のシーン offscreen にも描画していたが、
        // 編集中のパーティクルがゲームシーンに漏れて見えてしまうため撤去。
        // シミュレーション（DrawAllCompute）は上のフェーズで実行済みで、
        // 描画は RenderPreview()（プレビュー専用VP）だけが行う。

#ifdef _DEBUG
        if (stageIdx == 0) {
            DrawLine3D::GetInstance()->Draw(vp);
            collision_->DebugDraw(vp);
        }
#endif

        // ─── ポストエフェクト適用 → finalResult（コピーなし）───
        if (si == 0) {
            dxCommon_->PreDraw(); // 初回: バックバッファも遷移
        } else {
            dxCommon_->PreDrawForEffects(); // 2回目以降: バックバッファ遷移なし
        }
        stageOS->SetProjection(vp.matProjection_);
        stageOS->DrawWithoutCopy();
        dxCommon_->TransitionDepthBarrier();

        lastOffScreen = stageOS;
    }

    if (!lastOffScreen) {
        ParticleEditor::GetInstance()->UpdateFrameStats();
        return;
    }

    // ─── UI・シーン遷移を finalResult に合成 ───
    lastOffScreen->BeginCompositePass();
    srvManager_->SetDescriptorHeap();

    for (auto &entry : entries_) {
        if (entry.enabled && entry.stageIndex == kUILayer) {
            entry.draw(vp);
        }
    }

    // シーン遷移は最前面（UIの上）
    sceneManager_->DrawTransition();

    lastOffScreen->EndCompositePass();

    // ─── finalResult（フルフレーム）をバックバッファへコピー ───
    lastOffScreen->CopyFinalResultToBackBuffer();

    ParticleEditor::GetInstance()->UpdateFrameStats();
}

// -------------------------------------------------------
// ImGui
// -------------------------------------------------------

void DrawSystem::UpdateImGui() {
#ifdef _DEBUG
    if (ImGui::Begin("DrawSystem")) {
        ImGui::Text("登録エントリ数: %zu", entries_.size());
        ImGui::Separator();

        for (auto &entry : entries_) {
            ImGui::PushID(entry.name.c_str());

            ImGui::Checkbox("##en", &entry.enabled);
            ImGui::SameLine();

            if (entry.stageIndex == kUILayer) {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.5f, 0.3f, 0.7f, 0.85f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.6f, 0.4f, 0.8f, 0.90f});
                if (ImGui::Button("[UI Layer]")) {
                    entry.stageIndex = 0;
                }
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.5f, 0.8f, 0.85f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.3f, 0.6f, 0.9f, 0.90f});
                char label[32];
                snprintf(label, sizeof(label), "[Stage %d] ", entry.stageIndex);
                if (ImGui::Button(label)) {
                    entry.stageIndex = kUILayer;
                }
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();
            ImGui::TextUnformatted(entry.name.c_str());

            ImGui::PopID();
        }

        ImGui::Separator();
        if (ImGui::Button("保存")) { SaveConfig(); }
        ImGui::SameLine();
        if (ImGui::Button("読み込み")) { LoadConfig(); }
    }
    ImGui::End();
#endif
}

// -------------------------------------------------------
// JSON 保存 / 読み込み
// -------------------------------------------------------

void DrawSystem::SaveConfig(const std::string &fileName) {
    auto data = std::make_unique<DataHandler>("DrawSystem", fileName);
    int count = static_cast<int>(entries_.size());
    data->Save("count", count);
    for (int i = 0; i < count; ++i) {
        const auto &e = entries_[i];
        std::string prefix = "entry_" + std::to_string(i) + "_";
        data->Save(prefix + "name", e.name);
        data->Save(prefix + "stage", e.stageIndex);
        data->Save(prefix + "enabled", static_cast<int>(e.enabled));
    }
    ImGuiNotification::Post("描画設定を保存しました: " + fileName, {0.2f, 0.8f, 0.2f, 1.0f});
}

void DrawSystem::LoadConfig(const std::string &fileName) {
    auto data = std::make_unique<DataHandler>("DrawSystem", fileName);
    int count = data->Load<int>("count", 0);
    for (int i = 0; i < count; ++i) {
        std::string prefix = "entry_" + std::to_string(i) + "_";
        std::string name    = data->Load<std::string>(prefix + "name", "");
        int stage   = data->Load<int>(prefix + "stage", 0);
        int enabled = data->Load<int>(prefix + "enabled", 1);

        for (auto &e : entries_) {
            if (e.name == name) {
                e.stageIndex = stage;
                e.enabled    = static_cast<bool>(enabled);
                break;
            }
        }
    }
    ImGuiNotification::Post("描画設定を読み込みました: " + fileName, {0.2f, 0.8f, 0.8f, 1.0f});
}
