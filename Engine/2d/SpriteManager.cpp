#define NOMINMAX
#include "SpriteManager.h"
#include "Engine/Utility/Debug/ImGui/Debugui_improved.h"
#include "SpriteCommon.h"
#include "WinApp.h"
#include "myMath.h"
#include <Data/DataHandler.h>
#include <Engine/Utility/Debug/ImGui/ImGuiNotification.h>
#include <ShowFolder/ShowFolder.h>
#include <filesystem>

namespace fs = std::filesystem;

void SpriteManager::Finalize() {
    sprites_.clear();
}

void SpriteManager::RegisterSprite(const std::string &name, const std::string &textureFilePath, const SpriteTransform &transform) {
    auto spriteData = std::make_unique<SpriteData>(name, textureFilePath, transform.instanceCount);

    spriteData->sprite = std::make_unique<Sprite>();
    spriteData->sprite->Initialize(textureFilePath, transform.position, transform.color,
                                   transform.anchorPoint, transform.isFlipX, transform.isFlipY);
    spriteData->sprite->SetInstanceCount(transform.instanceCount);
    spriteData->sprite->SetUVPosition({0.0f, 0.0f});
    spriteData->sprite->SetUVSize({1.0f, 1.0f});
    spriteData->sprite->SetUVRotate(0.0f);

    // インスタンスデータの初期化
    for (uint32_t i = 0; i < transform.instanceCount; ++i) {
        spriteData->instanceData[i].translation = {transform.position.x, transform.position.y, 0.0f};
    }

    sprites_.push_back(std::move(spriteData));
    UpdateSpriteInstances(sprites_.back().get());
    ImGuiNotification::Post("スプライトを登録しました: " + name, {0.4f, 0.8f, 1.0f, 1.0f});
}

void SpriteManager::UnregisterSprite(const std::string &name) {
    auto it = std::find_if(sprites_.begin(), sprites_.end(),
                           [&name](const std::unique_ptr<SpriteData> &sprite) {
                               return sprite->name == name;
                           });

    if (it != sprites_.end()) {
        ImGuiNotification::Post("スプライトを削除しました: " + name, {0.9f, 0.7f, 0.2f, 1.0f});
        sprites_.erase(it);
    }
}

void SpriteManager::DrawAll() {
    for (auto &spriteData : sprites_) {
        if (spriteData->isVisible) {
            // ブレンドモードを設定してから描画
            SpriteCommon::GetInstance()->SetBlendMode(spriteData->blendMode);
            spriteData->sprite->Draw(spriteData->isBackMost);
        }
    }
}

void SpriteManager::SetSpriteBlendMode(const std::string &name, BlendMode blendMode) {
    auto spriteData = GetSprite(name);
    if (spriteData) {
        spriteData->blendMode = blendMode;
    }
}

void SpriteManager::UpdateAll(float deltaTime) {
    for (auto &spriteData : sprites_) {
        if (spriteData->isVisible) {
            // カスタム更新関数が設定されていれば実行
            if (spriteData->updateFunction) {
                spriteData->updateFunction(*spriteData, deltaTime);
            }

            // インスタンスデータを基にTransformationMatrixを更新
            UpdateSpriteInstances(spriteData.get());
        }
    }
}

void SpriteManager::UpdateImGui() {
#ifdef _DEBUG
    DrawSpriteCreationModal();
#endif // _DEBUG
}

std::string SpriteManager::GetTextureFilePath(const std::string &name) {
    auto spriteData = GetSprite(name);
    return spriteData ? spriteData->textureFilePath : "";
}

std::vector<SpriteData *> SpriteManager::GetAllSprites() {
    std::vector<SpriteData *> result;
    result.reserve(sprites_.size());
    for (auto &s : sprites_) {
        result.push_back(s.get());
    }
    return result;
}

void SpriteManager::SetTextureFilePath(const std::string &name, const std::string &textureFilePath) {
    auto spriteData = GetSprite(name);
    if (spriteData) {
        spriteData->textureFilePath = textureFilePath;
        spriteData->sprite->SetTexturePath(textureFilePath);
    }
}

