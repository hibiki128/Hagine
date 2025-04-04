#include "ParticleEditor.h"

ParticleEditor *ParticleEditor::instance = nullptr;

ParticleEditor *ParticleEditor::GetInstance() {
    if (instance == nullptr) {
        instance = new ParticleEditor();
    }
    return instance;
}

void ParticleEditor::Finalize() {
    delete instance;
    instance = nullptr;
}

void ParticleEditor::Initialize() {
}

void ParticleEditor::AddParticleEmitter(const std::string &name, const std::string &fileName, const std::string &texturePath) {
    // 新しい ParticleEmitter を作成
    auto emitter = std::make_unique<ParticleEmitter>();

    // 初期化処理
    emitter->Initialize(name, fileName);
    if (!texturePath.empty()) {
        emitter->CreateParticle(name, fileName, texturePath);
    } else {
        if (!isLoad_) {
            emitter->CreateParticle(name, fileName, emitter->GetTexturePath());
        }
    }
    // マップに追加
    emitters_[name] = std::move(emitter);
}

void ParticleEditor::Load() {
}

void ParticleEditor::AddParticleEmitter(const std::string &name) {
    // 新しい ParticleEmitter を作成
    auto emitter = std::make_unique<ParticleEmitter>();

    // 初期化処理
    emitter->Initialize(name);
    // マップに追加
    emitters_[name] = std::move(emitter);
}

void ParticleEditor::EditorWindow() {
    ImGui::Begin("パーティクルエディター");
    ShowImGuiEditor();
    ImGui::End();
}

void ParticleEditor::DrawAll(const ViewProjection &vp_) {
    for (auto &[name, emitter] : emitters_) {
        if (emitter) {
            emitter->Draw(vp_);
        }
    }
}

void ParticleEditor::DebugAll() {
    for (auto &[name, emitter] : emitters_) {
        if (emitter) {
            emitter->Debug();
        }
    }
}

