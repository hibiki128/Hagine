#pragma once
#include <Easing.h>
#include <type/Vector3.h>

class FollowCamera;
class Player;
namespace Hagine {
class DrawLine3D;
}

/// <summary>
/// フォローカメラのロックオンパーツ
/// 肩オフセット・高さオフセット・ロックオン遷移と、視錐台ロックオン（範囲内の敵検出）
/// および視錐台デバッグ描画を担当する
/// </summary>
class CameraLockOn
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（肩・高さオフセットの初期化）
    /// </summary>
    /// <param name="pOwner">所有者のフォローカメラ</param>
    void Init(FollowCamera *pOwner);

    /// <summary>
    /// ロックオンの開始/解除フレームに肩オフセットの目標を切り替える
    /// </summary>
    /// <param name="isCurrentlyLockedOn">今フレームのロックオン状態</param>
    void UpdateLockOnTransition(bool isCurrentlyLockedOn);

    /// <summary>
    /// ロックオン中の肩オフセット目標・高さオフセットを更新する（敵方向からヨー角も更新）
    /// </summary>
    /// <param name="pPlayer">追従対象プレイヤー</param>
    /// <param name="targetPos">追従対象の位置</param>
    /// <param name="velocity">追従対象の速度</param>
    void UpdateLockOnShoulderAndHeight(Player *pPlayer, const Hagine::Vector3 &targetPos, const Hagine::Vector3 &velocity);

    /// <summary>
    /// 肩オフセットを目標値へ補間する（解除時の戻り or 通常追従）
    /// </summary>
    void UpdateShoulderOffset();

    /// <summary>
    /// 視錐台ロックオンの更新処理（視錐台内に敵が入った瞬間に自動ロックオン）
    /// </summary>
    void UpdateFrustumLockOn();

    /// <summary>
    /// 視錐台のデバッグ描画（フラグが有効な場合のみ）
    /// </summary>
    void DrawFrustum();

    /// <summary>
    /// 視錐台のデバッグ描画
    /// </summary>
    /// <param name="pDrawLine3D">ライン描画クラスのポインタ</param>
    void DrawLockOnFrustum(Hagine::DrawLine3D *pDrawLine3D) const;

    /// <summary>
    /// ロックオン関連のImGui表示（肩・高さ・イージング・視錐台）
    /// </summary>
    void DrawImGui();

    /// ===================================================
    /// Getter（ファサードの ComputeCameraTransform から参照）
    /// ===================================================
    float GetShoulderOffsetX() const { return shoulderOffsetCurrent_.x; }
    float GetLockOnHeightOffsetCurrent() const { return lockOnHeightOffsetCurrent_; }
    float GetLockOnRange() const { return lockOnRange_; }
    float GetLockOnHalfFovH() const { return lockOnHalfFovH_; }
    float GetLockOnHalfFovV() const { return lockOnHalfFovV_; }
    bool GetDrawLockOnFrustumDebug() const { return drawLockOnFrustumDebug_; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetLockOnRange(float range) { lockOnRange_ = range; }
    void SetLockOnHalfFovH(float rad) { lockOnHalfFovH_ = rad; }
    void SetLockOnHalfFovV(float rad) { lockOnHalfFovV_ = rad; }
    void SetDrawLockOnFrustumDebug(bool flag) { drawLockOnFrustumDebug_ = flag; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 敵方向へヨー角を更新し、更新後のヨー角を返す
    /// </summary>
    /// <param name="targetPos">追従対象の位置</param>
    /// <param name="enemyPos">敵の位置</param>
    /// <returns>float: 更新後のヨー角</returns>
    float UpdateYawTowardEnemy(const Hagine::Vector3 &targetPos, const Hagine::Vector3 &enemyPos);

    /// <summary>
    /// プレイヤーの移動入力（キー/スティック）があるかを判定する
    /// </summary>
    /// <param name="pPlayer">追従対象プレイヤー</param>
    /// <returns>bool: 入力があれば true</returns>
    bool HasMovementInput(Player *pPlayer) const;

    /// <summary>
    /// 横方向速度に応じて肩オフセットの目標値を更新する
    /// </summary>
    /// <param name="lateralVelocity">カメラ右方向への速度成分</param>
    void UpdateShoulderOffsetTarget(float lateralVelocity);

    /// <summary>
    /// 接地状態に応じた高さオフセットの目標設定と補間を行う
    /// </summary>
    /// <param name="pPlayer">追従対象プレイヤー</param>
    void UpdateHeightOffset(Player *pPlayer);

    /// <summary>
    /// 指定した点がロックオン視錐台内にあるか判定
    /// </summary>
    /// <param name="point">判定する座標</param>
    /// <returns>bool: 範囲内であればtrue</returns>
    bool IsPointInLockOnFrustum(const Hagine::Vector3 &point) const;

    /// ===================================================
    /// private variables
    /// ===================================================

    // 閾値定数
    static constexpr float kEpsilon = 0.001f;               ///< 微小値
    static constexpr float kVelocityThreshold = 0.1f;       ///< 速度判定しきい値
    static constexpr float kShoulderTargetThreshold = 0.1f; ///< 肩オフセット目標到達しきい値
    static constexpr float kShoulderDiffThreshold = 0.01f;  ///< 肩オフセット誤差許容値
    static constexpr float kHeightDiffThreshold = 0.01f;    ///< 高さ誤差許容値
    static constexpr float kParallelThreshold = 0.999f;     ///< 平行判定しきい値

    // ベクトル・ブレンド定数
    static constexpr float kVectorZero = 0.0f;     ///< ゼロ値
    static constexpr float kTimerReset = 0.0f;     ///< タイマーリセット値
    static constexpr float kMaxBlendValue = 1.0f;  ///< 最大ブレンド値
    static constexpr float kEasingMaxValue = 1.0f; ///< イージング最大値

    // 視錐台ロックオン関連
    static constexpr float kDefaultLockOnRange = 150.0f;                            ///< デフォルトのロックオン射程
    static constexpr float kDefaultLockOnHalfFovH = 36.5f * (3.14159265f / 180.0f); ///< 水平視野角の半分
    static constexpr float kDefaultLockOnHalfFovV = 23.0f * (3.14159265f / 180.0f); ///< 垂直視野角の半分
    static constexpr float kFrustumDebugNear = 1.0f;                                ///< デバッグ描画の近面距離

    FollowCamera *pOwner_ = nullptr; ///< 所有者のフォローカメラ

    Hagine::Vector3 shoulderOffsetTarget_ = {0.0f, 0.0f, 0.0f};  ///< 肩オフセット目標値
    Hagine::Vector3 shoulderOffsetCurrent_ = {0.0f, 0.0f, 0.0f}; ///< 現在の肩オフセット
    Hagine::Vector3 shoulderOffsetStart_ = {0.0f, 0.0f, 0.0f};   ///< 補間開始時の肩オフセット

    float shoulderMaxOffset_ = 12.5f; ///< 肩の最大ズレ幅
    float shoulderLerpSpeed_ = 10.0f; ///< 肩オフセットの補間速度

    bool wasLockedOn_ = false; ///< 前フレームのロックオン状態

    bool isResettingShoulderOffset_ = false; ///< 肩オフセットリセット中フラグ
    float shoulderResetTimer_ = 0.0f;        ///< 肩リセット用タイマー
    float shoulderResetDuration_ = 0.5f;     ///< 肩リセット時間

    float shoulderLerpTimer_ = 0.0f;      ///< 肩補間用タイマー
    float shoulderLerpStartValue_ = 0.0f; ///< 肩補間開始値

    float lockOnHeightOffsetCurrent_ = 5.0f; ///< 現在の高さオフセット
    float lockOnHeightOffsetTarget_ = 5.0f;  ///< 目標の高さオフセット
    float lockOnGroundedHeight_ = 5.0f;      ///< 接地時の高さオフセット
    float lockOnAirborneHeight_ = 0.0f;      ///< 空中時の高さオフセット
    float lockOnHeightLerpSpeed_ = 5.0f;     ///< 高さオフセット補間速度

    Hagine::EasingType shoulderEasingType_ = Hagine::EasingType::InQuad;        ///< 肩補間のイージングタイプ
    Hagine::EasingType shoulderResetEasingType_ = Hagine::EasingType::OutCubic; ///< 肩リセットのイージングタイプ

    float lockOnRange_ = kDefaultLockOnRange;       ///< ロックオン有効距離
    float lockOnHalfFovH_ = kDefaultLockOnHalfFovH; ///< ロックオン水平半角
    float lockOnHalfFovV_ = kDefaultLockOnHalfFovV; ///< ロックオン垂直半角
    bool drawLockOnFrustumDebug_ = false;           ///< 視錐台デバッグ描画フラグ
};
