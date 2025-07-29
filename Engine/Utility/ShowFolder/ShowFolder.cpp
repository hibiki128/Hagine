#ifdef _DEBUG
#include "ShowFolder.h"
#include <algorithm>
#include <externals/icon/IconsFontAwesome5.h>
#include <filesystem>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>

// テクスチャファイルのプレビューキャッシュ用
struct TextureCache {
    // ここにテクスチャハンドル等を保存する実装を追加
};

void ShowTextureFile(std::string &selectedTexturePath) {
    // スタイルの設定
    ImGuiStyle &style = ImGui::GetStyle();
    const float origItemSpacing = style.ItemSpacing.y;
    const float origFramePadding = style.FramePadding.y;

    // テクスチャファイル選択
    static std::filesystem::path baseDirTex = "resources/images/";
    static std::filesystem::path currentDirTex = "resources/images";
    static std::string selectedFolderTex = "";
    static std::string selectedFileTex = "";
    static std::unordered_map<std::string, TextureCache> textureCache;
    static ImGuiTextFilter filter;
    static bool showDetails = true;

    // パンくずリスト表示
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.8f));

        std::filesystem::path tempPath = baseDirTex;
        std::string breadcrumb = "home";
        if (ImGui::Button(breadcrumb.c_str())) {
            currentDirTex = baseDirTex;
            selectedFolderTex = "";
            selectedFileTex = "";
        }

        // 現在のディレクトリへのパスを表示
        std::filesystem::path relativePath = currentDirTex.lexically_relative(baseDirTex);
        if (!relativePath.empty() && relativePath != ".") {
            for (auto &part : relativePath) {
                ImGui::SameLine();
                ImGui::Text(" > ");
                ImGui::SameLine();

                tempPath /= part;
                if (ImGui::Button(part.string().c_str())) {
                    currentDirTex = tempPath;
                    selectedFolderTex = "";
                    selectedFileTex = "";
                }
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::Separator();
    }

    // 検索バー
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        filter.Draw("##検索", ImGui::GetContentRegionAvail().x - 60);
        ImGui::SameLine();

        // 詳細表示切り替えボタン
        if (ImGui::Button(showDetails ? "シンプル表示" : "詳細表示")) {
            showDetails = !showDetails;
        }
    }

    ImGui::Spacing();

    // ディレクトリの読み取り
    std::vector<std::string> foldersTex;
    std::vector<std::string> texFiles;

    try {
        for (const auto &entry : std::filesystem::directory_iterator(currentDirTex)) {
            if (entry.is_directory()) {
                foldersTex.push_back(entry.path().filename().string());
            } else if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
                texFiles.push_back(entry.path().filename().string());
            }
        }

        // アルファベット順にソート
        std::sort(foldersTex.begin(), foldersTex.end());
        std::sort(texFiles.begin(), texFiles.end());
    } catch (std::exception &e) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "エラー: %s", e.what());
    }

    // フォルダとファイルのコンテナ
    ImGui::BeginChild("FileBrowser", ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // フォルダ表示セクション
    if (!foldersTex.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.8f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.9f, 0.7f));

        if (ImGui::CollapsingHeader("フォルダ", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 8.0f));

            for (auto &folder : foldersTex) {
                if (filter.PassFilter(folder.c_str())) {
                    // フォルダアイコンを表示
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));
                    ImGui::Text(ICON_FA_FOLDER); // FontAwesomeアイコンを使用（要設定）
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    if (ImGui::Selectable(folder.c_str(), selectedFolderTex == folder,
                                          ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            selectedFolderTex = folder;
                            currentDirTex = currentDirTex / folder; // フォルダ移動
                            selectedFileTex = "";                   // 新しいフォルダを開いたらファイル選択をリセット
                        }
                    }

                    // ドラッグ＆ドロップまたはコンテキストメニューの処理
                    if (ImGui::BeginPopupContextItem(folder.c_str())) {
                        if (ImGui::MenuItem("開く")) {
                            selectedFolderTex = folder;
                            currentDirTex = currentDirTex / folder;
                            selectedFileTex = "";
                        }
                        ImGui::EndPopup();
                    }
                }
            }

            ImGui::PopStyleVar();
            ImGui::Unindent(10.0f);
        }

        ImGui::PopStyleColor(3);
    }

    // テクスチャファイル表示セクション
    if (!texFiles.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.7f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.8f, 0.4f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.9f, 0.5f, 0.7f));

        if (ImGui::CollapsingHeader("テクスチャファイル", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);

            if (showDetails) {
                // 詳細表示モード（リスト形式）
                ImGui::Columns(2, "ファイルリスト", true);
                ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
                ImGui::Text("ファイル名");
                ImGui::NextColumn();
                ImGui::Text("拡張子");
                ImGui::NextColumn();
                ImGui::Separator();

                for (const auto &file : texFiles) {
                    if (filter.PassFilter(file.c_str())) {
                        std::filesystem::path filePath(file);
                        std::string extension = filePath.extension().string();

                        // ファイルアイコンを表示
                        ImGui::PushStyleColor(ImGuiCol_Text,
                                              extension == ".png" ? ImVec4(0.4f, 0.8f, 1.0f, 1.0f) : ImVec4(1.0f, 0.6f, 0.4f, 1.0f));
                        ImGui::Text(ICON_FA_FILE_IMAGE); // FontAwesomeアイコンを使用（要設定）
                        ImGui::PopStyleColor();

                        ImGui::SameLine();
                        bool isSelected = (file == selectedFileTex);
                        if (ImGui::Selectable(file.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                            selectedFileTex = file;
                            // `baseDirTex` からの相対パスを取得
                            std::filesystem::path relativePath = (currentDirTex / file).lexically_relative(baseDirTex);
                            // Windowsのバックスラッシュをスラッシュに変換
                            std::string pathStr = relativePath.string();
                            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
                            // 選択されたテクスチャパスを設定
                            selectedTexturePath = pathStr;
                        }

                        ImGui::NextColumn();
                        ImGui::Text("%s", extension.c_str());
                        ImGui::NextColumn();
                    }
                }

                ImGui::Columns(1);
            } else {
                // シンプル表示モード（グリッド形式）
                float cellSize = 100.0f;
                float panelWidth = ImGui::GetContentRegionAvail().x;
                int numColumns = static_cast<int>(panelWidth / cellSize);
                if (numColumns < 1)
                    numColumns = 1;

                ImGui::Columns(numColumns, "ファイルグリッド", false);

                for (const auto &file : texFiles) {
                    if (filter.PassFilter(file.c_str())) {
                        bool isSelected = (file == selectedFileTex);
                        ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.5f, 0.5f, 0.7f, 0.7f) : ImVec4(0.3f, 0.3f, 0.3f, 0.0f));

                        ImGui::PushID(file.c_str());
                        if (ImGui::Button("", ImVec2(cellSize - 10, cellSize - 10))) {
                            selectedFileTex = file;
                            // `baseDirTex` からの相対パスを取得
                            std::filesystem::path relativePath = (currentDirTex / file).lexically_relative(baseDirTex);
                            // Windowsのバックスラッシュをスラッシュに変換
                            std::string pathStr = relativePath.string();
                            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
                            // 選択されたテクスチャパスを設定
                            selectedTexturePath = pathStr;
                        }
                        ImGui::PopID();

                        ImGui::PopStyleColor();

                        // ファイル名を表示（短縮する必要がある場合）
                        if (file.length() > 8) {
                            std::string shortName = file.substr(0, 9) + "...";
                            ImGui::TextWrapped("%s", shortName.c_str());
                        } else {
                            ImGui::TextWrapped("%s", file.c_str());
                        }

                        ImGui::NextColumn();
                    }
                }

                ImGui::Columns(1);
            }

            ImGui::Unindent(10.0f);
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::EndChild();

    // 下部ステータスバー
    ImGui::Separator();
    ImGui::Text("現在のパス: %s", currentDirTex.string().c_str());
    ImGui::Text("ファイル数: %zu", texFiles.size());

    // スタイルを元に戻す
    style.ItemSpacing.y = origItemSpacing;
    style.FramePadding.y = origFramePadding;
}

