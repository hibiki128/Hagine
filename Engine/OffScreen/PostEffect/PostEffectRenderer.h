#pragma once
#include "PostEffectChain.h"
#include "RendererBuffer.h"

/// @brief ポストエフェクトの描画を担当するクラス
/// PostEffectChainのスロット順にエフェクトをピンポンバッファで適用し
/// 最終結果をfinalResultTextureに書き込む
class PostEffectRenderer {
  public:
    void Initialize(DirectXCommon *dxCommon, SrvManager *srvManager, PipeLineManager *psoManager);

    /// @brief エフェクトチェーンを適用して描画する
    /// @param effectChain スロットベースのエフェクトチェーン
    /// @param deltaTime   時間更新用（timeパラメータを持つエフェクト向け）
    void Draw(PostEffectChain &effectChain, float deltaTime);

    uint32_t GetFinalResultSrvIndex() const { return renderBuffer_.GetFinalResultSrvIndex(); }
    void CopyFinalResultToBackBuffer();

  private:
    /// @brief エフェクトなしで最終結果テクスチャに直接コピー
    void DrawToFinalResult();

    /// @brief 1つのエフェクトを適用して描画する
    /// @param slot             適用するエフェクトスロット
    /// @param isFirstInput     trueならオフスクリーンバッファ、falseならピンポンバッファを入力とする
    /// @param inputPingPong    入力ピンポンバッファのインデックス
    /// @param outputRtvIndex   出力先(-2=最終結果, 0/1=ピンポンバッファ)
    void DrawSingleEffect(const EffectSlot &slot,
                          bool isFirstInput,
                          int inputPingPong,
                          int outputRtvIndex);

    DirectXCommon *dxCommon_   = nullptr;
    SrvManager *srvManager_    = nullptr;
    PipeLineManager *psoManager_ = nullptr;
    RenderBuffer renderBuffer_;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
    D3D12_CPU_DESCRIPTOR_HANDLE finalResultRtvHandle_{};
};