SpriteData *SpriteManager::GetSprite(const std::string &name) {
    return FindSpriteByName(name);
}

SpriteData *SpriteManager::FindSpriteByName(const std::string &name) {
    auto it = std::find_if(sprites_.begin(), sprites_.end(),
                           [&name](const std::unique_ptr<SpriteData> &sprite) {
                               return sprite->name == name;
                           });
    return (it != sprites_.end()) ? it->get() : nullptr;
}

int SpriteManager::FindSpriteIndex(const std::string &name) {
    for (size_t i = 0; i < sprites_.size(); ++i) {
        if (sprites_[i]->name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void SpriteManager::SetInstanceSRT(const std::string &name, uint32_t index, const InstanceSRT &srt) {
    auto spriteData = GetSprite(name);
    if (spriteData && index < spriteData->instanceData.size()) {
        spriteData->instanceData[index] = srt;
    }
}

void SpriteManager::SetInstanceScale(const std::string &name, uint32_t index, const Vector3 &scale) {
    auto spriteData = GetSprite(name);
    if (spriteData && index < spriteData->instanceData.size()) {
        spriteData->instanceData[index].scale = scale;
    }
}

void SpriteManager::SetInstanceRotation(const std::string &name, uint32_t index, const Vector3 &rotation) {
    auto spriteData = GetSprite(name);
    if (spriteData && index < spriteData->instanceData.size()) {
        spriteData->instanceData[index].rotation = rotation;
    }
}

void SpriteManager::SetInstanceTranslation(const std::string &name, uint32_t index, const Vector3 &translation) {
    auto spriteData = GetSprite(name);
    if (spriteData && index < spriteData->instanceData.size()) {
        spriteData->instanceData[index].translation = translation;
    }
}

void SpriteManager::SetInstanceActive(const std::string &name, uint32_t index, bool isActive) {
    auto spriteData = GetSprite(name);
    if (spriteData && index < spriteData->instanceData.size()) {
        spriteData->instanceData[index].isActive = isActive;
    }
}

InstanceSRT *SpriteManager::GetInstanceSRT(const std::string &name, uint32_t index) {
    auto spriteData = GetSprite(name);
    if (spriteData && index < spriteData->instanceData.size()) {
        return &spriteData->instanceData[index];
    }
    return nullptr;
}

void SpriteManager::SetSpriteVisible(const std::string &name, bool visible) {
    auto spriteData = GetSprite(name);
    if (spriteData) {
        spriteData->isVisible = visible;
    }
}

void SpriteManager::SetSpriteBackMost(const std::string &name, bool isBackMost) {
    auto spriteData = GetSprite(name);
    if (spriteData) {
        spriteData->isBackMost = isBackMost;
    }
}

void SpriteManager::SetSpritePosition(const std::string &name, const Vector2 &position) {
    auto spriteData = GetSprite(name);
    if (spriteData) {
        spriteData->sprite->SetPosition(position);
    }
}

void SpriteManager::SetSpriteSize(const std::string &name, const Vector2 &size) {
    auto spriteData = GetSprite(name);
    if (spriteData) {
        spriteData->sprite->SetSize(size);
    }
}

void SpriteManager::SetSpriteColor(const std::string &name, const Vector4 &color) {
    auto spriteData = GetSprite(name);
    if (spriteData) {
        spriteData->sprite->SetColor({color.x, color.y, color.z});
        spriteData->sprite->SetAlpha(color.w);
    }
}

void SpriteManager::SetUpdateFunction(const std::string &name, std::function<void(SpriteData &, float)> updateFunc) {
    auto spriteData = GetSprite(name);
    if (spriteData) {
        spriteData->updateFunction = updateFunc;
    }
}

void SpriteManager::Clear() {
    sprites_.clear();
}

void SpriteManager::UpdateSpriteInstances(SpriteData *spriteData) {
    if (!spriteData || !spriteData->sprite)
        return;

    // 全インスタンス数を設定（非アクティブも含む）
    spriteData->sprite->SetInstanceCount(static_cast<uint32_t>(spriteData->instanceData.size()));

    for (uint32_t i = 0; i < spriteData->instanceData.size(); ++i) {
        const auto &instanceSRT = spriteData->instanceData[i];

        Transform transform;
        transform.scale = instanceSRT.scale;
        transform.rotate = instanceSRT.rotation;
        transform.translate = instanceSRT.translation;
        transform.scale.z = 1.0f;
        transform.translate.z = 0.0f;

        // 非アクティブなインスタンスは画面外に配置するか、スケールを0にする
        if (!instanceSRT.isActive) {
            transform.scale = {0.0f, 0.0f, 1.0f};
        }

        Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
        Matrix4x4 viewMatrix = MakeIdentity4x4();
        Matrix4x4 projectionMatrix = MakeOrthographicMatrix(
            0.0f, 0.0f,
            float(WinApp::kClientWidth),
            float(WinApp::kClientHeight),
            0.0f, 100.0f);

        TransformationMatrix transformMatrix;
        transformMatrix.WVP = worldMatrix * viewMatrix * projectionMatrix;
        transformMatrix.World = worldMatrix;

        spriteData->sprite->SetInstanceTransform(i, transformMatrix);
    }
}

void SpriteManager::DrawSpriteCreationModal() {
#ifdef _DEBUG
    if (showSpriteCreationModal_) {
        ImGui::OpenPopup("スプライト生成##modal");
        showSpriteCreationModal_ = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    ImGui::SetNextWindowSize(ImVec2(1080, 0), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("スプライト生成##modal", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoResize)) {
        static char nameBuf[128] = "";
        static SpriteTransform tf;
        static bool inited = false;
        if (!inited) {
            tf = SpriteTransform();
            inited = true;
        }

        // ---- Name ----
        SectionHeader("[ 名前 ]", DebugTheme::kAccentBlue);
        ImGui::SetNextItemWidth(-1);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgBlue);
        ImGui::InputText("##spname", nameBuf, IM_ARRAYSIZE(nameBuf));
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // ---- Texture ----
        SectionHeader("[ テクスチャ ]", DebugTheme::kAccentOrange);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.10f, 0.10f, 0.12f, 1.0f});
        ImGui::BeginChild("TexSel##modal", ImVec2(-1, 360), true);
        ShowTextureFile(texturePath_);
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::Text("選択中: %s",
                    texturePath_.empty() ? "(未選択)" : texturePath_.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // ---- Settings ----
        SectionHeader("[ 設定 ]", DebugTheme::kAccentGreen);

        // 位置
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::TextUnformatted("位置 (X / Y)");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(-1);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgGreen);
        ImGui::DragFloat2("##sppos", &tf.position.x, 1.0f);
        ImGui::PopStyleColor();

        // 色
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::TextUnformatted("カラー (R / G / B / A)");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(-1);
        ImGui::ColorEdit4("##spcol", &tf.color.x);

        // アンカー
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::TextUnformatted("アンカーポイント (0.0 - 1.0)");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat2("##spanc", &tf.anchorPoint.x, 0.0f, 1.0f, "%.2f");

        // フリップ
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::TextUnformatted("反転");
        ImGui::PopStyleColor();
        ImGui::Checkbox("水平##spfx", &tf.isFlipX);
        ImGui::SameLine();
        ImGui::Checkbox("垂直##spfy", &tf.isFlipY);

        // インスタンス数
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::TextUnformatted("インスタンス数 (1 - 1000)");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputScalar("##spinst", ImGuiDataType_U32, &tf.instanceCount,
                           nullptr, nullptr, nullptr,
                           ImGuiInputTextFlags_CharsDecimal);
        tf.instanceCount = std::clamp(tf.instanceCount, 1u, 1000u);

        ImGui::Spacing();
        ImGui::Separator();

        // ---- バリデーション ----
        bool nameOk = strlen(nameBuf) > 0;
        bool texOk = !texturePath_.empty();
        bool nameUniq = (GetSprite(nameBuf) == nullptr);
        bool canCreate = nameOk && texOk && nameUniq;

        if (!canCreate) {
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentRed);
            if (!nameOk)
                ImGui::TextUnformatted("  * スプライト名を入力してください");
            if (!texOk)
                ImGui::TextUnformatted("  * テクスチャファイルを選択してください");
            if (!nameUniq)
                ImGui::TextUnformatted("  * 同名のスプライトが既に存在します");
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        // ---- ボタン ----
        auto ResetModal = [&]() {
            memset(nameBuf, 0, sizeof(nameBuf));
            texturePath_ = "";
            tf = SpriteTransform();
            inited = false;
        };

        float bw = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        ImGui::PushStyleColor(ImGuiCol_Button,
                              canCreate ? ImVec4{0.20f, 0.50f, 0.20f, 0.85f}
                                        : ImVec4{0.25f, 0.25f, 0.28f, 0.60f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              canCreate ? ImVec4{0.25f, 0.60f, 0.25f, 0.90f}
                                        : ImVec4{0.25f, 0.25f, 0.28f, 0.60f});
        if (ImGui::Button("生成##spcreate", ImVec2(bw, 0)) && canCreate) {
            RegisterSprite(nameBuf, texturePath_, tf);
            ResetModal();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, {0.45f, 0.20f, 0.20f, 0.85f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.60f, 0.25f, 0.25f, 0.90f});
        if (ImGui::Button("キャンセル##spcancel", ImVec2(bw, 0))) {
            ResetModal();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(3);
#endif // _DEBUG
}

void SpriteManager::DrawSpriteManager() {
#ifdef _DEBUG
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    // ---- 新規作成ボタン ----
    ImGui::PushStyleColor(ImGuiCol_Button, {0.20f, 0.50f, 0.20f, 0.85f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f, 0.60f, 0.25f, 0.90f});
    if (ImGui::Button("+ スプライト新規作成##spmain", ImVec2(-1, 0)))
        ShowSpriteCreationModal();
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
    ImGui::Text("登録スプライト数: %zu", sprites_.size());
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (sprites_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::TextUnformatted("  スプライトが登録されていません。");
        ImGui::PopStyleColor();
    } else {
        // ====================================================
        // リストテーブル（描画順・表示切替・削除）
        // ====================================================
        SectionHeader("[ 描画順 (上が手前) ]", DebugTheme::kAccentBlue);

        float tableH = std::min((float)sprites_.size() * 26.f + 36.f, 160.f);

        if (ImGui::BeginTable("SprList", 6,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable,
                              ImVec2(-1, tableH))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, 22.f);
            ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthFixed, 80.f);
            ImGui::TableSetupColumn("表示", ImGuiTableColumnFlags_WidthFixed, 34.f);
            ImGui::TableSetupColumn("数", ImGuiTableColumnFlags_WidthFixed, 28.f);
            ImGui::TableSetupColumn("テクスチャ", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("削除", ImGuiTableColumnFlags_WidthFixed, 34.f);
            ImGui::TableHeadersRow();

            std::vector<std::string> toDelete;
            for (size_t i = 0; i < sprites_.size(); ++i) {
                auto &sp = sprites_[i];
                ImGui::TableNextRow();
                ImGui::PushID((int)i);

                // 順序 & 矢印
                ImGui::TableNextColumn();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
                if (i > 0 && ImGui::ArrowButton("U", ImGuiDir_Up))
                    std::swap(sprites_[i], sprites_[i - 1]);
                if (i < sprites_.size() - 1 && ImGui::ArrowButton("D", ImGuiDir_Down))
                    std::swap(sprites_[i], sprites_[i + 1]);
                ImGui::PopStyleVar();

                // 名前
                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(sp->name.c_str());

                // 表示チェック
                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_CheckMark,
                                      sp->isVisible ? DebugTheme::kAccentGreen : DebugTheme::kTextDim);
                ImGui::Checkbox("##vis", &sp->isVisible);
                ImGui::PopStyleColor();

                // インスタンス数
                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                ImGui::Text("%zu", sp->instanceData.size());
                ImGui::PopStyleColor();

                // テクスチャ
                ImGui::TableNextColumn();
                std::string p = sp->textureFilePath;
                if (p.size() > 20)
                    p = ".." + p.substr(p.size() - 18);
                ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                ImGui::TextUnformatted(p.c_str());
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", sp->textureFilePath.c_str());

                // 削除
                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Button, {0.65f, 0.20f, 0.20f, 0.75f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.85f, 0.25f, 0.25f, 0.90f});
                if (ImGui::SmallButton("削除##del"))
                    toDelete.push_back(sp->name);
                ImGui::PopStyleColor(2);

                ImGui::PopID();
            }
            for (auto &n : toDelete)
                UnregisterSprite(n);
            ImGui::EndTable();
        }

        ImGui::Spacing();

        // ====================================================
        // 各スプライト詳細
        // ====================================================
        SectionHeader("[ スプライト詳細 ]", DebugTheme::kAccentPurple);

        for (auto &sp : sprites_) {
            ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kBgPurple);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.7f, 0.3f, 1.0f, 0.20f});
            bool open = ImGui::CollapsingHeader(sp->name.c_str());
            ImGui::PopStyleColor(2);
            if (!open)
                continue;

            ImGui::PushID(sp->name.c_str());
            ImGui::Indent(6.0f);

            // ---- Basic ----
            ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kBgBlue);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.20f, 0.55f, 1.0f, 0.20f});
            if (ImGui::TreeNodeEx("基本設定##bs", ImGuiTreeNodeFlags_SpanAvailWidth)) {
                // Position
                {
                    Vector2 pos = sp->sprite->GetPosition();
                    ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                    ImGui::TextUnformatted("位置 (X / Y)");
                    ImGui::PopStyleColor();
                    float v[2] = {pos.x, pos.y};
                    ImGui::SetNextItemWidth(-1);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgBlue);
                    if (ImGui::DragFloat2("##bspos", v, 1.f))
                        sp->sprite->SetPosition({v[0], v[1]});
                    ImGui::PopStyleColor();
                }
                ImGui::Spacing();

                // Size - 比率維持 / XY独立 モード切り替え
                {
                    Vector2 sz = sp->sprite->GetSize();

                    // ラベルとモード切り替えボタンを横並びに配置
                    ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                    ImGui::TextUnformatted("サイズ");
                    ImGui::PopStyleColor();
                    ImGui::SameLine();

                    // アスペクト比ロック状態に応じてボタン色を切り替える
                    const bool locked = sp->lockAspectRatio;
                    if (locked) {
                        ImGui::PushStyleColor(ImGuiCol_Button, DebugTheme::kAccentBlue);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.35f, 0.65f, 1.0f, 0.9f});
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, DebugTheme::kBgBlue);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.4f, 0.6f, 0.5f});
                    }
                    if (ImGui::SmallButton(locked ? "[比率維持]##lar" : "[XY独立]##lar"))
                        sp->lockAspectRatio = !sp->lockAspectRatio;
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip(locked ? "クリックでXY独立モードへ切り替え" : "クリックで比率維持モードへ切り替え");
                    ImGui::PopStyleColor(2);

                    ImGui::SetNextItemWidth(-1);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgBlue);

                    if (sp->lockAspectRatio) {
                        // 比率維持モード: X幅を基準にアスペクト比を保ちながらスケール変更
                        const float aspect = (sz.x > 0.0f) ? sz.y / sz.x : 1.0f;
                        float scaleW = sz.x;
                        if (ImGui::DragFloat("##bssz_w", &scaleW, 1.0f, 1.0f, 2000.0f, "W: %.1f")) {
                            scaleW = std::max(1.0f, scaleW);
                            sp->sprite->SetSize({scaleW, scaleW * aspect});
                        }
                        // 現在の高さとアスペクト比を補足表示
                        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                        ImGui::Text("H: %.1f  (比率 1 : %.3f)", sz.y, aspect);
                        ImGui::PopStyleColor();
                    } else {
                        // XY独立モード: W・H を個別に変更
                        float v[2] = {sz.x, sz.y};
                        if (ImGui::DragFloat2("##bssz", v, 1.f, 0.f, 2000.f))
                            sp->sprite->SetSize({v[0], v[1]});
                    }
                    ImGui::PopStyleColor();
                }
                ImGui::Spacing();

                // Color
                {
                    Vector4 c = sp->sprite->GetColor();
                    ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                    ImGui::TextUnformatted("カラー (R / G / B / A)");
                    ImGui::PopStyleColor();
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::ColorEdit4("##bscol", &c.x, ImGuiColorEditFlags_NoInputs)) {
                        sp->sprite->SetColor({c.x, c.y, c.z});
                        sp->sprite->SetAlpha(c.w);
                    }
                }
                ImGui::Spacing();

                // Rotation
                {
                    float rot = sp->sprite->GetRotation();
                    ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                    ImGui::TextUnformatted("回転 [rad] (スライダーでドラッグ)");
                    ImGui::PopStyleColor();
                    ImGui::SetNextItemWidth(-1);
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, DebugTheme::kAccentBlue);
                    if (ImGui::SliderAngle("##bsrot", &rot))
                        sp->sprite->SetRotation(rot);
                    ImGui::PopStyleColor();
                }
                ImGui::Spacing();

                // Blend mode
                {
                    static const char *bmNames[] = {
                        "なし", "通常", "加算", "減算", "乗算", "スクリーン"};
                    int bm = static_cast<int>(sp->blendMode);
                    ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                    ImGui::TextUnformatted("ブレンドモード");
                    ImGui::PopStyleColor();
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::Combo("##bsbm", &bm, bmNames, IM_ARRAYSIZE(bmNames)))
                        sp->blendMode = static_cast<BlendMode>(bm);
                }

                ImGui::TreePop();
            }
            ImGui::PopStyleColor(2);

            // ---- UV ----
            ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kBgOrange);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {1.0f, 0.60f, 0.10f, 0.20f});
            if (ImGui::TreeNodeEx("UV設定##uv", ImGuiTreeNodeFlags_SpanAvailWidth)) {
                Vector2 uvPos = sp->sprite->GetUVPosition();
                Vector2 uvSz = sp->sprite->GetUVSize();
                float uvRot = sp->sprite->GetUVRotate();
                bool changed = false;

                // UV Scale
                ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                ImGui::TextUnformatted("UVスケール (X / Y)");
                ImGui::PopStyleColor();
                float uvszV[2] = {uvSz.x, uvSz.y};
                ImGui::SetNextItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgOrange);
                if (ImGui::DragFloat2("##uvsc", uvszV, 0.01f, 0.1f, 10.f)) {
                    uvSz = {uvszV[0], uvszV[1]};
                    changed = true;
                }
                ImGui::PopStyleColor();

                // UV Rotation
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                ImGui::TextUnformatted("UV回転 [rad]");
                ImGui::PopStyleColor();
                ImGui::SetNextItemWidth(-1);
                if (ImGui::SliderAngle("##uvrt", &uvRot))
                    changed = true;

                // UV Position
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
                ImGui::TextUnformatted("UVオフセット (X / Y)");
                ImGui::PopStyleColor();
                float uvposV[2] = {uvPos.x, uvPos.y};
                ImGui::SetNextItemWidth(-1);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgOrange);
                if (ImGui::DragFloat2("##uvpos", uvposV, 0.01f, -2.f, 2.f)) {
                    uvPos = {uvposV[0], uvposV[1]};
                    changed = true;
                }
                ImGui::PopStyleColor();

                if (changed) {
                    sp->sprite->SetUVPosition(uvPos);
                    sp->sprite->SetUVSize(uvSz);
                    sp->sprite->SetUVRotate(uvRot);
                }

                ImGui::Spacing();
                if (ImGui::SmallButton("UVリセット##uvrs")) {
                    sp->sprite->SetUVPosition({0, 0});
                    sp->sprite->SetUVSize({1, 1});
                    sp->sprite->SetUVRotate(0);
                }
                ImGui::TreePop();
            }
            ImGui::PopStyleColor(2);

            ImGui::Unindent(6.0f);
            ImGui::Separator();
            ImGui::PopID();
        }
    }

    ImGui::Spacing();

    // ====================================================
    // ファイル操作
    // ====================================================
    SectionHeader("[ ファイル操作 ]", DebugTheme::kAccentOrange);

    // 説明テキスト
    ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
    ImGui::TextWrapped("スプライトは resources/jsons/Sprites/<フォルダ名> にJSONとして保存されます。");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // フォルダ名入力
    ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
    ImGui::TextUnformatted("保存/読み込みフォルダ名");
    ImGui::PopStyleColor();
    static char folderBuf[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgOrange);
    ImGui::InputText("##spfolder", folderBuf, sizeof(folderBuf));
    ImGui::PopStyleColor();
    saveFolder_ = folderBuf;

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, {0.20f, 0.45f, 0.20f, 0.80f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f, 0.55f, 0.25f, 0.90f});
    if (ImGui::Button("全スプライトを保存##spsvall", ImVec2(-1, 0)))
        SaveAllSprites();
    ImGui::Spacing();
    if (ImGui::Button("全スプライトを読み込み##spldall", ImVec2(-1, 0))) {
        Clear();
        LoadAllSprites();
    }
    ImGui::PopStyleColor(2);

    ImGui::Spacing();

    // 全削除
    ImGui::PushStyleColor(ImGuiCol_Button, {0.55f, 0.15f, 0.15f, 0.80f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.75f, 0.20f, 0.20f, 0.90f});
    if (ImGui::Button("全スプライトを削除##spdelall", ImVec2(-1, 0)))
        ImGui::OpenPopup("全削除の確認##spdelconfirm");
    ImGui::PopStyleColor(2);

    // 確認モーダル
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    if (ImGui::BeginPopupModal("全削除の確認##spdelconfirm", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentRed);
        ImGui::TextUnformatted("全スプライトを削除しますか？");
        ImGui::TextUnformatted("この操作は元に戻せません。");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float bw = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        ImGui::PushStyleColor(ImGuiCol_Button, {0.60f, 0.15f, 0.15f, 0.85f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.80f, 0.20f, 0.20f, 0.90f});
        if (ImGui::Button("削除##spdelok", ImVec2(bw, 0))) {
            Clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        if (ImGui::Button("キャンセル##spdelcancel", ImVec2(bw, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    ImGui::PopStyleVar(3);
#endif // _DEBUG
}

void SpriteManager::SetSaveFolder(const std::string &folderName) {
    saveFolder_ = folderName;
}

void SpriteManager::SaveDrawOrder() {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("Sprites/" + saveFolder_, "DrawOrder");

    // スプライト名の順序を保存
    for (size_t i = 0; i < sprites_.size(); ++i) {
        data->Save("sprite_" + std::to_string(i), sprites_[i]->name);
    }
    data->Save("sprite_count", static_cast<int>(sprites_.size()));
}

void SpriteManager::LoadDrawOrder() {
    // DrawOrder.jsonファイルが存在するかチェック
    std::string drawOrderPath = "resources/jsons/Sprites/" + saveFolder_ + "/DrawOrder.json";
    if (!fs::exists(drawOrderPath)) {
        return;
    }

    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("Sprites/" + saveFolder_, "DrawOrder");

    int spriteCount = data->Load<int>("sprite_count", 0);
    if (spriteCount == 0)
        return;

    std::vector<std::string> loadedOrder;
    for (int i = 0; i < spriteCount; ++i) {
        std::string spriteName = data->Load<std::string>("sprite_" + std::to_string(i), "");
        if (!spriteName.empty()) {
            loadedOrder.push_back(spriteName);
        }
    }

    // ロードした順序に基づいてスプライトを並び替え
    std::vector<std::unique_ptr<SpriteData>> reorderedSprites;

    for (const std::string &name : loadedOrder) {
        auto it = std::find_if(sprites_.begin(), sprites_.end(),
                               [&name](const std::unique_ptr<SpriteData> &sprite) {
                                   return sprite->name == name;
                               });
        if (it != sprites_.end()) {
            reorderedSprites.push_back(std::move(*it));
            sprites_.erase(it);
        }
    }

    // 順序リストに含まれなかった残りのスプライトを末尾に追加
    for (auto &sprite : sprites_) {
        if (sprite) {
            reorderedSprites.push_back(std::move(sprite));
        }
    }

    sprites_ = std::move(reorderedSprites);
}

void SpriteManager::SaveAllSprites() {
    SaveDrawOrder();

    std::string folderPath = "resources/jsons/Sprites/" + saveFolder_;
    if (!fs::exists(folderPath)) {
        fs::create_directories(folderPath);
    }

    for (const auto &spriteData : sprites_) {
        if (!spriteData || !spriteData->sprite)
            continue;

        std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("Sprites/" + saveFolder_, spriteData->name);

        data->Save("name", spriteData->name);
        data->Save("texturePath", spriteData->textureFilePath);

        Vector2 pos = spriteData->sprite->GetPosition();
        Vector2 size = spriteData->sprite->GetSize();
        Vector4 color = spriteData->sprite->GetColor();
        float rotation = spriteData->sprite->GetRotation();
        Vector2 anchor = spriteData->sprite->GetAnchorPoint();
        Matrix4x4 uvTransform = spriteData->sprite->GetUVTransform();

        data->Save("position", pos);
        data->Save("size", size);
        data->Save("color", color);
        data->Save("rotation", rotation);
        data->Save("anchor", anchor);
        data->Save("uvTransform", uvTransform);
        data->Save("blendMode", static_cast<int>(spriteData->blendMode));

        // アスペクト比ロック状態を保存
        data->Save("lockAspectRatio", static_cast<int>(spriteData->lockAspectRatio));
    }
    ImGuiNotification::Post("スプライトデータを保存しました: " + saveFolder_, {0.2f, 0.8f, 0.2f, 1.0f});
}

void SpriteManager::LoadAllSprites() {
    std::string folderPath = "resources/jsons/Sprites/" + saveFolder_;

    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        return;
    }

    std::vector<std::string> jsonNames;
    for (const auto &entry : fs::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".json" && entry.path().stem().string() != "DrawOrder") {
            jsonNames.push_back(entry.path().stem().string());
        }
    }

    for (const auto &name : jsonNames) {
        std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("Sprites/" + saveFolder_, name);

        std::string spriteName = data->Load<std::string>("name", "");
        std::string texturePath = data->Load<std::string>("texturePath", "");

        Vector2 position = data->Load<Vector2>("position", {0.0f, 0.0f});
        Vector2 size = data->Load<Vector2>("size", {300.0f, 300.0f});
        Vector4 color = data->Load<Vector4>("color", {1.0f, 1.0f, 1.0f, 1.0f});
        float rotation = data->Load<float>("rotation", 0.0f);
        Vector2 anchor = data->Load<Vector2>("anchor", {0.0f, 0.0f});
        Matrix4x4 uvTransform = data->Load<Matrix4x4>("uvTransform", MakeIdentity4x4());
        int blendModeInt = data->Load<int>("blendMode", static_cast<int>(BlendMode::kNormal));

        // アスペクト比ロック状態を復元（旧データには存在しないためデフォルトfalse）
        bool lockAspectRatio = static_cast<bool>(data->Load<int>("lockAspectRatio", 0));

        SpriteTransform transform;
        transform.position = position;
        transform.color = color;
        transform.anchorPoint = anchor;
        transform.instanceCount = 1;

        RegisterSprite(spriteName, texturePath, transform);

        auto sprite = GetSprite(spriteName);
        if (sprite && sprite->sprite) {
            sprite->sprite->SetSize(size);
            sprite->sprite->SetRotation(rotation);
            sprite->sprite->SetUVTransform(uvTransform);
            sprite->blendMode = static_cast<BlendMode>(blendModeInt);
            sprite->lockAspectRatio = lockAspectRatio;
        }
    }

    LoadDrawOrder();
    ImGuiNotification::Post("スプライトデータを読み込みました: " + saveFolder_, {0.2f, 0.8f, 0.8f, 1.0f});
}