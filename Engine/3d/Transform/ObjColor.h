#pragma once
#include"type/Vector4.h"
#include"d3d12.h"
#include"wrl.h"
#include"DirectXCommon.h"
namespace Hagine::Transform {
// 定数バッファ用データ構造体
struct ConstBufferDataObjColor {
    Math::Vector4 color_;
};

class ObjColor {
  public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 行列の転送
    /// </summary>
    void TransferMatrix();

    /// <summary>
    /// グラフィックスコマンドを積む
    /// </summary>
    void SetGraphicCommand(UINT rootParameterIndex) const;

    const Math::Vector4 GetColor() {
        return color_;
    }

    void SetColor(Math::Vector4 color) {
        color_ = color;
    }

  private:
    Core::DirectXCommon *dxCommon_ = nullptr;

    Math::Vector4 color_ = {
        1.0f,
        1.0f,
        1.0f,
        1.0f,
    };
    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    // マッピング済みアドレス
    ConstBufferDataObjColor *constMap_ = nullptr;

    /// <summary>
    /// 定数バッファ生成
    /// </summary>
    void CreateConstBuffer();

    /// <summary>
    /// マッピングする
    /// </summary>
    void Map();
};
} // namespace Hagine::Transform
