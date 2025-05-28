#include "ImGuiManager.h"
#ifdef _DEBUG
#include "Engine/Offscreen/OffScreen.h"
#include "ImGuizmo.h"
#include "ImGuizmoManager.h"
#include "SceneManager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include <Engine/Frame/Frame.h>
#include <externals/icon/IconsFontAwesome5.h>
#include <imgui_impl_dx12.h>

ImGuiManager *ImGuiManager::instance = nullptr;

void ImGuiManager::Initialize(WinApp *winApp) {

    dxCommon_ = DirectXCommon::GetInstance();
    baseObjectManager_ = BaseObjectManager::GetInstance();

    // ImGuiのコンテキストを生成
    ImGui::CreateContext();

    // Docking機能を有効化
    ImGuiIO &io = ImGui::GetIO();
    // 高度な機能を有効化
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // ドッキング機能
    io.ConfigWindowsResizeFromEdges = true;           // エッジからリサイズ
    io.ConfigWindowsMoveFromTitleBarOnly = true;      // タイトルバーからの移動

    // パフォーマンス関連の設定
    io.ConfigMemoryCompactTimer = 300.0f; // メモリ圧縮の間隔を長く
    io.IniFilename = nullptr;             // 設定ファイルの保存場所
                                          // 設定ファイルの保存場所
    LoadLayoutForCurrentMode();

    io.Fonts->Clear(); // 既存のフォントをクリア

    float fontSize = 16.0f;

    io.Fonts->AddFontFromFileTTF("resources/fonts/PixelMplus12-Regular.ttf", 14.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

    // アイコンフォント読み込み（FontAwesomeなど）
    // FontAwesomeの設定
    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = fontSize;
    io.Fonts->AddFontFromFileTTF("resources/fonts/fa-solid-900.ttf", fontSize, &icons_config, icon_ranges);

    // フォントの生成
    unsigned char *tex_pixels = nullptr;
    int tex_width, tex_height;
    io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);

    // カスタムテーマを設定
    SetupTheme();

    ImGui_ImplWin32_Init(winApp->GetHwnd());

    // srvインデックスを割り
    srvManager_ = SrvManager::GetInstance();

    uint32_t srvIndex = srvManager_->Allocate();
    ImGui_ImplDX12_Init(
        dxCommon_->GetDevice().Get(),
        2,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        srvManager_->GetDescriptorHeap(),
        srvManager_->GetCPUDescriptorHandle(srvIndex),
        srvManager_->GetGPUDescriptorHandle(srvIndex));
    imGuizmoManager_ = ImGuizmoManager::GetInstance();
}

void ImGuiManager::SetupTheme() {
    // モダンなダークテーマ
    ImGuiStyle &style = ImGui::GetStyle();

    // カラースキーム
    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.27f, 0.36f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.37f, 0.37f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.63f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.67f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.57f, 0.67f, 0.94f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.53f, 0.54f, 0.72f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.71f, 0.72f, 0.96f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.49f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.47f, 0.47f, 0.61f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.31f, 0.31f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.37f, 0.37f, 0.45f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.61f, 0.59f, 0.87f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.73f, 0.71f, 0.94f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.43f, 0.43f, 0.54f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.53f, 0.88f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.71f, 0.69f, 0.94f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.53f, 0.53f, 0.66f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.41f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.57f, 0.57f, 0.91f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.60f, 0.76f, 0.47f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.86f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);

    // スタイル設定
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 6);
    style.CellPadding = ImVec2(6, 3);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 6);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 20;
    style.ScrollbarSize = 14;
    style.GrabMinSize = 12;

    // 外観
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 0;
    style.TabBorderSize = 0;

    // 丸み
    style.WindowRounding = 6;
    style.ChildRounding = 6;
    style.FrameRounding = 4;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 6;
    style.GrabRounding = 4;
    style.TabRounding = 4;

    // Viewportsの設定（マルチウィンドウモード）
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void ImGuiManager::CreateDescriptorHeap() {
    HRESULT result;

    // デスクリプタヒープ設定
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    // デスクリプタヒープ生成
    result = dxCommon_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap_));

    ImGui_ImplDX12_Init(
        dxCommon_->GetDevice().Get(),
        static_cast<int>(dxCommon_->GetBackBufferCount()),
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, srvHeap_.Get(),
        srvHeap_->GetCPUDescriptorHandleForHeapStart(),
        srvHeap_->GetGPUDescriptorHandleForHeapStart());
}

