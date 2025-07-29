#include "BaseObjectManager.h"
#ifdef _DEBUG
#include "Debug/ImGui/ImGuizmoManager.h"
#endif // _DEBUG
#include <ShowFolder/ShowFolder.h>
#include <Debug/Log/Logger.h>

BaseObjectManager *BaseObjectManager::instance = nullptr;

BaseObjectManager *BaseObjectManager::GetInstance() {
    if (instance == nullptr) {
        instance = new BaseObjectManager();
    }
    return instance;
}

void BaseObjectManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void BaseObjectManager::RemoveAllObjects() {
    // 親子関係をすべてクリア
    for (auto &pair : baseObjects_) {
        BaseObject *obj = pair.second.get();
        if (obj) {
            obj->SetParent(nullptr);
        }
    }

    // すべてのオブジェクトを削除
    baseObjects_.clear();

// ImGuizmoManagerもクリア
#ifdef _DEBUG
    ImGuizmoManager::GetInstance()->DeleteTarget();
#endif
}

void BaseObjectManager::AddObject(std::unique_ptr<BaseObject> baseObject) {
    const std::string &name = baseObject->GetName();
#ifdef _DEBUG
    ImGuizmoManager::GetInstance()->AddTarget(baseObject->GetName(), baseObject.get());
#endif // _DEBUG
    baseObjects_.emplace(name, std::move(baseObject));
}

void BaseObjectManager::Update() {
    for (auto &[name, obj] : baseObjects_) {
        obj->UpdateHierarchy();
        obj->UpdateWorldTransformHierarchy();
    }
}

void BaseObjectManager::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    for (auto &[name, obj] : baseObjects_) {
        obj->Draw(viewProjection, offSet);
    }
}

void BaseObjectManager::UpdateImGui() {
#ifdef _DEBUG
    DrawSceneSaveModel();
    DrawSceneLoadModel();
    DrawObjectCreationModel();
    DrawObjectLoadModel();
#endif // _DEBUG
}

void BaseObjectManager::SaveAll() {
    for (auto &[name, obj] : baseObjects_) {
        obj->SetFolderPath("SceneData/" + sceneName_ + "/ObjectDatas");
        obj->SceneSaveToJson();
        obj->SaveParentChildRelationship(); 
    }
}

void BaseObjectManager::LoadAll(std::string sceneName) {
    // シーンデータのフォルダパスを構築
    std::string sceneDataPath = "Resources/jsons/SceneData/" + sceneName + "/ObjectDatas";

    // フォルダが存在するかチェック
    if (!std::filesystem::exists(sceneDataPath)) {
        // フォルダが存在しない場合は何もしない
        return;
    }

    // JSONファイルを検索
    std::vector<std::string> jsonFiles;
    for (const auto &entry : std::filesystem::directory_iterator(sceneDataPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            jsonFiles.push_back(entry.path().filename().string());
        }
    }

    // 既存のオブジェクトをクリア
    RemoveAllObjects();

    // 各JSONファイルを読み込んでオブジェクトを生成
    for (const std::string &jsonFile : jsonFiles) {
        // JSONファイル名から拡張子を除去してオブジェクト名とする
        std::string objectName = jsonFile.substr(0, jsonFile.find_last_of('.'));

        // 新しいオブジェクトを作成
        std::unique_ptr<BaseObject> newObject = std::make_unique<BaseObject>();

        // フォルダパスを設定
        newObject->SetFolderPath("SceneData/" + sceneName + "/ObjectDatas");

        // オブジェクト名でInit
        newObject->Init(objectName);

        // 読み込んだデータからモデルとテクスチャのパスを取得
        std::string modelPath = newObject->GetModelPath();
        std::string texturePath = newObject->GetTexturePath();

        // モデルとテクスチャを設定
        if (!modelPath.empty()) {
            newObject->CreateModel(modelPath);
        } else {
            newObject->CreatePrimitiveModel(newObject->GetPrimitiveType());
        }

        if (!texturePath.empty()) {
            newObject->SetTexture(texturePath, 0);
        }

        // オブジェクトマネージャーに追加
        this->AddObject(std::move(newObject));
    }

    // 全オブジェクト読み込み後に親子関係を復元
    LoadAllParentChildRelationships();
}

