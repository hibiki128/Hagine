#pragma once
#include <particle/gpu/ParticleCSEmitter.h>
#include <type/Vector3.h>

/// <summary>
/// 足元の演出を管理するクラス
/// ・着地の砂煙（landing）: 空中から接地した瞬間に一度だけ発生させる
/// ・走行中の砂煙（running）: 接地して走っている間、進行方向と逆へ流し続ける
/// 発生速度はワールド空間なので、走行側はCPUで毎フレーム進行方向から計算して
/// エミッターを足元へ追従させながら流し込む。
/// </summary>
class FootEffect
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
    /// <param name="velocity">追従対象の速度（進行方向・着地の落下速度に使う）</param>
    /// <param name="grounded">接地中なら true</param>
    /// <param name="active">演出を発生させてよい状態なら true（死亡・ポーズ中は false）</param>
    void Update(const Hagine::Vector3 &position, const Hagine::Vector3 &velocity,
                bool grounded, bool active);

    /// <summary>
    /// ImGuiデバッグ表示（エミッター設定の調整用）
    /// </summary>
    void DrawImGui();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 定数定義
    static constexpr float kLandingMinFallSpeed = 4.0f; // 着地演出を出す最低落下速度（これ未満の段差は無視）
    static constexpr float kLandingOffsetY = -1.0f;     // 着地演出の発生位置の高さオフセット（足元へ下げる）
    static constexpr float kDirEpsilon = 0.001f;        // 進行方向を更新する最低水平速度
    static constexpr float kRunMinSpeed = 8.0f;         // 走行演出を出す最低水平速度（最大速度20の4割）
    static constexpr float kRunStartDelay = 0.2f;       // この時間だけ走り続けて初めて発生させる（一瞬の入力を弾く）
    static constexpr float kRunSpeedMin = 5.0f;         // 砂煙が後方へ流れる最低速度
    static constexpr float kRunSpeedMax = 7.0f;         // 砂煙が後方へ流れる最高速度
    static constexpr float kRunSideSpread = 7.0f;       // 砂煙の左右方向のばらつき
    static constexpr float kRunUpSpread = 1.0f;         // 砂煙の上下方向のばらつき
    static constexpr float kRunBackOffset = 0.3f;       // 発生位置を進行方向の後ろへずらす距離
    static constexpr float kRunOffsetY = -1.0f;         // 走行演出の発生位置の高さオフセット（足元へ下げる）

    // 着地/走行の砂煙。実体は ParticleCSSpawner が所有する（ここは借用ポインタ）。
    // シーン遷移時に Spawner 側でまとめて破棄されるため、明示的な後始末は不要。
    Hagine::ParticleCSEmitter *landingEmitter_ = nullptr; // 着地の砂煙
    Hagine::ParticleCSEmitter *runEmitter_ = nullptr;     // 走行中の砂煙

    Hagine::Vector3 lastDir_ = {0.0f, 0.0f, 1.0f}; // 直近の進行方向（水平成分）
    bool wasGrounded_ = true;                      // 前フレームの接地状態（着地の瞬間の検出用）
    float fallSpeed_ = 0.0f;                       // 空中での落下速度（着地の強さの判定用・正の値）
    float runTimer_ = 0.0f;                        // 走り続けている時間（止まるとゼロに戻す）
};
