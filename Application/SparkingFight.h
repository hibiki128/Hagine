#pragma once
#include "Framework.h"

/// <summary>
/// ゲーム本体クラス
/// Hagine::Framework を継承し、SparkingFight 固有のリソース読み込み・更新処理を行う
/// </summary>
class SparkingFight : public Hagine::Framework {
  public: // メンバ関数
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize() override;

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize() override;

    /// <summary>
    /// 更新
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画
    /// </summary>
    void Draw() override;

  private:
    /// <summary>
    /// ゲーム固有リソースの読み込み（パーティクルエミッター・フィールド）
    /// </summary>
    void LoadGameResources();
};
