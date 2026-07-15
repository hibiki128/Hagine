#pragma once
#include <animation/AnimationController.h>

class Enemy;

/// <summary>
/// 敵の見た目パーツクラス
/// アニメーションクリップの切り替えと、ガード・被弾に伴う色（点滅）を担当する。
/// アニメーション構成はプレイヤーと同一のクリップを流用する
/// </summary>
class EnemyVisual
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（アニメーションクリップの登録を行う）
    /// </summary>
    /// <param name="owner">所有者の敵</param>
    void Init(Enemy *owner);

    /// <summary>
    /// 移動・コンボ・ガード状態に応じてアニメーションクリップを切り替える
    /// 敵はBT駆動で moveDir_ を持たないため、向きと velocity から進行方向を判定する
    /// </summary>
    void UpdateAnimation();

    /// <summary>
    /// 死亡アニメーションを再生する（死亡中は毎フレーム呼んでよい。同一クリップは無視される）
    /// </summary>
    void PlayDeathAnimation();

    /// <summary>
    /// 死亡アニメーションが最後まで再生し終わったか
    /// （粒子化して消える死亡演出の開始判定に使う）
    /// </summary>
    /// <returns>bool: 再生終了していれば true</returns>
    bool IsDeathAnimationFinished() const;

    /// <summary>
    /// ガード状態に応じた体色（点滅）を更新する
    /// </summary>
    void UpdateGuardBlink();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 色関連定数
    static constexpr float kColorRed = 1.0f;
    static constexpr float kColorZero = 0.0f;
    static constexpr float kColorOpaque = 1.0f;

    // 点滅関連定数
    static constexpr float kBlinkInterval = 0.1f;
    static constexpr int kBlinkModulo = 2;
    static constexpr int kEvenBlink = 0;

    // アニメーション切り替え閾値
    static constexpr float kFlyVerticalAnimThreshold = 0.5f; ///< 飛行中、上昇/下降アニメに切り替えるY速度の閾値
    static constexpr float kMoveAnimMinSpeed = 0.1f;         ///< 待機/移動アニメを切り替える水平速度の閾値
    static constexpr float kBackwardDotThreshold = 0.5f;     ///< 後退とみなす（前方ベクトルとの内積）の閾値
    static constexpr float kMinRotationDistance = 0.001f;    ///< 進行方向判定の最小距離

    Enemy *pOwner_ = nullptr; ///< 所有者の敵

    Hagine::AnimationController animationController_; ///< アニメーション制御（プレイヤーと同一クリップ構成）
};