void BaseObjectManager::CreateObject(std::string objectName, std::string modelPath, std::string texturePath) {
    std::unique_ptr<BaseObject> newObject = std::make_unique<BaseObject>();
    newObject->Init(objectName);
    newObject->CreateModel(modelPath);
    newObject->SetTexture(texturePath, 0);
    this->AddObject(std::move(newObject));
}

BaseObject *BaseObjectManager::GetObjectByName(const std::string &name) {
    auto it = baseObjects_.find(name);
    if (it != baseObjects_.end()) {
        return it->second.get();
    }
    return nullptr;
}

// メニューからモーダルを開くメソッド
void BaseObjectManager::OpenSceneSaveModal() {
    showSceneSaveModal_ = true;
}

void BaseObjectManager::OpenSceneLoadModal() {
    showSceneLoadModal_ = true;
}

void BaseObjectManager::OpenObjectCreationModal() {
    showObjectCreationModal_ = true;
}

void BaseObjectManager::OpenObjectLoadModal() {
    showObjectLoadModal_ = true;
}

void BaseObjectManager::ShowParentChildHierarchy() {
#ifdef _DEBUG

    if (ImGui::CollapsingHeader("親子関係設定", ImGuiTreeNodeFlags_DefaultOpen)) {

        // 親子付けセクション
        ImGui::Separator();
        ImGui::Text("親子付け:");

        static int selectedChild = 0;
        static int selectedParent = 0;

        std::vector<std::string> objectNames = GetObjectNames();

        if (!objectNames.empty()) {
            std::vector<const char *> objectNamesCStr;
            for (const auto &name : objectNames) {
                objectNamesCStr.push_back(name.c_str());
            }

            ImGui::Text("子オブジェクト:");
            ImGui::SameLine();
            ImGui::PushItemWidth(150);
            ImGui::Combo("##ChildObject", &selectedChild, objectNamesCStr.data(), static_cast<int>(objectNamesCStr.size()));
            ImGui::PopItemWidth();

            ImGui::Text("親オブジェクト:");
            ImGui::SameLine();
            ImGui::PushItemWidth(150);
            ImGui::Combo("##ParentObject", &selectedParent, objectNamesCStr.data(), static_cast<int>(objectNamesCStr.size()));
            ImGui::PopItemWidth();

            selectedChild = std::clamp(selectedChild, 0, static_cast<int>(objectNames.size()) - 1);
            selectedParent = std::clamp(selectedParent, 0, static_cast<int>(objectNames.size()) - 1);

            if (ImGui::Button("親子付け")) {
                if (selectedChild != selectedParent) {
                    SetParentChild(objectNames[selectedChild], objectNames[selectedParent]);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("親子解除")) {
                RemoveParentChild(objectNames[selectedChild]);
            }
        }

        ImGui::Separator();
        ImGui::Text("階層表示:");

        // 階層構造を表示
        ImGui::BeginChild("HierarchyView", ImVec2(0, 300), true);

        for (auto &[name, obj] : baseObjects_) {
            if (!obj->GetParent()) { // ルートオブジェクトのみ表示
                ShowObjectHierarchy(obj.get(), 0);
            }
        }

        ImGui::EndChild();
    }
#endif // _DEBUG
}

void BaseObjectManager::ShowObjectHierarchy(BaseObject *obj, int depth) {
#ifdef _DEBUG

    if (!obj)
        return;

    // インデントを設定
    std::string indent(depth * 2, ' ');
    std::string displayName = indent + obj->GetName();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    // 子がない場合は葉ノードフラグを追加
    if (obj->GetChildren()->empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool nodeOpen = ImGui::TreeNodeEx(displayName.c_str(), flags);

    if (nodeOpen) {
        // 子オブジェクトを表示
        for (BaseObject *child : *obj->GetChildren()) {
            ShowObjectHierarchy(child, depth + 1);
        }
        ImGui::TreePop();
    }
#endif // _DEBUG
}

void BaseObjectManager::SetParentChild(const std::string &childName, const std::string &parentName) {
    BaseObject *child = GetObjectByName(childName);
    BaseObject *parent = GetObjectByName(parentName);

    if (child && parent && child != parent) {
        // 循環参照チェック
        BaseObject *currentParent = parent;
        while (currentParent) {
            if (currentParent == child) {
                // 循環参照が発生するため、親子付けを拒否
                return;
            }
            currentParent = currentParent->GetParent();
        }

        child->SetParent(parent);
    }
}

void BaseObjectManager::RemoveParentChild(const std::string &childName) {
    BaseObject *child = GetObjectByName(childName);
    if (child) {
        child->DetachParent();
    }
}

std::vector<std::string> BaseObjectManager::GetObjectNames() const {
    std::vector<std::string> names;
    for (const auto &[name, obj] : baseObjects_) {
        names.push_back(name);
    }
    return names;
}

void BaseObjectManager::SaveAllParentChildRelationships() {
    for (auto &[name, obj] : baseObjects_) {
        obj->SaveParentChildRelationship();
    }
}

void BaseObjectManager::LoadAllParentChildRelationships() {
    // まず全オブジェクトから親子関係情報を読み込む
    std::unordered_map<std::string, std::string> parentRelations;
    std::unordered_map<std::string, std::vector<std::string>> childRelations;

    for (auto &[name, obj] : baseObjects_) {
        if (!obj->ObjectDatas_)
            continue;

        std::string parentName = obj->ObjectDatas_->Load<std::string>("parentName", "");
        if (!parentName.empty()) {
            parentRelations[name] = parentName;
        }

        std::vector<std::string> childrenNames = obj->ObjectDatas_->Load<std::vector<std::string>>("childrenNames", std::vector<std::string>());
        if (!childrenNames.empty()) {
            childRelations[name] = childrenNames;
        }
    }

    // 親子関係を復元
    for (const auto &[childName, parentName] : parentRelations) {
        BaseObject *child = GetObjectByName(childName);
        BaseObject *parent = GetObjectByName(parentName);

        if (child && parent) {
            child->SetParent(parent);
        }
    }
}

void BaseObjectManager::RemoveObject(const std::string &name) {
    auto it = baseObjects_.find(name);
    if (it != baseObjects_.end()) {
        BaseObject *targetObject = it->second.get();

        // 親子関係の処理
        if (targetObject) {
            // 削除するオブジェクトの子オブジェクトの親を解除
            for (auto &pair : baseObjects_) {
                BaseObject *obj = pair.second.get();
                if (obj && obj->GetParent() == targetObject) {
                    obj->SetParent(nullptr);
                }
            }

            // 削除するオブジェクトの親からも解除
            if (targetObject->GetParent()) {
                targetObject->SetParent(nullptr);
            }
        }
        // オブジェクトを削除
        baseObjects_.erase(it);
    }
}

// シーン保存モーダルの描画
void BaseObjectManager::DrawSceneSaveModel() {
#ifdef _DEBUG
    // メニューから呼び出された場合のモーダル表示
    if (showSceneSaveModal_) {
        ImGui::OpenPopup("シーン保存");
        showSceneSaveModal_ = false;
    }

    // モーダルウィンドウ（中央に表示、背景は自動で薄暗くなる）
    if (ImGui::BeginPopupModal("シーン保存", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("シーンの名前を入力してください");

        static char sceneNameBuffer[128] = "";

        // テキスト入力欄（sceneName_ を編集）
        ImGui::InputText("シーン名", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer));

        // 横並びに「保存」ボタンと「キャンセル」ボタン
        if (ImGui::Button("保存", ImVec2(120, 0))) {
            sceneName_ = sceneNameBuffer;      // 入力内容を保存
            SaveAll();                         // 実際の保存処理
            SaveAllParentChildRelationships(); // 親子関係も保存
            ImGui::CloseCurrentPopup();        // モーダルを閉じる
            sceneName_.clear();                // 入力欄をクリア
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup(); // キャンセル時も閉じる
        }

        ImGui::EndPopup();
    }
#endif // _DEBUG
}

// シーン読み込みモーダルの描画
void BaseObjectManager::DrawSceneLoadModel() {
#ifdef _DEBUG
    // メニューから呼び出された場合のモーダル表示
    if (showSceneLoadModal_) {
        ImGui::OpenPopup("シーン読み込み");
        showSceneLoadModal_ = false;
    }

    if (ImGui::BeginPopupModal("シーン読み込み", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("シーンの名前を入力してください");

        static char sceneNameBuffer[128] = "";

        // テキスト入力欄（sceneName_ を編集）
        ImGui::InputText("シーン名", sceneNameBuffer, IM_ARRAYSIZE(sceneNameBuffer));

        // 横並びに「読み込み」ボタンと「キャンセル」ボタン
        if (ImGui::Button("読み込み", ImVec2(120, 0))) {
            sceneName_ = sceneNameBuffer;      // 入力内容を保存
            LoadAll(sceneName_);               // 実際の読み込み処理
            LoadAllParentChildRelationships(); // 親子関係も読み込み
            ImGui::CloseCurrentPopup();        // モーダルを閉じる
            sceneName_.clear();                // 読み込み後はシーン名をクリア
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup(); // キャンセル時も閉じる
        }

        ImGui::EndPopup();
    }
#endif // _DEBUG
}

// オブジェクト生成モーダルの描画
void BaseObjectManager::DrawObjectCreationModel() {
#ifdef _DEBUG
    // メニューから呼び出された場合のモーダル表示
    if (showObjectCreationModal_) {
        ImGui::OpenPopup("オブジェクト生成");
        showObjectCreationModal_ = false;
    }

    // オブジェクト生成モーダルウィンドウ
    if (ImGui::BeginPopupModal("オブジェクト生成", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("新しいオブジェクトを作成します");

        static char objectNameBuffer[128] = "";

        // オブジェクト名入力欄
        ImGui::InputText("オブジェクト名", objectNameBuffer, IM_ARRAYSIZE(objectNameBuffer));

        ImGui::Separator();

        // モデルファイル選択セクション
        ImGui::Text("モデルファイル選択:");
        ImGui::BeginChild("ModelFileSelector", ImVec2(600, 300), true);
        ShowModelFile(modelPath_);
        ImGui::EndChild();

        ImGui::Separator();

        // テクスチャファイル選択セクション
        ImGui::Text("テクスチャファイル選択 (オプション):");
        ImGui::BeginChild("TextureFileSelector", ImVec2(600, 300), true);
        ShowTextureFile(texturePath_);
        ImGui::EndChild();

        ImGui::Separator();

        // 選択状況の表示
        ImGui::Text("選択されたモデル: %s", modelPath_.empty() ? "未選択" : modelPath_.c_str());
        ImGui::Text("選択されたテクスチャ: %s", texturePath_.empty() ? "未選択" : texturePath_.c_str());

        ImGui::Separator();

        // 生成ボタンとキャンセルボタン
        bool canCreate = strlen(objectNameBuffer) > 0 && !modelPath_.empty();

        if (!canCreate) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        }

        if (ImGui::Button("生成", ImVec2(120, 0))) {
            if (canCreate) {
                objectName_ = objectNameBuffer;
                CreateObject(objectName_, modelPath_, texturePath_);

                // 入力欄とパスをリセット
                memset(objectNameBuffer, 0, sizeof(objectNameBuffer));
                modelPath_ = "";
                texturePath_ = "";

                ImGui::CloseCurrentPopup();
            }
        }

        if (!canCreate) {
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            // 入力欄とパスをリセット
            memset(objectNameBuffer, 0, sizeof(objectNameBuffer));
            modelPath_ = "";
            texturePath_ = "";

            ImGui::CloseCurrentPopup();
        }

        // 生成できない場合の理由を表示
        if (!canCreate) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "生成するには:");
            if (strlen(objectNameBuffer) == 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "・オブジェクト名を入力してください");
            }
            if (modelPath_.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "・モデルファイルを選択してください");
            }
        }

        ImGui::EndPopup();
    }
