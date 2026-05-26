#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "OffScreen.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class DirectXCommon;
class SrvManager;
class SceneManager;
class CollisionManager;

// 後方互換用列挙体
enum class DrawLayer {
    kPreEffect = 0,  // stage 0 相当
    kPostEffect = 1, // UI レイヤー相当
};

struct DrawEntry {
    std::string name;
    int stageIndex = 0; // 0,1,2... = 3Dステージ; kUILayer = UI（ポストエフェクトなし）
    std::function<void(const ViewProjection &)> draw;
    bool enabled = true;
};

/// @brief 描画パイプライン全体を管理するシングルトン
///
/// 【ステージ仕様】
///   stageIndex 0,1,2... : 各ステージで3Dオブジェクトを描画しポストエフェクトを適用。
///                         ステージN+1にはN以前の結果が背景として合成される。
///   stageIndex kUILayer : ポストエフェクトなし。全ステージ結果の上にUI/スプライトを描画。
///
/// 【描画順】
///   stage0 → PostEffect0 → stage1(+bg) → PostEffect1 → ... → UI → SceneTransition
class DrawSystem {
  public:
    static constexpr int kUILayer = -1;

    static DrawSystem *GetInstance() {
        static DrawSystem instance;
        return &instance;
    }

    void Initialize(DirectXCommon *dxCommon, SrvManager *srvManager,
                    OffScreen *offscreen, SceneManager *sceneManager,
                    CollisionManager *collision);

    // -------------------------------------------------------
    // 描画エントリ登録 API
    // -------------------------------------------------------

    /// @brief 後方互換 API (DrawLayer)
    void Register(std::string name, DrawLayer layer,
                  std::function<void(const ViewProjection &)> drawFunc);

    /// @brief マルチステージ API (stageIndex)
    void Register(std::string name, int stageIndex,
                  std::function<void(const ViewProjection &)> drawFunc);

    void Unregister(const std::string &name);
    void Clear();

    // -------------------------------------------------------
    // マルチステージ管理 API
    // -------------------------------------------------------

    /// @brief 新しいレンダリングステージを作成する
    /// @return 割り当てられたステージインデックス
    int CreateStage();

    /// @brief 指定ステージの OffScreen を取得する（ポストエフェクト設定用）
    OffScreen *GetStageOffScreen(int stageIndex);

    // -------------------------------------------------------
    // 描画 / デバッグ
    // -------------------------------------------------------

    void Draw(const ViewProjection &vp);
    void UpdateImGui();

    void SaveConfig(const std::string &fileName = "DrawSystem");
    void LoadConfig(const std::string &fileName = "DrawSystem");

  private:
    DrawSystem() = default;
    ~DrawSystem() = default;
    DrawSystem(const DrawSystem &) = delete;
    DrawSystem &operator=(const DrawSystem &) = delete;

    void RegisterImpl(std::string name, int stageIndex,
                      std::function<void(const ViewProjection &)> drawFunc);

    std::vector<DrawEntry> entries_;

    DirectXCommon *dxCommon_     = nullptr;
    SrvManager *srvManager_      = nullptr;
    SceneManager *sceneManager_  = nullptr;
    CollisionManager *collision_ = nullptr;

    // stageIndex → OffScreen* (所有しないステージ0 + 所有するステージ1以降)
    std::map<int, OffScreen *> stageOffScreens_;
    std::vector<std::unique_ptr<OffScreen>> ownedOffScreens_;
    int nextStageIndex_ = 1;
};
