#pragma once
#include "Windows.h"
#include "externals/imgui/imgui.h"
#include <cstdint>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
// WindowsAPI
class WinApp {
  private:
    static WinApp *instance;
    WinApp() = default;
    ~WinApp() = default;
    WinApp(const WinApp &) = delete;
    const WinApp &operator=(const WinApp &) = delete;

  public: // 静的メンバ関数
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  public: // メンバ関数
    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    /// <returns></returns>
    static WinApp *GetInstance();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// メッセージの処理
    /// </summary>
    /// <returns></returns>
    bool ProcessMessage();

    /// <summary>
    /// フルスクリーンの切り替え
    /// </summary>
    void ToggleFullScreen();

    /// <summary>
    /// getter
    /// </summary>
    /// <returns></returns>
    HWND GetHwnd() const { return hwnd; }
    HINSTANCE GetHInstance() const { return wc.hInstance; }
    bool& IsFullScreen() { return isFullScreen_; }

  public: // 定数
    // クライアント領域のサイズ
    static const int32_t kClientWidth = 1760; // 横
    static const int32_t kClientHeight = 990; // 縦
  private:                                    // メンバ変数
    HWND hwnd = nullptr;                      // ウィンドウハンドル
    WNDCLASS wc{};                            // ウィンドウクラスの設定
    bool isFullScreen_ = false;
    // ウィンドウモードの復元用の矩形
    RECT windowRect_ = {0, 0, kClientWidth, kClientHeight};
};