#endif // _DEBUG
}

void BaseObjectManager::DrawObjectLoadModel() {
#ifdef _DEBUG
    // メニューから呼び出された場合のモーダル表示
    if (showObjectLoadModal_) {
        ImGui::OpenPopup("保存済みオブジェクト呼び出し");
        showObjectLoadModal_ = false;
    }
    std::string startPath = "ObjectDatas";
    // オブジェクト呼び出しモーダルウィンドウ
    if (ImGui::BeginPopupModal("保存済みオブジェクト呼び出し", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("保存済みのオブジェクトを読み込みます");

        // JSONファイル選択セクション
        ImGui::Text("保存済みオブジェクト選択:");
        ImGui::BeginChild("JsonFileSelector", ImVec2(600, 400), true);
        ShowJsonFile(selectedJsonPath_, startPath);
        ImGui::EndChild();
        ImGui::Separator();

        // 選択状況の表示とオブジェクト名の自動設定
        ImGui::Text("選択されたファイル: %s", selectedJsonPath_.empty() ? "未選択" : selectedJsonPath_.c_str());

        // JSONファイルが選択されている場合、ファイル名からオブジェクト名を取得
        std::string autoObjectName = "";
        if (!selectedJsonPath_.empty()) {
            std::filesystem::path jsonPath(selectedJsonPath_);
            autoObjectName = jsonPath.stem().string(); // 拡張子なしのファイル名を取得
            ImGui::Text("オブジェクト名: %s", autoObjectName.c_str());
        }

        ImGui::Separator();

        // 読み込みボタンとキャンセルボタン
        bool canLoad = !selectedJsonPath_.empty();
        if (!canLoad) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        }
        if (ImGui::Button("読み込み", ImVec2(120, 0))) {
            if (canLoad) {
                LoadObjectFromJson(startPath, autoObjectName);
                // パスをリセット
                selectedJsonPath_ = "";
                ImGui::CloseCurrentPopup();
            }
        }
        if (!canLoad) {
            ImGui::PopStyleColor(3);
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
            // パスをリセット
            selectedJsonPath_ = "";
            ImGui::CloseCurrentPopup();
        }
        // 読み込みできない場合の理由を表示
        if (!canLoad) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "読み込みするには:");
            if (selectedJsonPath_.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "・JSONファイルを選択してください");
            }
        }
        ImGui::EndPopup();
    }