void ParticleEditor::ShowImGuiEditor() {
    if (ImGui::BeginTabBar("パーティクル")) {
        if (ImGui::BeginTabItem("パーティクル作成")) {

            // 名前の入力
            char nameBuffer[256];
            strcpy_s(nameBuffer, sizeof(nameBuffer), localName_.c_str());
            ImGui::Text("パーティクルの名前");
            if (ImGui::InputText(" ", nameBuffer, sizeof(nameBuffer))) {
                localName_ = std::string(nameBuffer);
            }

            // モデルとテクスチャの処理を分ける
            ImGui::Spacing();

            // モデル選択アイテム
            if (ImGui::CollapsingHeader("モデル")) {
                // モデルファイル選択
                static std::filesystem::path baseDirObj = "resources/models/";
                static std::filesystem::path currentDirObj = "resources/models";
                static std::string selectedFolderObj = "";
                static std::string selectedFileObj = "";

                // 「戻る」ボタン（上の階層に戻る）
                if (currentDirObj != "resources/models") {
                    if (ImGui::Button("< 戻る(Model)")) {
                        currentDirObj = currentDirObj.parent_path();
                        selectedFolderObj = "";
                        selectedFileObj = "";
                    }
                }

                // フォルダ一覧
                std::vector<std::string> foldersObj;
                std::vector<std::string> objFiles;

                for (const auto &entry : std::filesystem::directory_iterator(currentDirObj)) {
                    if (entry.is_directory()) {
                        foldersObj.push_back(entry.path().filename().string());
                    } else if (entry.path().extension() == ".obj") {
                        objFiles.push_back(entry.path().filename().string());
                    }
                }

                // フォルダ選択 (クリックで移動)
                if (!foldersObj.empty()) {
                    ImGui::Text("フォルダ");
                    ImGui::Separator();
                    for (const auto &folder : foldersObj) {
                        std::string folderNameTex = folder + " (Model)"; // フォルダ名に "(Tex)" を追加
                        if (ImGui::Selectable(folderNameTex.c_str(), selectedFolderObj == folder)) {
                            selectedFolderObj = folderNameTex;
                            currentDirObj = currentDirObj / folder; // フォルダ移動
                            selectedFileObj = "";                   // 新しいフォルダを開いたらファイル選択をリセット
                        }
                        ImGui::Separator();
                    }
                }

                // `.obj` ファイル選択
                if (!objFiles.empty()) {
                    ImGui::Text("モデルファイル:");
                    if (ImGui::BeginCombo("ファイル選択", selectedFileObj.empty() ? "なし" : selectedFileObj.c_str())) {
                        for (const auto &file : objFiles) {
                            bool isSelected = (file == selectedFileObj);
                            if (ImGui::Selectable(file.c_str(), isSelected)) {
                                selectedFileObj = file;

                                // `baseDirObj` からの相対パスを取得
                                std::filesystem::path relativePath = (currentDirObj / file).lexically_relative(baseDirObj);

                                // Windowsのバックスラッシュをスラッシュに変換
                                std::string pathStr = relativePath.string();
                                std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                                // `fileNameObj_` に保存
                                localFileObj_ = pathStr;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
            }

            // テクスチャ選択アイテム
            if (ImGui::CollapsingHeader("テクスチャ")) {
                // テクスチャファイル選択
                static std::filesystem::path baseDirTex = "resources/images/";
                static std::filesystem::path currentDirTex = "resources/images";
                static std::string selectedFolderTex = "";
                static std::string selectedFileTex = "";

                // 「戻る」ボタン（テクスチャ用）
                if (currentDirTex != "resources/images") {
                    if (ImGui::Button("< 戻る(Tex)")) {
                        currentDirTex = currentDirTex.parent_path();
                        selectedFolderTex = "";
                        selectedFileTex = "";
                    }
                }

                // フォルダ一覧
                std::vector<std::string> foldersTex;
                std::vector<std::string> texFiles;

                for (const auto &entry : std::filesystem::directory_iterator(currentDirTex)) {
                    if (entry.is_directory()) {
                        foldersTex.push_back(entry.path().filename().string());
                    } else if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
                        texFiles.push_back(entry.path().filename().string());
                    }
                }

                // フォルダ選択 (クリックで移動)
                if (!foldersTex.empty()) {
                    ImGui::Text("フォルダ");
                    ImGui::Separator();
                    for (auto &folder : foldersTex) {
                        std::string folderNameTex = folder + " (Tex)"; // フォルダ名に "(Tex)" を追加
                        if (ImGui::Selectable(folderNameTex.c_str(), selectedFolderTex == folder)) {
                            selectedFolderTex = folderNameTex;
                            currentDirTex = currentDirTex / folder; // フォルダ移動
                            selectedFileTex = "";                   // 新しいフォルダを開いたらファイル選択をリセット
                        }
                        ImGui::Separator();
                    }
                }

                // `.png`, `.jpg` テクスチャファイル選択
                if (!texFiles.empty()) {
                    ImGui::Text("テクスチャファイル:");
                    if (ImGui::BeginCombo("ファイル選択 ", selectedFileTex.empty() ? "なし" : selectedFileTex.c_str())) {
                        for (const auto &file : texFiles) {
                            bool isSelected = (file == selectedFileTex);
                            if (ImGui::Selectable(file.c_str(), isSelected)) {
                                selectedFileTex = file;

                                // `baseDirTex` からの相対パスを取得
                                std::filesystem::path relativePath = (currentDirTex / file).lexically_relative(baseDirTex);

                                // Windowsのバックスラッシュをスラッシュに変換
                                std::string pathStr = relativePath.string();
                                std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

                                // `texturePath_` に保存
                                localTexturePath_ = pathStr;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
            }

            // パーティクル作成ボタン
            ImGui::Spacing();
            if (!localName_.empty() && !localFileObj_.empty()) {
                if (ImGui::Button("パーティクル生成")) {
                    AddParticleEmitter(localName_, localFileObj_, localTexturePath_);
                    localName_.clear();
                    localFileObj_.clear();
                    localTexturePath_.clear(); // テクスチャのパスもクリア
                }
            }

            if (ImGui::CollapsingHeader("パーティクルデータのロード")) {
                ShowFileSelector();
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void ParticleEditor::ShowFileSelector() {
    static int selectedIndex = -1;
    std::vector<std::string> jsonFiles = GetJsonFiles();

    // JSONファイルがない場合のチェック
    if (jsonFiles.empty()) {
        ImGui::Text("Jsonファイルが見つかりませんでした");
        return;
    }

    // ファイルリストをCスタイル文字列の配列に変換
    std::vector<const char *> fileNames;
    for (const auto &filePath : jsonFiles) {
        fileNames.push_back(filePath.c_str());
    }

    ImGui::Text("Jsonファイルの選択:");
    ImGui::Separator();

    // Comboボックスでファイル選択
    if (ImGui::Combo("JSON Files", &selectedIndex, fileNames.data(), static_cast<int>(fileNames.size()))) {
        // ファイル選択時の動作（選択されたファイル名を表示）
        if (selectedIndex >= 0) {
            ImGui::Text("ファイル選択:");
            ImGui::TextWrapped("%s", jsonFiles[selectedIndex].c_str());
        }
    }

    // ボタンでパーティクルデータをセット
    if (selectedIndex >= 0 && ImGui::Button("パーティクルデータのセット")) {
        isLoad_ = true;
        // name_ に ".json" を除いた名前を設定
        std::string selectedFileName = jsonFiles[selectedIndex];
        name_ = selectedFileName.substr(0, selectedFileName.find_last_of('.')); // ".json" を除去
        AddParticleEmitter(name_, fileName_, texturePath_);
        isLoad_ = false;
    }
}

std::vector<std::string> ParticleEditor::GetJsonFiles() {

    static std::vector<std::string> jsonFiles; // キャッシュされたJSONファイルリスト
    static size_t lastFileCount = 0;           // 最後に取得したJSONファイル数
    std::filesystem::path baseDir = "resources/jsons/Particle";

    // ディレクトリが存在しない場合はキャッシュをクリア
    if (!std::filesystem::exists(baseDir) || !std::filesystem::is_directory(baseDir)) {
        jsonFiles.clear();
        lastFileCount = 0;
        return jsonFiles;
    }

    // 現在のファイル数をカウント
    size_t currentFileCount = std::count_if(
        std::filesystem::directory_iterator(baseDir),
        std::filesystem::directory_iterator{},
        [](const std::filesystem::directory_entry &entry) {
            return entry.path().extension() == ".json";
        });

    // ファイル数が変わった場合のみ更新
    if (currentFileCount != lastFileCount) {
        jsonFiles.clear(); // リストをクリア
        for (const auto &entry : std::filesystem::directory_iterator(baseDir)) {
            if (entry.path().extension() == ".json") {
                jsonFiles.push_back(entry.path().filename().string());
            }
        }
        lastFileCount = currentFileCount; // 更新したファイル数を記録
    }

    return jsonFiles;
}

std::unique_ptr<ParticleEmitter> ParticleEditor::GetEmitter(const std::string &name) {
    auto it = emitters_.find(name);
    if (it != emitters_.end()) {
        // マップから取り出し、所有権を呼び出し元に移動
        return std::move(it->second);
    }
    return nullptr;
}