#pragma once
#include "d3d12.h"
// dxcapi.h を先に include する必要がある（ShaderCompiler.h が面倒を見ている）
#include <shader/ShaderCompiler.h>
#include <string>
#include <vector>
#include <wrl.h>

namespace Hagine {
class DirectXCommon;

/// <summary>
/// シェーダーリフレクションからルートシグネチャを自動生成するヘルパー。
///
/// これまではシェーダーを1つ足すたびに、そのシェーダーが使う定数バッファ・テクスチャの数だけ
/// D3D12_ROOT_PARAMETER を手書きする関数を1つ増やしていた（ポストエフェクト17種で34関数）。
/// コンパイル済みバイナリにはどのレジスタに何がバインドされているかが入っているので、
/// それを読んでルートシグネチャを組めば、シェーダーを追加するだけで済むようになる。
///
/// 生成されるルートシグネチャの規約:
///   - b# (ConstantBuffer)  → ルートCBV（レジスタ番号の昇順）
///   - t# (Texture/SRV)     → 1つのデスクリプタテーブル（連続レンジ）にまとめる
///   - u# (RWTexture/UAV)   → 1つのデスクリプタテーブル（連続レンジ）にまとめる
///   - s# (SamplerState)    → スタティックサンプラー（下記 SamplerPreset から選ぶ）
/// ルートパラメータの並びは「CBV群 → SRVテーブル → UAVテーブル」の順で、
/// 各リソースのルートパラメータ番号は GetRootParameterIndex() で引ける。
/// </summary>
class ShaderRootSignature
{
  public:
    /// <summary>
    /// スタティックサンプラーの内容。シェーダー側の s# の並びに対応させる。
    /// </summary>
    enum class SamplerPreset
    {
        LinearClamp, // 線形補間・端はクランプ（画面全体を扱うポストエフェクトの既定）
        PointClamp,  // 補間なし・端はクランプ（深度など補間してはいけないもの）
        LinearWrap,  // 線形補間・繰り返し
    };

    /// <summary>
    /// リフレクションを読んでルートシグネチャを構築する
    /// </summary>
    /// <param name="pDxCommon">デバイス取得用</param>
    /// <param name="pReflection">対象シェーダーのリフレクション</param>
    /// <param name="samplerPresets">s0 から順に割り当てるサンプラー設定。
    /// 足りない分は LinearClamp で埋める</param>
    /// <param name="debugName">失敗時のログに出す名前</param>
    /// <returns>bool: 構築に成功したか</returns>
    bool BuildFromReflection(DirectXCommon *pDxCommon,
                             ID3D12ShaderReflection *pReflection,
                             const std::vector<SamplerPreset> &samplerPresets,
                             const std::string &debugName);

    /// <summary>
    /// 構築したルートシグネチャを取得する
    /// </summary>
    ID3D12RootSignature *Get() const { return rootSignature_.Get(); }

    /// <summary>
    /// 指定レジスタに対応するルートパラメータ番号を取得する
    /// SRV / UAV はテーブルにまとめているので、同じ種類なら同じ番号が返る
    /// </summary>
    /// <param name="type">リソース種別</param>
    /// <param name="shaderRegister">レジスタ番号（b0 なら 0）</param>
    /// <returns>UINT: ルートパラメータ番号。見つからなければ UINT_MAX</returns>
    UINT GetRootParameterIndex(D3D12_DESCRIPTOR_RANGE_TYPE type, UINT shaderRegister) const;

    /// <summary>SRV をまとめたデスクリプタテーブルのルートパラメータ番号（無ければ UINT_MAX）</summary>
    UINT GetSrvTableIndex() const { return srvTableIndex_; }

    /// <summary>UAV をまとめたデスクリプタテーブルのルートパラメータ番号（無ければ UINT_MAX）</summary>
    UINT GetUavTableIndex() const { return uavTableIndex_; }

    /// <summary>シェーダーが宣言している SRV の数</summary>
    UINT GetSrvCount() const { return srvCount_; }

    /// <summary>シェーダーが宣言している UAV の数</summary>
    UINT GetUavCount() const { return uavCount_; }

  private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

    // b# → ルートパラメータ番号（添字がレジスタ番号）
    std::vector<UINT> cbvRootIndices_;

    UINT srvTableIndex_ = UINT_MAX; // t# をまとめたテーブルのルートパラメータ番号
    UINT uavTableIndex_ = UINT_MAX; // u# をまとめたテーブルのルートパラメータ番号
    UINT srvCount_ = 0;
    UINT uavCount_ = 0;
};
} // namespace Hagine
