#include "SpriteManager.h"
#include "WinApp.h"
#include "myMath.h"
#include <Data/DataHandler.h>
#include <ShowFolder/ShowFolder.h>
#include <filesystem>
namespace fs = std::filesystem;

SpriteManager *SpriteManager::instance = nullptr;

SpriteManager *SpriteManager::GetInstance() {
    if (instance == nullptr) {
        instance = new SpriteManager();
    }
    return instance;
}

void SpriteManager::Finalize() {
    delete instance;
    instance = nullptr;
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
}

void SpriteManager::UnregisterSprite(const std::string &name) {
    auto it = std::find_if(sprites_.begin(), sprites_.end(),
                           [&name](const std::unique_ptr<SpriteData> &sprite) {
                               return sprite->name == name;
                           });

    if (it != sprites_.end()) {
        sprites_.erase(it);
    }
}

void SpriteManager::DrawAll() {
    for (auto &spriteData : sprites_) {
        if (spriteData->isVisible) {
            spriteData->sprite->Draw(spriteData->isBackMost);
        }
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
            transform.scale = {0.0f, 0.0f, 1.0f}; // スケールを0にして非表示
            // または transform.translate.z = -1000.0f; // 画面外に配置
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

        // 元のインデックスをそのまま使用
        spriteData->sprite->SetInstanceTransform(i, transformMatrix);
    }
}

void SpriteManager::DrawSpriteCreationModal() {
#ifdef _DEBUG
    // メニューから呼び出された場合のモーダル表示
    if (showSpriteCreationModal_) {
        ImGui::OpenPopup("スプライト生成");
        showSpriteCreationModal_ = false;
    }

    // スプライト生成モーダルウィンドウ
    if (ImGui::BeginPopupModal("スプライト生成", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("新しいスプライトを作成します");

        static char spriteNameBuffer[128] = "";
        static SpriteTransform transform;
        static bool initialized = false;

        // 初回時の初期化
        if (!initialized) {
            transform = SpriteTransform(); // デフォルト値で初期化
            initialized = true;
        }

        // スプライト名入力欄
        ImGui::InputText("スプライト名", spriteNameBuffer, IM_ARRAYSIZE(spriteNameBuffer));

        ImGui::Separator();

        // テクスチャファイル選択セクション
        ImGui::Text("テクスチャファイル選択:");
        ImGui::BeginChild("TextureFileSelector", ImVec2(600, 300), true);
        ShowTextureFile(texturePath_);
        ImGui::EndChild();

        ImGui::Separator();

        // スプライト設定セクション
        ImGui::Text("スプライト設定:");

        // 位置設定
        ImGui::DragFloat2("位置", &transform.position.x, 1.0f);

        // 色設定
        ImGui::ColorEdit4("色", &transform.color.x);

        // アンカーポイント設定
        ImGui::DragFloat2("アンカーポイント", &transform.anchorPoint.x, 0.0f, 1.0f);

        // フリップ設定
        ImGui::Checkbox("左右反転", &transform.isFlipX);
        ImGui::SameLine();
        ImGui::Checkbox("上下反転", &transform.isFlipY);

        // インスタンス数設定
        ImGui::InputScalar("インスタンス数", ImGuiDataType_U32, &transform.instanceCount, nullptr, nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal);
        if (transform.instanceCount < 1)
            transform.instanceCount = 1;
        if (transform.instanceCount > 1000)
            transform.instanceCount = 1000; // 最大値制限

        ImGui::Separator();

        // 選択状況の表示
        ImGui::Text("選択されたテクスチャ: %s", texturePath_.empty() ? "未選択" : texturePath_.c_str());

        ImGui::Separator();

        // 生成ボタンとキャンセルボタン
        bool canCreate = strlen(spriteNameBuffer) > 0 && !texturePath_.empty();

        // 既に同じ名前のスプライトが存在するかチェック
        bool nameExists = (GetSprite(spriteNameBuffer) != nullptr);
        if (nameExists) {
            canCreate = false;
        }

        if (!canCreate) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        }

        if (ImGui::Button("生成", ImVec2(120, 0))) {
            if (canCreate) {
                RegisterSprite(spriteNameBuffer, texturePath_, transform);

                // 入力欄とパスをリセット
                memset(spriteNameBuffer, 0, sizeof(spriteNameBuffer));
                texturePath_ = "";
                transform = SpriteTransform(); // デフォルト値にリセット
                initialized = false;
                ImGui::CloseCurrentPopup();
            }
        }

        if (!canCreate) {
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            // 入力欄とパスをリセット
            memset(spriteNameBuffer, 0, sizeof(spriteNameBuffer));
            texturePath_ = "";
            transform = SpriteTransform(); // デフォルト値にリセット
            initialized = false;
            ImGui::CloseCurrentPopup();
        }

        // 生成できない場合の理由を表示
        if (!canCreate) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "生成するには:");
            if (strlen(spriteNameBuffer) == 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "・スプライト名を入力してください");
            }
            if (texturePath_.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "・テクスチャファイルを選択してください");
            }
            if (nameExists) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "・同じ名前のスプライトが既に存在します");
            }
        }

        ImGui::EndPopup();
    }
