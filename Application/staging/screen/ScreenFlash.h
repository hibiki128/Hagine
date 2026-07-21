#pragma once
#include <string>

/// <summary>
/// 必殺技発動時などに画面を「白黒(二値)＋ブルーム＋ブラー」でパッと明滅させる画面演出。
/// ポストエフェクトの Monochrome / Bloom / Blur スロットをまとめて確保・管理し、
/// 指定パターン（点灯→通常→点灯→通常…）で明滅させる。
/// ドラゴンボール スパーキングゼロ風の必殺技演出を想定している。
/// </summary>
class ScreenFlash
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// メインのオフスクリーンに Monochrome / Bloom / Blur スロットを確保する（初期は無効）。
    /// 既に同名スロットがあれば再利用する（多重確保を防ぐ）
    /// </summary>
    void Initialize();

    /// <summary>
    /// 確保したスロットを削除する（所有者の破棄前に呼ぶ）
    /// </summary>
    void Finalize();

    /// <summary>
    /// フラッシュ演出を先頭から開始する
    /// </summary>
    void Trigger();

    /// <summary>
    /// タイムラインを進め、エフェクトのON/OFFを切り替える（毎フレーム呼ぶ）
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    /// <param name="owner">出所ラベル</param>
    void RegisterParams(const std::string &owner);

    /// <summary>演出中かどうか</summary>
    bool IsActive() const { return active_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>Monochrome / Bloom / Blur エフェクトの有効・無効をまとめて切り替える</summary>
    void SetEffectsEnabled(bool enabled);

    /// <summary>各エフェクトの調整パラメータを確保済みスロットへ反映する</summary>
    void ApplyParams();

    /// ===================================================
    /// private variants
    /// ===================================================

    // ─── タイムライン状態 ───
    bool active_ = false;    ///< 演出中フラグ
    float timer_ = 0.0f;     ///< 現フェーズの経過時間
    int flashIndex_ = 0;     ///< 済んだ点灯回数
    bool currentOn_ = false; ///< 現在点灯中か

    // ─── スロット管理 ───
    int monoSlot_ = -1;        ///< Monochrome（完全な白黒）エフェクトのスロット番号
    int bloomSlot_ = -1;       ///< Bloom エフェクトのスロット番号
    int blurSlot_ = -1;        ///< Blur（ラジアルブラー）エフェクトのスロット番号
    bool initialized_ = false; ///< 初期化済みフラグ

    // ─── 調整パラメータ（GameParamで調整可）───
    float onDuration_ = 0.075f;   ///< 1回の点灯（白黒）時間（秒）＝約4.5フレーム(60fps)
    float gapDuration_ = 0.06f;   ///< 点灯と点灯の間の通常表示時間（秒）
    int flashCount_ = 2;          ///< 明滅回数
    float monoThreshold_ = 0.2f;  ///< 白黒二値化の明度閾値（これ以上で白・未満で黒）
    float bloomThreshold_ = 0.6f; ///< ブルームの輝度閾値
    float bloomIntensity_ = 2.0f; ///< ブルームの強度
    float blurWidth_ = 0.003f;    ///< ラジアルブラーのブラー幅

    static constexpr const char *kMonoName = "skillFlashMono";   ///< Monochrome スロットの識別名
    static constexpr const char *kBloomName = "skillFlashBloom"; ///< Bloom スロットの識別名
    static constexpr const char *kBlurName = "skillFlashBlur";   ///< Blur スロットの識別名
};
