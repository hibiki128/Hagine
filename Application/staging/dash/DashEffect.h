#pragma once
#include <particle/gpu/ParticleCSEmitter.h>
#include <type/Vector3.h>

/// <summary>
/// ダッシュ中の演出を管理するクラス
/// 進行方向と逆へ流れる風切りライン（dashWind）をGPUパーティクルで発生させる。
/// 発生速度はワールド空間なので、CPU側で毎フレーム進行方向から計算して
/// エミッター自体を動かしながら流し込む。
/// 空中ダッシュでは発生位置を少し上げ、地上ダッシュではエミッターを縦に伸ばす。
/// </summary>
class DashEffect
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（テンプレートからエミッターを生成）
    /// </summary>
    void Init();

    /// <summary>
    /// 更新処理。emitフラグ残留を防ぐため、発生の有無に関わらず毎フレーム呼ぶ
    /// </summary>
    /// <param name="position">追従対象のワールド座標（足元基準）</param>
    /// <param name="velocity">追従対象の速度（進行方向の算出に使う）</param>
    /// <param name="forward">追従対象の正面方向（低速時の進行方向の代用）</param>
    /// <param name="active">ダッシュ中（発生させる）なら true</param>
    /// <param name="grounded">接地中なら true（地上ダッシュ）。false なら空中ダッシュ</param>
    void Update(const Hagine::Vector3 &position, const Hagine::Vector3 &velocity,
                const Hagine::Vector3 &forward, bool active, bool grounded);

    /// <summary>
    /// GPUパーティクルのComputeフェーズ描画
    /// </summary>
    /// <param name="vp">ビュープロジェクション</param>
    void DrawCompute(const Hagine::ViewProjection &vp);

    /// <summary>
    /// GPUパーティクルのGraphicsフェーズ描画
    /// </summary>
    /// <param name="vp">ビュープロジェクション</param>
    void DrawGraphics(const Hagine::ViewProjection &vp);

    /// <summary>
    /// ImGuiデバッグ表示（エミッター設定の調整用）
    /// </summary>
    void DrawImGui();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 定数定義
    static constexpr float kWindSpeedMin = 14.0f;      // 風切りラインの最低速度
    static constexpr float kWindSpeedMax = 24.0f;      // 風切りラインの最高速度
    static constexpr float kWindSpread = 1.0f;         // 風切りラインの速度ばらつき
    static constexpr float kWindForwardOffset = 2.0f;  // 発生位置を進行方向へずらす距離
    static constexpr float kBodyHeightOffset = 1.0f;   // 体の中心へ合わせる高さオフセット
    static constexpr float kMinMoveSpeed = 0.5f;       // 進行方向を速度から取る最低速度
    static constexpr float kAirDashRiseY = 1.0f;       // 空中ダッシュ時に発生位置をさらに上げる量
    static constexpr float kGroundWindScaleY = 2.5f;   // 地上ダッシュ時のエミッターY方向スケール

    std::unique_ptr<Hagine::ParticleCSEmitter> windEmitter_; // 風切りライン
    Hagine::Vector3 lastDir_ = {0.0f, 0.0f, 1.0f};           // 直近の進行方向
    Hagine::Vector3 baseWindScale_ = {1.0f, 1.0f, 1.0f};     // 生成時のエミッター基準スケール（空中時に戻す用）
};
