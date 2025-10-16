#pragma once
#include "DirectXCommon.h"
#include <Graphics/PipeLine/PipeLineManager.h>

/// <summary>
/// スプライト描画に必要な共通処理を管理するシングルトンクラス
/// DirectX、パイプライン、ブレンドモードなどを制御
/// </summary>
class SpriteCommon {
  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// プライベートコンストラクタ
    /// </summary>
    SpriteCommon() = default;

    /// <summary>
    /// プライベートデストラクタ
    /// </summary>
    ~SpriteCommon() = default;

    /// <summary>
    /// コピーコンストラクタ削除
    /// </summary>
    SpriteCommon(SpriteCommon &) = delete;

    /// <summary>
    /// 代入演算子削除
    /// </summary>
    SpriteCommon &operator=(SpriteCommon &) = delete;

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    static SpriteCommon *instance;
    DirectXCommon *dxCommon_;
    PipeLineManager *psoManager_ = nullptr;

  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// シングルトンインスタンスを取得
    /// </summary>
    /// <returns>SpriteCommon*: インスタンスのポインタ</returns>
    static SpriteCommon *GetInstance();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 共通描画設定を適用
    /// </summary>
    void DrawCommonSetting();

    /// <summary>
    /// DirectXCommonを取得
    /// </summary>
    /// <returns>DirectXCommon*: DirectXCommonのポインタ</returns>
    DirectXCommon *GetDxCommon() const { return dxCommon_; }

    /// <summary>
    /// ブレンドモードを設定
    /// </summary>
    /// <param name="blendMode">設定するブレンドモード</param>
    void SetBlendMode(BlendMode blendMode);
};