#endif // _DEBUG
}

void BaseObjectManager::LoadObjectFromJson(const std::string &startPath, const std::string &objectName) {
    // フルパスを構築 (startPath/objectName.json)
    std::string fullPath = "resources/jsons/" + startPath + "/" + objectName + ".json";

    // BaseObjectのLoadFromJson機能を使用してオブジェクトを作成
    auto newObject = std::make_unique<BaseObject>();
    newObject->Init(objectName);
    newObject->GetName() = objectName;
    newObject->LoadFromJson(startPath, objectName);
    if (!newObject->GetModelPath().empty()) {
        newObject->CreateModel(newObject->GetModelPath());
    } else {
        newObject->CreatePrimitiveModel(newObject->GetPrimitiveType());
    }

    // 親子関係の復元
    RestoreParentChildRelationshipForObject(newObject.get());

    this->AddObject(std::move(newObject));
    Logger::Log("オブジェクト読み込み完了: " + objectName + " (" + fullPath + ")");
}

void BaseObjectManager::RestoreParentChildRelationshipForObject(BaseObject *object) {
    if (!object)
        return;

    // 親の復元
    std::string parentName = object->GetParentName();
    if (!parentName.empty()) {
        auto it = baseObjects_.find(parentName);
        if (it != baseObjects_.end()) {
            object->SetParent(it->second.get());
        }
    }

    // 子の復元
    std::vector<std::string> childrenNames = object->GetChildrenNames();
    for (const std::string &childName : childrenNames) {
        auto it = baseObjects_.find(childName);
        if (it != baseObjects_.end()) {
            object->AddChild(it->second.get());
        }
    }
}

void BaseObjectManager::DrawImGui() {
#ifdef _DEBUG
    ImGui::Begin("階層エディター");

    ShowParentChildHierarchy();

    ImGui::End();
#endif // _DEBUG
}