#endif // _DEBUG
}

// スプライト管理ウィンドウの描画
void SpriteManager::DrawSpriteManager() {
#ifdef _DEBUG
    // 新規作成ボタン
    if (ImGui::Button("新しいスプライトを作成", ImVec2(200, 30))) {
        ShowSpriteCreationModal();
    }

    ImGui::Separator();

    // 登録されているスプライトのリスト
    ImGui::Text("登録済みスプライト (%zu個):", sprites_.size());

    if (sprites_.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "スプライトが登録されていません");
    } else {
        // 描画順変更セクション
        ImGui::Text("描画順 (上が手前、下が奥):");

        // スプライトリストをより見やすく表示
        if (ImGui::BeginTable("SpriteTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("順序", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("表示", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("数", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("テクスチャ", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            // 削除予定のスプライト名を記録
            std::vector<std::string> spritesToDelete;

            for (size_t i = 0; i < sprites_.size(); ++i) {
                auto &managedSprite = sprites_[i];
                ImGui::TableNextRow();

                // 順序表示と移動ボタン
                ImGui::TableNextColumn();
                ImGui::Text("%zu", i + 1);

                // 上に移動ボタン
                if (i > 0) {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton(("##up_" + managedSprite->name).c_str(), ImGuiDir_Up)) {
                        std::swap(sprites_[i], sprites_[i - 1]);
                    }
                }

                // 下に移動ボタン
                if (i < sprites_.size() - 1) {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton(("##down_" + managedSprite->name).c_str(), ImGuiDir_Down)) {
                        std::swap(sprites_[i], sprites_[i + 1]);
                    }
                }

                // 名前
                ImGui::TableNextColumn();
                ImGui::Text("%s", managedSprite->name.c_str());

                // 表示/非表示切り替え
                ImGui::TableNextColumn();
                bool visible = managedSprite->isVisible;
                if (ImGui::Checkbox(("##visible_" + managedSprite->name).c_str(), &visible)) {
                    managedSprite->isVisible = visible;
                }

                // インスタンス数
                ImGui::TableNextColumn();
                ImGui::Text("%zu", managedSprite->instanceData.size());

                // テクスチャパス（短縮表示）
                ImGui::TableNextColumn();
                std::string shortPath = managedSprite->textureFilePath;
                if (shortPath.length() > 30) {
                    shortPath = "..." + shortPath.substr(shortPath.length() - 27);
                }
                ImGui::Text("%s", shortPath.c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", managedSprite->textureFilePath.c_str());
                }

                // 操作ボタン
                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button(("削除##delete_" + managedSprite->name).c_str(), ImVec2(60, 0))) {
                    spritesToDelete.push_back(managedSprite->name);
                }
                ImGui::PopStyleColor();
            }

            // 削除処理
            for (const std::string &nameToDelete : spritesToDelete) {
                UnregisterSprite(nameToDelete);
            }

            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::Text("詳細設定:");

        // 各スプライトの詳細設定を縦レイアウトで表示
        for (auto &managedSprite : sprites_) {
            if (ImGui::CollapsingHeader(managedSprite->name.c_str())) {
                ImGui::PushID(managedSprite->name.c_str());

                // 基本設定セクション
                if (ImGui::TreeNode("基本設定")) {
                    // 位置設定
                    Vector2 spritePos = managedSprite->sprite->GetPosition();
                    ImGui::Text("位置:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::DragFloat("##pos_x", &spritePos.x, 1.0f)) {
                        managedSprite->sprite->SetPosition(spritePos);
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::DragFloat("##pos_y", &spritePos.y, 1.0f)) {
                        managedSprite->sprite->SetPosition(spritePos);
                    }

                    // サイズ設定
                    Vector2 spriteSize = managedSprite->sprite->GetSize();
                    ImGui::Text("サイズ:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::DragFloat("##size_x", &spriteSize.x, 1.0f, 0.0f, 2000.0f)) {
                        managedSprite->sprite->SetSize(spriteSize);
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::DragFloat("##size_y", &spriteSize.y, 1.0f, 0.0f, 2000.0f)) {
                        managedSprite->sprite->SetSize(spriteSize);
                    }

                    // 色設定
                    Vector4 spriteColor = managedSprite->sprite->GetColor();
                    ImGui::Text("色:");
                    if (ImGui::ColorEdit4("##color", &spriteColor.x, ImGuiColorEditFlags_NoInputs)) {
                        managedSprite->sprite->SetColor({spriteColor.x, spriteColor.y, spriteColor.z});
                        managedSprite->sprite->SetAlpha(spriteColor.w);
                    }

                    // 回転設定
                    float rotation = managedSprite->sprite->GetRotation();
                    ImGui::Text("回転:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::SliderAngle("##rotation", &rotation)) {
                        managedSprite->sprite->SetRotation(rotation);
                    }

                    ImGui::TreePop();
                }

                // UV設定セクション
                if (ImGui::TreeNode("UV設定")) {
                    Vector2 uvPosition = managedSprite->sprite->GetUVPosition();
                    Vector2 uvSize = managedSprite->sprite->GetUVSize();
                    float uvRotation = managedSprite->sprite->GetUVRotate();

                    bool uvChanged = false;

                    // UVスケール
                    ImGui::Text("UVスケール:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::DragFloat("##uv_scale_x", &uvSize.x, 0.01f, 0.1f, 10.0f)) {
                        uvChanged = true;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::DragFloat("##uv_scale_y", &uvSize.y, 0.01f, 0.1f, 10.0f)) {
                        uvChanged = true;
                    }

                    // UV回転
                    ImGui::Text("UV回転:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::SliderAngle("##uv_rotation", &uvRotation)) {
                        uvChanged = true;
                    }

                    // UV位置
                    ImGui::Text("UV位置:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::DragFloat("##uv_pos_x", &uvPosition.x, 0.01f, -2.0f, 2.0f)) {
                        uvChanged = true;
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::DragFloat("##uv_pos_y", &uvPosition.y, 0.01f, -2.0f, 2.0f)) {
                        uvChanged = true;
                    }

                    if (uvChanged) {
                        managedSprite->sprite->SetUVPosition(uvPosition);
                        managedSprite->sprite->SetUVSize(uvSize);
                        managedSprite->sprite->SetUVRotate(uvRotation);
                    }

                    // UVリセットボタン
                    if (ImGui::Button("UVリセット", ImVec2(100, 0))) {
                        managedSprite->sprite->SetUVPosition({0.0f, 0.0f});
                        managedSprite->sprite->SetUVSize({1.0f, 1.0f});
                        managedSprite->sprite->SetUVRotate(0.0f);
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }
    }

    // 保存・ロード関連のUIを整理
    ImGui::Separator();
    ImGui::Text("ファイル操作");
    ImGui::Separator();

    static char folderBuffer[128] = "";
    ImGui::Text("保存フォルダ:");
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##folder", folderBuffer, sizeof(folderBuffer));
    saveFolder_ = folderBuffer;

    ImGui::Spacing();

    // ボタンを縦に配置
    if (ImGui::Button("全スプライトをセーブ", ImVec2(200, 30))) {
        SaveAllSprites();
    }

    if (ImGui::Button("全スプライトをロード", ImVec2(200, 30))) {
        Clear();
        LoadAllSprites();
    }

    ImGui::Spacing();

    // 危険な操作は色を変える
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("全スプライト削除", ImVec2(200, 30))) {
        ImGui::OpenPopup("全削除確認");
    }
    ImGui::PopStyleColor();

    // 全削除確認ダイアログ
    if (ImGui::BeginPopupModal("全削除確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("全てのスプライトを削除しますか？");
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "この操作は取り消せません。");
        ImGui::Separator();

        if (ImGui::Button("削除する", ImVec2(120, 0))) {
            Clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
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
        // ファイルが存在しない場合は何もしない（現在の順序を維持）
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

    // まず、保存された順序通りに追加
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

    // 残りのスプライト（新規追加されたもの）を末尾に追加
    for (auto &sprite : sprites_) {
        if (sprite) {
            reorderedSprites.push_back(std::move(sprite));
        }
    }

    sprites_ = std::move(reorderedSprites);
}

void SpriteManager::SaveAllSprites() {
    SaveDrawOrder();
    // 保存先フォルダのパスを組み立て
    std::string folderPath = "resources/jsons/Sprites/" + saveFolder_;

    // フォルダが存在しない場合は作る
    if (!fs::exists(folderPath)) {
        fs::create_directories(folderPath);
    }

    for (const auto &spriteData : sprites_) {
        if (!spriteData || !spriteData->sprite)
            continue;

        std::string texturePath = spriteData->textureFilePath;
        std::string textureFilename = fs::path(texturePath).stem().string();
        std::string fileName = textureFilename;

        std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("Sprites/" + saveFolder_, spriteData->name);

        data->Save("name", spriteData->name);
        data->Save("texturePath", texturePath);

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
    }
}

void SpriteManager::LoadAllSprites() {
    std::string folderPath = "resources/jsons/Sprites/" + saveFolder_;

    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        return;
    }

    // 既存の読み込み処理...（そのまま）
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
        }
    }

    // 描画順をロード
    LoadDrawOrder();
}