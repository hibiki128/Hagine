#pragma once
#include "Object/Base/BaseObject.h"

/// <summary>
/// 地面のゲームオブジェクトクラス
/// ステージの基盤となる固定オブジェクト
/// </summary>
class Ground : public BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="className">クラス名</param>
    void Init(const std::string className) override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    /// <param name="offSet">描画オフセット</param>
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    /// <summary>
    /// ライティングの有効フラグを設定
    /// </summary>
    /// <param name="isLighting">有効にするか</param>
    void SetLighting(bool isLighting) { isLighting_ = isLighting; }
};