ImGuiManager *ImGuiManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ImGuiManager;
    }
    return instance;
}

void ImGuiManager::Finalize() {
    SaveCurrentLayout();
    // 後始末
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // デスクリプタヒープを解放
    srvHeap_.Reset();

    delete instance;
    instance = nullptr;
}

void ImGuiManager::Begin() {
    // ImGuiフレーム開始
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::End() {
    // 描画前準備
    ImGui::Render();
}

void ImGuiManager::Draw() {
    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();

    //// デスクリプタヒープの配列をセットするコマンド
    // ID3D12DescriptorHeap *ppHeaps[] = {srvHeap_.Get()};
    // commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    //  描画コマンドを発行
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void ImGuiManager::UpdateIni() {
    if (!isShowMainUI_) {
        SwitchToGameMode();
    } else {
        SwitchToEditorMode();
    }
}

void ImGuiManager::ShowMainMenu() {
    // メインメニューバー作成
    if (ImGui::BeginMainMenuBar()) {
        // ファイルメニュー
        if (ImGui::BeginMenu(ICON_FA_FILE " ファイル")) {
            if (ImGui::MenuItem(ICON_FA_PLUS " 新規作成", "Ctrl+N")) {
                // 新規プロジェクト作成処理
            }
            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " 開く", "Ctrl+O")) {
                // プロジェクトを開く処理
            }
            if (ImGui::MenuItem(ICON_FA_SAVE " 保存", "Ctrl+S")) {
                // プロジェクト保存処理
            }
            if (ImGui::MenuItem(ICON_FA_SAVE " 名前を付けて保存", "Ctrl+Shift+S")) {
                // 名前を付けて保存処理
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " エクスポート")) {
                // エクスポート処理
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_DOOR_OPEN " 終了", "Alt+F4")) {
                // アプリケーション終了処理
                WinApp::GetInstance()->ProcessMessage(); // 終了メッセージ送信
            }
            ImGui::EndMenu();
        }

        // 編集メニュー
        if (ImGui::BeginMenu(ICON_FA_EDIT " 編集")) {
            if (ImGui::MenuItem(ICON_FA_UNDO " 元に戻す", "Ctrl+Z", false, false)) {
            }
            if (ImGui::MenuItem(ICON_FA_REDO " やり直し", "Ctrl+Y", false, false)) {
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_CUT " 切り取り", "Ctrl+X")) {
            }
            if (ImGui::MenuItem(ICON_FA_COPY " コピー", "Ctrl+C")) {
            }
            if (ImGui::MenuItem(ICON_FA_PASTE " 貼り付け", "Ctrl+V")) {
            }
            if (ImGui::MenuItem(ICON_FA_TRASH_ALT " 削除", "Delete")) {
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_COG " 環境設定", "Ctrl+,")) {
                // showSettingsWindow_ = true;
            }
            ImGui::EndMenu();
        }

        // 表示メニュー
        if (ImGui::BeginMenu(ICON_FA_EYE " 表示")) {
            // ウィンドウ表示設定
            if (ImGui::BeginMenu(ICON_FA_WINDOW_MAXIMIZE " ウィンドウ")) {
                // ImGui::MenuItem(ICON_FA_GAMEPAD " ゲームビュー", nullptr, &showGameView_);
                ImGui::MenuItem(ICON_FA_BOOK_OPEN " シーンビュー", nullptr, &showSceneView_);
                ImGui::MenuItem(ICON_FA_CUBE " オブジェクトビュー", nullptr, &showObjectView_);
                ImGui::MenuItem(ICON_FA_STAR " パーティクルビュー", nullptr, &showParticleView_);
                ImGui::MenuItem(ICON_FA_DATABASE " FPSビュー", nullptr, &showFPSView_);
                ImGui::MenuItem(ICON_FA_STAR_OF_DAVID " オフスクリーンビュー", nullptr, &showOfScreenView_);
                ImGui::MenuItem(ICON_FA_LIGHTBULB " ライトビュー", nullptr, &showLightView_);
                // ImGui::MenuItem(ICON_FA_FOLDER " プロジェクト", nullptr, &showProject_);
                ImGui::EndMenu();
            }

            // 表示モード切替
            ImGui::Separator();
            if (isShowMainUI_) {
                if (ImGui::MenuItem(ICON_FA_GAMEPAD " ゲームモードに切替", "F5")) {
                    isShowMainUI_ = false;
                    SwitchToGameMode();
                }
            } else {
                if (ImGui::MenuItem(ICON_FA_WRENCH " エディターモードに切替", "F5")) {
                    isShowMainUI_ = true;
                    SwitchToEditorMode();
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_EXPAND " フルスクリーン切替", "F11")) {
                WinApp::GetInstance()->ToggleFullScreen();
            }
            ImGui::EndMenu();
        }

        // オブジェクトメニュー
        if (ImGui::BeginMenu(ICON_FA_CUBE " オブジェクト")) {
            if (ImGui::MenuItem(ICON_FA_PLUS " 新規オブジェクト", "Ctrl+Shift+N")) {
                // 新規オブジェクト作成
            }

            // 3Dオブジェクト
            if (ImGui::BeginMenu(ICON_FA_CUBE " 3Dオブジェクト")) {
                if (ImGui::MenuItem(ICON_FA_CUBE " キューブ")) {
                    std::string name = "cube_" + std::to_string(++cubeCount);
                    std::unique_ptr<BaseObject> object = std::make_unique<BaseObject>();
                    object->Init(name);
                    object->CreatePrimitiveModel(PrimitiveType::Cube);
                    baseObjectManager_->AddObject(std::move(object));
                }

                if (ImGui::MenuItem(ICON_FA_CIRCLE " 球体")) {
                    std::string name = "sphere_" + std::to_string(++sphereCount);
                    std::unique_ptr<BaseObject> object = std::make_unique<BaseObject>();
                    object->Init(name);
                    object->CreatePrimitiveModel(PrimitiveType::Sphere);
                    baseObjectManager_->AddObject(std::move(object));
                }

                if (ImGui::MenuItem(ICON_FA_CUBE " 平面")) {
                    std::string name = "plane_" + std::to_string(++planeCount);
                    std::unique_ptr<BaseObject> object = std::make_unique<BaseObject>();
                    object->Init(name);
                    object->CreatePrimitiveModel(PrimitiveType::Plane);
                    baseObjectManager_->AddObject(std::move(object));
                }

                if (ImGui::MenuItem(ICON_FA_CIRCLE " シリンダー")) {
                    std::string name = "cylinder_" + std::to_string(++cylinderCount);
                    std::unique_ptr<BaseObject> object = std::make_unique<BaseObject>();
                    object->Init(name);
                    object->CreatePrimitiveModel(PrimitiveType::Cylinder);
                    baseObjectManager_->AddObject(std::move(object));
                }

                if (ImGui::MenuItem(ICON_FA_RING " リング")) {
                    std::string name = "ring_" + std::to_string(++ringCount);
                    std::unique_ptr<BaseObject> object = std::make_unique<BaseObject>();
                    object->Init(name);
                    object->CreatePrimitiveModel(PrimitiveType::Ring);
                    baseObjectManager_->AddObject(std::move(object));
                }

                if (ImGui::MenuItem(ICON_FA_CARET_UP " 三角形")) {
                    std::string name = "triangle_" + std::to_string(++triangleCount);
                    std::unique_ptr<BaseObject> object = std::make_unique<BaseObject>();
                    object->Init(name);
                    object->CreatePrimitiveModel(PrimitiveType::Triangle);
                    baseObjectManager_->AddObject(std::move(object));
                }

                if (ImGui::MenuItem(ICON_FA_MOUNTAIN " ピラミッド")) {
                    std::string name = "pyramid_" + std::to_string(++pyramidCount);
                    std::unique_ptr<BaseObject> object = std::make_unique<BaseObject>();
                    object->Init(name);
                    object->CreatePrimitiveModel(PrimitiveType::Pyramid);
                    baseObjectManager_->AddObject(std::move(object));
                }

                if (ImGui::MenuItem(ICON_FA_CHART_AREA " 円柱")) {
                    std::string name = "cone_" + std::to_string(++coneCount);
                    std::unique_ptr<BaseObject> object = std::make_unique<BaseObject>();
                    object->Init(name);
                    object->CreatePrimitiveModel(PrimitiveType::Cone);
                    baseObjectManager_->AddObject(std::move(object));
                }

                if (ImGui::MenuItem(ICON_FA_TRASH_ALT " オブジェクト全削除")) {
                    baseObjectManager_->DeleteObject();
                    imGuizmoManager_->DeleteTarget();
                }

                ImGui::EndMenu();
            }

            // 2Dオブジェクト
            if (ImGui::BeginMenu(ICON_FA_SQUARE " 2Dオブジェクト")) {
                if (ImGui::MenuItem(ICON_FA_SQUARE " スプライト")) {
                }
                if (ImGui::MenuItem(ICON_FA_FONT " テキスト")) {
                }
                if (ImGui::MenuItem(ICON_FA_IMAGE " 画像")) {
                }
                ImGui::EndMenu();
            }

            // エフェクト
            if (ImGui::BeginMenu(ICON_FA_MAGIC " エフェクト")) {
                if (ImGui::MenuItem(ICON_FA_SNOWFLAKE " パーティクルシステム")) {
                }
                if (ImGui::MenuItem(ICON_FA_LIGHTBULB " ライト")) {
                }
                if (ImGui::MenuItem(ICON_FA_VIDEO " カメラ")) {
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        // コンポーネントメニュー
        if (ImGui::BeginMenu(ICON_FA_PUZZLE_PIECE " コンポーネント")) {
            if (ImGui::BeginMenu(ICON_FA_COGS " 物理")) {
                if (ImGui::MenuItem(ICON_FA_WEIGHT " リジッドボディ")) {
                }
                if (ImGui::MenuItem(ICON_FA_CUBE " コライダー")) {
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_VOLUME_UP " オーディオ")) {
                if (ImGui::MenuItem(ICON_FA_MUSIC " オーディオソース")) {
                }
                if (ImGui::MenuItem(ICON_FA_HEADPHONES " オーディオリスナー")) {
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_CODE " スクリプト")) {
                if (ImGui::MenuItem(ICON_FA_FILE_CODE " 新規スクリプト")) {
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        // ツールメニュー
        if (ImGui::BeginMenu(ICON_FA_TOOLS " ツール")) {
            if (ImGui::MenuItem(ICON_FA_PAINT_BRUSH " マテリアルエディタ")) {
            }
            if (ImGui::MenuItem(ICON_FA_FILTER " パーティクルエディタ")) {
            }
            if (ImGui::MenuItem(ICON_FA_WATER " シェーダーエディタ")) {
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_BUG " デバッグ情報表示", nullptr /*, &showDebugInfo_*/)) {
            }
            ImGui::EndMenu();
        }

        // ヘルプメニュー
        if (ImGui::BeginMenu(ICON_FA_QUESTION_CIRCLE " ヘルプ")) {
            if (ImGui::MenuItem(ICON_FA_BOOK " ドキュメント", "F1")) {
            }
            if (ImGui::MenuItem(ICON_FA_INFO_CIRCLE " バージョン情報")) {
                // showAboutWindow_ = true;
            }
            ImGui::EndMenu();
        }

        // シーンメニュー
        if (ImGui::BeginMenu(ICON_FA_GLOBE " シーン選択")) { // 地球アイコン（意味：全体メニュー）

            if (ImGui::MenuItem(ICON_FA_HOME " タイトルシーン")) { // home アイコン
                SceneManager::GetInstance()->SceneSelection("TITLE");
            }
            if (ImGui::MenuItem(ICON_FA_BARS " セレクトシーン")) { // bars アイコン（メニュー選択感）
                SceneManager::GetInstance()->SceneSelection("SELECT");
            }
            if (ImGui::MenuItem(ICON_FA_GAMEPAD " ゲームシーン")) { // gamepad アイコン
                SceneManager::GetInstance()->SceneSelection("GAME");
            }
            if (ImGui::MenuItem(ICON_FA_TROPHY " クリアシーン")) { // trophy アイコン
                SceneManager::GetInstance()->SceneSelection("CLEAR");
            }
            if (ImGui::MenuItem(ICON_FA_FILM " デモシーン")) { // film アイコン
                SceneManager::GetInstance()->SceneSelection("DEMO");
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void ImGuiManager::ShowSceneSettingWindow() {
    if (!showSceneView_)
        return; // 表示しない場合は早期リターン

    // パフォーマンス改善: 軽量化フラグを追加
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("シーン設定", &showSceneView_, flags);

    currentScene_->AddSceneSetting();

    ImGui::End();
}

void ImGuiManager::ShowObjectSettingWindow() {
    if (!showObjectView_)
        return; // 表示しない場合は早期リターン

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("オブジェクト設定", &showObjectView_, flags);

    currentScene_->AddObjectSetting();
    // baseObjectManager_->DrawImGui();

    ImGui::End();
}

void ImGuiManager::ShowParticleSettingWindow() {
    if (!showParticleView_)
        return; // 表示しない場合は早期リターン

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("パーティクル設定", &showParticleView_, flags);

    currentScene_->AddParticleSetting();

    ImGui::End();
}

void ImGuiManager::ShowFPSWindow() {
    if (!showFPSView_)
        return; // 表示しない場合は早期リターン

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("FPS", &showFPSView_, flags);

    DisplayFPS();

    ImGui::End();
}

void ImGuiManager::ShowOffScreenSettingWindow(OffScreen *offscreen) {
    if (!showOfScreenView_)
        return; // 表示しない場合は早期リターン

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("オフスクリーン設定", &showOfScreenView_, flags);

    offscreen->Setting();

    ImGui::End();
}

void ImGuiManager::ShowLightSettingWindow() {
    if (!showLightView_)
        return; // 表示しない場合は早期リターン

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("ライト設定", &showLightView_, flags);

    LightGroup::GetInstance()->imgui();

    ImGui::End();
}

void ImGuiManager::FixAspectRatio() {

    // 横幅ベースで16:9に合わせた高さ
    float adjustedHeight = (sceneTextureSize_.x * 9.0f / 16.0f);
    // 高さベースで16:9に合わせた横幅
    float adjustedWidth = (sceneTextureSize_.y * 16.0f / 9.0f);

    // 元のサイズとの差を計算
    float deltaFromWidth = std::abs(adjustedHeight - sceneTextureSize_.y);
    float deltaFromHeight = std::abs(adjustedWidth - sceneTextureSize_.x);

    // 近い方を採用
    if (deltaFromWidth < deltaFromHeight) {
        sceneTextureSize_.y = adjustedHeight;
    } else {
        sceneTextureSize_.x = adjustedWidth;
    }
}

void ImGuiManager::ShowSceneWindow() {
    // ImGuiウィンドウ開始前にNextWindowSizeは設定しない（手動サイズ変更を許可）
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    // フォーカスされていない場合は描画を最適化
    if (!isShowMainUI_) {
        flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    }

    ImGui::Begin("Scene", nullptr, flags);

    // ウィンドウ内の位置を取得（ImGuizmoのためにシーンウィンドウの絶対位置を計算）
    ImVec2 sceneWindowPos = ImGui::GetWindowPos();
    ImVec2 contentPos = ImGui::GetCursorScreenPos();

    // 以下は既存の処理を最適化
    // キャッシュされた値を使用し、毎フレーム計算しないようにする
    static ImVec2 lastContentRegion = ImVec2(0, 0);
    static ImVec2 lastSceneTextureSize = ImVec2(0, 0);

    // ウィンドウがリサイズされたか、フォーカスがあるときのみ再計算
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    if (contentRegion.x != lastContentRegion.x ||
        contentRegion.y != lastContentRegion.y ||
        ImGui::IsWindowFocused()) {
        lastContentRegion = contentRegion;

        // 横幅ベースで16:9にしたときの高さ
        float adjustedHeight = contentRegion.x * 9.0f / 16.0f;
        // 高さベースで16:9にしたときの横幅
        float adjustedWidth = contentRegion.y * 16.0f / 9.0f;

        // 画面内に収まるように調整
        if (adjustedHeight <= contentRegion.y) {
            lastSceneTextureSize.x = contentRegion.x;
            lastSceneTextureSize.y = adjustedHeight;
        } else {
            lastSceneTextureSize.x = adjustedWidth;
            lastSceneTextureSize.y = contentRegion.y;
        }

        // 計算結果を保存
        sceneTextureSize_ = lastSceneTextureSize;
    }

    // 背景カラー設定
    uint32_t srvIndex = dxCommon_->GetOffScreenSrvIndex();
    static ImVec4 lastBgColor = ImVec4(0, 0, 0, 0);
    ImVec4 backgroundColor;

    // 背景色も必要時のみ更新
    if (ImGui::IsWindowFocused()) {
        backgroundColor = ImVec4(
            dxCommon_->GetClearColor().x,
            dxCommon_->GetClearColor().y,
            dxCommon_->GetClearColor().z,
            dxCommon_->GetClearColor().w);
        lastBgColor = backgroundColor;
    } else {
        backgroundColor = lastBgColor;
    }

    // シーンテクスチャの中央配置のための計算
    ImVec2 sceneOffset;
    sceneOffset.x = (contentRegion.x - sceneTextureSize_.x) * 0.5f;
    sceneOffset.y = (contentRegion.y - sceneTextureSize_.y) * 0.5f;

    // テクスチャ描画位置を調整
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + sceneOffset.x, ImGui::GetCursorPosY() + sceneOffset.y));

    // レンダーテクスチャをImGuiウィンドウに描画
    ImGui::ImageWithBg(
        static_cast<ImTextureID>(SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex).ptr),
        sceneTextureSize_, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
        backgroundColor);

    // ImGuizmoのために正確なシーン位置を計算
    ImVec2 actualScenePos = ImVec2(
        contentPos.x + sceneOffset.x,
        contentPos.y + sceneOffset.y);

    imGuizmoManager_->Update(actualScenePos, sceneTextureSize_);

    ImGui::End();
}

void ImGuiManager::ShowMainUI(OffScreen *offscreen) {

    // ヒエラルキーウィンドウ
    ShowSceneSettingWindow();
    // インスペクターウィンドウ
    ShowObjectSettingWindow();
    // プロジェクトウィンドウを描画
    ShowParticleSettingWindow();
    // FPSを描画
    ShowFPSWindow();
    // オフスクリーンウィンドウを描画
    ShowOffScreenSettingWindow(offscreen);
    // ライトウィンドウを描画
    ShowLightSettingWindow();
}

bool &ImGuiManager::GetIsShowMainUI() {
    return isShowMainUI_;
}

void ImGuiManager::ShowDockSpace() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    // DockSpaceの生成
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

#endif //_DEBUG

void ImGuiManager::DisplayFPS() {
#ifdef _DEBUG
    ImGuiIO &io = ImGui::GetIO();
    // FPSを取得
    float fps = Frame::GetFPS();
    float deltaTime = Frame::DeltaTime() * 1000.0f; // ミリ秒単位に変換

    // FPSを色付きで表示
    ImVec4 color;
    if (fps >= 59.0f) {
        color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 60FPS付近なら緑色
    } else if (fps >= 30.0f) {
        color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // 30-59FPSなら黄色
    } else {
        color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // 30FPS未満なら赤色
    }

    ImGui::TextColored(color, "FPS: %.1f", fps);
    ImGui::TextColored(color, "Frame: %.2f ms", deltaTime);
#endif // _DEBUG
}

// 現在のDockレイアウトを保存
void ImGuiManager::BackupDockLayout() {
    ImGuiContext *context = ImGui::GetCurrentContext();
    if (context) {
        // Dockingレイアウトを文字列として保存
        dockLayoutBackup_ = ImGui::SaveIniSettingsToMemory();
    }
}

// 保存したレイアウトを復元
void ImGuiManager::RestoreDockLayout() {
    if (!dockLayoutBackup_.empty()) {
        // メモリ上の設定を再適用
        ImGui::LoadIniSettingsFromMemory(dockLayoutBackup_.c_str(), dockLayoutBackup_.size());
    }
}

void ImGuiManager::SwitchToEditorMode() {
    if (!isEditorMode_) {
        // ゲームモードからエディターモードへの切替
        SaveCurrentLayout(); // 現在のゲームモードレイアウトを保存
        isEditorMode_ = true;
        LoadLayoutForCurrentMode(); // エディターモードのレイアウトをロード
    }
}

void ImGuiManager::SwitchToGameMode() {
    if (isEditorMode_) {
        // エディターモードからゲームモードへの切替
        SaveCurrentLayout(); // 現在のエディターモードレイアウトを保存
        isEditorMode_ = false;
        LoadLayoutForCurrentMode(); // ゲームモードのレイアウトをロード
    }
}

void ImGuiManager::SaveCurrentLayout() {
    // 現在のモードに応じたファイルにレイアウトを保存
    const char *iniFilePath = isEditorMode_ ? editorIniFilePath_.c_str() : gameIniFilePath_.c_str();

    // メモリからiniデータを取得
    size_t size = 0;
    const char *iniData = ImGui::SaveIniSettingsToMemory(&size);

    // ファイルに書き込み
    FILE *f = nullptr;
    if (fopen_s(&f, iniFilePath, "wt") == 0 && f) {
        fwrite(iniData, sizeof(char), size, f);
        fclose(f);
    }
}

void ImGuiManager::LoadLayoutForCurrentMode() {
    // モードに応じたiniファイルをロード
    const char *iniFilePath = isEditorMode_ ? editorIniFilePath_.c_str() : gameIniFilePath_.c_str();

    // ファイルが存在する場合はロード
    FILE *f = nullptr;
    if (fopen_s(&f, iniFilePath, "rt") == 0 && f) {
        // ファイルサイズを取得
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);

        // バッファを確保してファイル内容を読み込む
        char *buf = new char[size + 1];
        if (buf) {
            size_t read_size = fread(buf, 1, size, f);
            buf[read_size] = 0;

            // 読み込んだデータをImGuiに適用
            ImGui::LoadIniSettingsFromMemory(buf, read_size);

            delete[] buf;
        }
        fclose(f);
    }
    // ファイルが存在しない場合は新規に作成される
}