void ShowModelFile(std::string &selectedModelPath) {
    // スタイルの設定
    ImGuiStyle &style = ImGui::GetStyle();
    const float origItemSpacing = style.ItemSpacing.y;
    const float origFramePadding = style.FramePadding.y;

    // モデルファイル選択
    static std::filesystem::path baseDirModel = "resources/models/";
    static std::filesystem::path currentDirModel = "resources/models";
    static std::string selectedFolderModel = "";
    static std::string selectedFileModel = "";
    static ImGuiTextFilter filter;
    static bool showDetails = true;

    // パンくずリスト表示
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.8f));

        std::filesystem::path tempPath = baseDirModel;
        std::string breadcrumb = "home";
        if (ImGui::Button(breadcrumb.c_str())) {
            currentDirModel = baseDirModel;
            selectedFolderModel = "";
            selectedFileModel = "";
        }

        // 現在のディレクトリへのパスを表示
        std::filesystem::path relativePath = currentDirModel.lexically_relative(baseDirModel);
        if (!relativePath.empty() && relativePath != ".") {
            for (auto &part : relativePath) {
                ImGui::SameLine();
                ImGui::Text(" > ");
                ImGui::SameLine();

                tempPath /= part;
                if (ImGui::Button(part.string().c_str())) {
                    currentDirModel = tempPath;
                    selectedFolderModel = "";
                    selectedFileModel = "";
                }
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::Separator();
    }

    // 検索バー
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        filter.Draw("##検索", ImGui::GetContentRegionAvail().x - 60);
        ImGui::SameLine();

        // 詳細表示切り替えボタン
        if (ImGui::Button(showDetails ? "シンプル表示" : "詳細表示")) {
            showDetails = !showDetails;
        }
    }

    ImGui::Spacing();

    // ディレクトリの読み取り
    std::vector<std::string> foldersModel;
    std::vector<std::string> modelFiles;

    try {
        for (const auto &entry : std::filesystem::directory_iterator(currentDirModel)) {
            if (entry.is_directory()) {
                foldersModel.push_back(entry.path().filename().string());
            } else {
                std::string ext = entry.path().extension().string();
                // 一般的な3Dモデルファイル形式をサポート
                if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" ||
                    ext == ".dae" || ext == ".3ds" || ext == ".ply" || ext == ".x3d") {
                    modelFiles.push_back(entry.path().filename().string());
                }
            }
        }

        // アルファベット順にソート
        std::sort(foldersModel.begin(), foldersModel.end());
        std::sort(modelFiles.begin(), modelFiles.end());
    } catch (std::exception &e) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "エラー: %s", e.what());
    }

    // フォルダとファイルのコンテナ
    ImGui::BeginChild("FileBrowser", ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // フォルダ表示セクション
    if (!foldersModel.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.8f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.9f, 0.7f));

        if (ImGui::CollapsingHeader("フォルダ", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 8.0f));

            for (auto &folder : foldersModel) {
                if (filter.PassFilter(folder.c_str())) {
                    // フォルダアイコンを表示
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));
                    ImGui::Text(ICON_FA_FOLDER);
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    if (ImGui::Selectable(folder.c_str(), selectedFolderModel == folder,
                                          ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            selectedFolderModel = folder;
                            currentDirModel = currentDirModel / folder;
                            selectedFileModel = "";
                        }
                    }

                    // コンテキストメニューの処理
                    if (ImGui::BeginPopupContextItem(folder.c_str())) {
                        if (ImGui::MenuItem("開く")) {
                            selectedFolderModel = folder;
                            currentDirModel = currentDirModel / folder;
                            selectedFileModel = "";
                        }
                        ImGui::EndPopup();
                    }
                }
            }

            ImGui::PopStyleVar();
            ImGui::Unindent(10.0f);
        }

        ImGui::PopStyleColor(3);
    }

    // モデルファイル表示セクション
    if (!modelFiles.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.7f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.8f, 0.4f, 0.4f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.9f, 0.5f, 0.5f, 0.7f));

        if (ImGui::CollapsingHeader("モデルファイル", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);

            if (showDetails) {
                // 詳細表示モード（リスト形式）
                ImGui::Columns(2, "ファイルリスト", true);
                ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
                ImGui::Text("ファイル名");
                ImGui::NextColumn();
                ImGui::Text("拡張子");
                ImGui::NextColumn();
                ImGui::Separator();

                for (const auto &file : modelFiles) {
                    if (filter.PassFilter(file.c_str())) {
                        std::filesystem::path filePath(file);
                        std::string extension = filePath.extension().string();

                        // ファイルアイコンを表示（拡張子に応じて色を変更）
                        ImVec4 iconColor;
                        if (extension == ".obj" || extension == ".fbx") {
                            iconColor = ImVec4(1.0f, 0.8f, 0.4f, 1.0f); // オレンジ
                        } else if (extension == ".gltf" || extension == ".glb") {
                            iconColor = ImVec4(0.4f, 1.0f, 0.8f, 1.0f); // 水色
                        } else {
                            iconColor = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // 赤
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
                        ImGui::Text(ICON_FA_CUBE);
                        ImGui::PopStyleColor();

                        ImGui::SameLine();
                        bool isSelected = (file == selectedFileModel);
                        if (ImGui::Selectable(file.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                            selectedFileModel = file;
                            // `baseDirModel` からの相対パスを取得
                            std::filesystem::path relativePath = (currentDirModel / file).lexically_relative(baseDirModel);
                            // Windowsのバックスラッシュをスラッシュに変換
                            std::string pathStr = relativePath.string();
                            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
                            // 選択されたモデルパスを設定
                            selectedModelPath = pathStr;
                        }

                        ImGui::NextColumn();
                        ImGui::Text("%s", extension.c_str());
                        ImGui::NextColumn();
                    }
                }

                ImGui::Columns(1);
            } else {
                // シンプル表示モード（グリッド形式）
                float cellSize = 100.0f;
                float panelWidth = ImGui::GetContentRegionAvail().x;
                int numColumns = static_cast<int>(panelWidth / cellSize);
                if (numColumns < 1)
                    numColumns = 1;

                ImGui::Columns(numColumns, "ファイルグリッド", false);

                for (const auto &file : modelFiles) {
                    if (filter.PassFilter(file.c_str())) {
                        bool isSelected = (file == selectedFileModel);
                        ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.7f, 0.5f, 0.5f, 0.7f) : ImVec4(0.3f, 0.3f, 0.3f, 0.0f));

                        ImGui::PushID(file.c_str());
                        if (ImGui::Button("", ImVec2(cellSize - 10, cellSize - 10))) {
                            selectedFileModel = file;
                            // `baseDirModel` からの相対パスを取得
                            std::filesystem::path relativePath = (currentDirModel / file).lexically_relative(baseDirModel);
                            // Windowsのバックスラッシュをスラッシュに変換
                            std::string pathStr = relativePath.string();
                            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
                            // 選択されたモデルパスを設定
                            selectedModelPath = pathStr;
                        }
                        ImGui::PopID();

                        ImGui::PopStyleColor();

                        // ファイル名を表示（短縮する必要がある場合）
                        if (file.length() > 12) {
                            std::string shortName = file.substr(0, 9) + "...";
                            ImGui::TextWrapped("%s", shortName.c_str());
                        } else {
                            ImGui::TextWrapped("%s", file.c_str());
                        }

                        ImGui::NextColumn();
                    }
                }

                ImGui::Columns(1);
            }

            ImGui::Unindent(10.0f);
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::EndChild();

    // 下部ステータスバー
    ImGui::Separator();
    ImGui::Text("現在のパス: %s", currentDirModel.string().c_str());
    ImGui::Text("ファイル数: %zu", modelFiles.size());

    // 選択したファイルの情報表示（プレビューなし）
    if (!selectedFileModel.empty()) {
        ImGui::Separator();
        ImGui::Text("選択されたモデル: %s", selectedFileModel.c_str());
        ImGui::Text("パス: %s", selectedModelPath.c_str());
    }

    // スタイルを元に戻す
    style.ItemSpacing.y = origItemSpacing;
    style.FramePadding.y = origFramePadding;
}

