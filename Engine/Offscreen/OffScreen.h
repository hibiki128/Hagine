#pragma once
#include "PostEffect/PostEffectChain.h"
#include "PostEffect/PostEffectDataManager.h"
#include "PostEffect/PostEffectRenderer.h"
#include <string>
#include <type/Matrix4x4.h>

/// @brief ポストエフェクト全体を管理するファサードクラス
///
/// 使用例:
///   offScreen_.AddEffect(ShaderMode::kVignette, "ビネット");
///   offScreen_.AddEffect(ShaderMode::kBloom,    "ブルーム", 5); // スロット5に配置
///
///   // パラメータを直接取得して変更
///   if (auto* p = offScreen_.GetEffectParams<VignetteParams>(slotIndex)) {
///       p->GetData().strength = 2.0f;
///   }
class OffScreen {
  public:
    void Initialize();
    void Draw();
    void Setting(); // ImGui

    void SetProjection(Matrix4x4 projectionMatrix);
    uint32_t GetFinalResultSrvIndex() const;
    void CopyFinalResultToBackBuffer();

    // -------------------------------------------------------
    //  エフェクト管理API
    // -------------------------------------------------------

    /// @brief エフェクトを追加する
    /// @param mode      シェーダーモード
    /// @param name      識別名（省略可）
    /// @param slotIndex 配置スロット（-1で自動）
    /// @return 実際に配置されたスロット番号。失敗時-1
    int AddEffect(ShaderMode mode,
                  const std::string &name = "",
                  int slotIndex = -1);

    /// @brief スロット番号を指定して削除する
    bool RemoveEffect(int slotIndex);

    /// @brief 名前で検索して最初にヒットした1つを削除する
    /// @return 削除されたスロット番号。見つからなければ-1
    int RemoveEffectByName(const std::string &name);

    /// @brief 同名のエフェクトをすべて削除する
    /// @return 削除した数
    int RemoveAllEffectsByName(const std::string &name);

    /// @brief エフェクトの有効/無効を切り替える
    bool SetEffectEnabled(int slotIndex, bool enabled);

    /// @brief エフェクトを上に移動する（描画順変更）
    bool MoveEffectUp(int slotIndex);

    /// @brief エフェクトを下に移動する（描画順変更）
    bool MoveEffectDown(int slotIndex);

    /// @brief 指定スロットのパラメータを型付きで取得する
    /// @tparam T  具体的なパラメータ型（例: VignetteParams）
    /// @return パラメータへのポインタ。型不一致またはスロット未使用時はnullptr
    template <typename T>
    T *GetEffectParams(int slotIndex) {
        return effectChain_.GetParams<T>(slotIndex);
    }

    /// @brief 名前からスロット番号を取得する
    /// @return スロット番号。見つからなければ-1
    int FindEffectSlotByName(const std::string &name) {
        return effectChain_.FindSlotByName(name);
    }

  private:
    PostEffectChain effectChain_;
    PostEffectRenderer renderer_;
    PostEffectDataManager dataManager_;

    DirectXCommon *dxCommon_ = nullptr;
    Matrix4x4 projectionMatrix_;
};