void ShowJsonFile(std::string &selectedJsonPath, std::string &startPath) {
    // スタイルの設定
    ImGuiStyle &style = ImGui::GetStyle();
    const float origItemSpacing = style.ItemSpacing.y;
    const float origFramePadding = style.FramePadding.y;

    // JSONファイル選択
    static std::filesystem::path baseDirJson = "resources/jsons/" + startPath;
    static std::filesystem::path currentDirJson = "resources/jsons/" + startPath;
    static std::string selectedFolderJson = "";
    static std::string selectedFileJson = "";
    static ImGuiTextFilter filter;
    static bool showDetails = true;

    // パンくずリスト表示
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.8f));

        std::filesystem::path tempPath = baseDirJson;
        std::string breadcrumb = "home";
        if (ImGui::Button(breadcrumb.c_str())) {
            currentDirJson = baseDirJson;
            selectedFolderJson = "";
            selectedFileJson = "";
        }

        // 現在のディレクトリへのパスを表示
        std::filesystem::path relativePath = currentDirJson.lexically_relative(baseDirJson);
        if (!relativePath.empty() && relativePath != ".") {
            for (auto &part : relativePath) {
                ImGui::SameLine();
                ImGui::Text(" > ");
                ImGui::SameLine();

                tempPath /= part;
                if (ImGui::Button(part.string().c_str())) {
                    currentDirJson = tempPath;
                    selectedFolderJson = "";
                    selectedFileJson = "";
                }
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::Separator();
    }

    // 検索バー
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        filter.Draw("##検索", ImGui::GetContentRegionAvail().x - 60);
        ImGui::SameLine();

        // 詳細表示切り替えボタン
        if (ImGui::Button(showDetails ? "シンプル表示" : "詳細表示")) {
            showDetails = !showDetails;
        }
    }

    ImGui::Spacing();

    // ディレクトリの読み取り
    std::vector<std::string> foldersJson;
    std::vector<std::string> jsonFiles;

    try {
        for (const auto &entry : std::filesystem::directory_iterator(currentDirJson)) {
            if (entry.is_directory()) {
                foldersJson.push_back(entry.path().filename().string());
            } else {
                std::string ext = entry.path().extension().string();
                // JSONファイル形式をサポート
                if (ext == ".json" || ext == ".jsonl" || ext == ".geojson") {
                    jsonFiles.push_back(entry.path().filename().string());
                }
            }
        }

        // アルファベット順にソート
        std::sort(foldersJson.begin(), foldersJson.end());
        std::sort(jsonFiles.begin(), jsonFiles.end());
    } catch (std::exception &e) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "エラー: %s", e.what());
    }

    // フォルダとファイルのコンテナ
    ImGui::BeginChild("FileBrowser", ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // フォルダ表示セクション
    if (!foldersJson.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.8f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.9f, 0.7f));

        if (ImGui::CollapsingHeader("フォルダ", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 8.0f));

            for (auto &folder : foldersJson) {
                if (filter.PassFilter(folder.c_str())) {
                    // フォルダアイコンを表示
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));
                    ImGui::Text(ICON_FA_FOLDER);
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    if (ImGui::Selectable(folder.c_str(), selectedFolderJson == folder,
                                          ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            selectedFolderJson = folder;
                            currentDirJson = currentDirJson / folder;
                            selectedFileJson = "";
                        }
                    }

                    // コンテキストメニューの処理
                    if (ImGui::BeginPopupContextItem(folder.c_str())) {
                        if (ImGui::MenuItem("開く")) {
                            selectedFolderJson = folder;
                            currentDirJson = currentDirJson / folder;
                            selectedFileJson = "";
                        }
                        ImGui::EndPopup();
                    }
                }
            }

            ImGui::PopStyleVar();
            ImGui::Unindent(10.0f);
        }

        ImGui::PopStyleColor(3);
    }

    // JSONファイル表示セクション
    if (!jsonFiles.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.7f, 0.7f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.8f, 0.8f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.9f, 0.9f, 0.7f));

        if (ImGui::CollapsingHeader("JSONファイル", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);

            if (showDetails) {
                // 詳細表示モード（リスト形式）
                ImGui::Columns(2, "ファイルリスト", true);
                ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
                ImGui::Text("ファイル名");
                ImGui::NextColumn();
                ImGui::Text("拡張子");
                ImGui::NextColumn();
                ImGui::Separator();

                for (const auto &file : jsonFiles) {
                    if (filter.PassFilter(file.c_str())) {
                        std::filesystem::path filePath(file);
                        std::string extension = filePath.extension().string();

                        // ファイルアイコンを表示（拡張子に応じて色を変更）
                        ImVec4 iconColor;
                        if (extension == ".json") {
                            iconColor = ImVec4(1.0f, 0.9f, 0.4f, 1.0f); // 黄色
                        } else if (extension == ".jsonl") {
                            iconColor = ImVec4(0.4f, 0.8f, 1.0f, 1.0f); // 水色
                        } else if (extension == ".geojson") {
                            iconColor = ImVec4(0.4f, 1.0f, 0.6f, 1.0f); // 緑
                        } else {
                            iconColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // 灰色
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
                        ImGui::Text(ICON_FA_FILE_CODE);
                        ImGui::PopStyleColor();

                        ImGui::SameLine();
                        bool isSelected = (file == selectedFileJson);
                        if (ImGui::Selectable(file.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                            selectedFileJson = file;
                            // `baseDirJson` からの相対パスを取得
                            std::filesystem::path relativePath = (currentDirJson / file).lexically_relative(baseDirJson);
                            // Windowsのバックスラッシュをスラッシュに変換
                            std::string pathStr = relativePath.string();
                            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
                            // 選択されたJSONパスを設定
                            selectedJsonPath = pathStr;
                        }

                        ImGui::NextColumn();
                        ImGui::Text("%s", extension.c_str());
                        ImGui::NextColumn();
                    }
                }

                ImGui::Columns(1);
            } else {
                // シンプル表示モード（グリッド形式）
                float cellSize = 100.0f;
                float panelWidth = ImGui::GetContentRegionAvail().x;
                int numColumns = static_cast<int>(panelWidth / cellSize);
                if (numColumns < 1)
                    numColumns = 1;

                ImGui::Columns(numColumns, "ファイルグリッド", false);

                for (const auto &file : jsonFiles) {
                    if (filter.PassFilter(file.c_str())) {
                        bool isSelected = (file == selectedFileJson);
                        ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.5f, 0.7f, 0.7f, 0.7f) : ImVec4(0.3f, 0.3f, 0.3f, 0.0f));

                        ImGui::PushID(file.c_str());
                        if (ImGui::Button("", ImVec2(cellSize - 10, cellSize - 10))) {
                            selectedFileJson = file;
                            // `baseDirJson` からの相対パスを取得
                            std::filesystem::path relativePath = (currentDirJson / file).lexically_relative(baseDirJson);
                            // Windowsのバックスラッシュをスラッシュに変換
                            std::string pathStr = relativePath.string();
                            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
                            // 選択されたJSONパスを設定
                            selectedJsonPath = pathStr;
                        }
                        ImGui::PopID();

                        ImGui::PopStyleColor();

                        // ファイル名を表示（短縮する必要がある場合）
                        if (file.length() > 12) {
                            std::string shortName = file.substr(0, 9) + "...";
                            ImGui::TextWrapped("%s", shortName.c_str());
                        } else {
                            ImGui::TextWrapped("%s", file.c_str());
                        }

                        ImGui::NextColumn();
                    }
                }

                ImGui::Columns(1);
            }

            ImGui::Unindent(10.0f);
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::EndChild();

    // 下部ステータスバー
    ImGui::Separator();
    ImGui::Text("現在のパス: %s", currentDirJson.string().c_str());
    ImGui::Text("ファイル数: %zu", jsonFiles.size());

    // 選択したファイルの情報表示（プレビューなし）
    if (!selectedFileJson.empty()) {
        ImGui::Separator();
        ImGui::Text("選択されたJSON: %s", selectedFileJson.c_str());
        ImGui::Text("パス: %s", selectedJsonPath.c_str());
    }

    // スタイルを元に戻す
    style.ItemSpacing.y = origItemSpacing;
    style.FramePadding.y = origFramePadding;
}

#endif // _DEBUG