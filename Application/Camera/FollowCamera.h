#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Easing.h"
#include "Transform/WorldTransform.h"

class Player;
namespace Hagine { class DrawLine3D; }

/// <summary>
/// ターゲットを追従するカメラクラス
/// </summary>
class FollowCamera {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Init();

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// デバッグ用のImGui表示
    /// </summary>
    void imgui();

    /// <summary>
    /// 視錐台の描画
    /// </summary>
    void DrawFrustum();

    /// <summary>
    /// 視錐台ロックオンの更新処理
    /// 視錐台内に敵が入った際、自動的にロックオンを試みる
    /// </summary>
    void UpdateFrustumLockOn();

    /// <summary>
    /// 視錐台のデバッグ描画
    /// </summary>
    /// <param name="drawLine3D">ライン描画クラスのポインタ</param>
    void DrawLockOnFrustum(Hagine::DrawLine3D *drawLine3D) const;

    /// ===================================================
    /// Getter
    /// ===================================================

    /// <summary>
    /// ヨー角を取得
    /// </summary>
    /// <returns>float: ヨー角</returns>
    float GetYaw() { return yaw_; }

    /// <summary>
    /// ビュープロジェクションを取得
    /// </summary>
    /// <returns>ViewProjection&: ビュープロジェクション参照</returns>
    Hagine::ViewProjection &GetViewProjection() { return viewProjection_; }

    /// <summary>
    /// ロックオン有効距離を取得
    /// </summary>
    /// <returns>float: ロックオン距離</returns>
    float GetLockOnRange() const { return lockOnRange_; }

    /// <summary>
    /// ロックオン水平半角を取得
    /// </summary>
    /// <returns>float: 水平半角（ラジアン）</returns>
    float GetLockOnHalfFovH() const { return lockOnHalfFovH_; }

    /// <summary>
    /// ロックオン垂直半角を取得
    /// </summary>
    /// <returns>float: 垂直半角（ラジアン）</returns>
    float GetLockOnHalfFovV() const { return lockOnHalfFovV_; }

    /// <summary>
    /// 視錐台デバッグ描画フラグを取得
    /// </summary>
    /// <returns>bool: 描画フラグ</returns>
    bool GetDrawLockOnFrustumDebug() const { return drawLockOnFrustumDebug_; }

    /// ===================================================
    /// Setter
    /// ===================================================

    /// <summary>
    /// 追従対象を設定
    /// </summary>
    /// <param name="target">ターゲットプレイヤーのポインタ</param>
    void SetPlayer(Player *target) { target_ = target; }

    /// <summary>
    /// カメラの視野角を設定
    /// </summary>
    /// <param name="fov">視野角（度数）</param>
    void SetCameraFov(float fov) {
        viewProjection_.fovAngleY_ = fov * std::numbers::pi_v<float> / 180.0f;
    }

    /// <summary>
    /// ロックオン有効距離を設定
    /// </summary>
    /// <param name="range">有効距離</param>
    void SetLockOnRange(float range) { lockOnRange_ = range; }

    /// <summary>
    /// ロックオン水平半角を設定
    /// </summary>
    /// <param name="rad">水平半角（ラジアン）</param>
    void SetLockOnHalfFovH(float rad) { lockOnHalfFovH_ = rad; }

    /// <summary>
    /// ロックオン垂直半角を設定
    /// </summary>
    /// <param name="rad">垂直半角（ラジアン）</param>
    void SetLockOnHalfFovV(float rad) { lockOnHalfFovV_ = rad; }

    /// <summary>
    /// 視錐台デバッグ描画フラグを設定
    /// </summary>
    /// <param name="flag">描画フラグ</param>
    void SetDrawLockOnFrustumDebug(bool flag) { drawLockOnFrustumDebug_ = flag; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// カメラの移動計算
    /// </summary>
    void Move();

    /// <summary>
    /// ロックオンの開始/解除フレームに肩オフセットの目標を切り替える
    /// </summary>
    /// <param name="isCurrentlyLockedOn">今フレームのロックオン状態</param>
    void UpdateLockOnTransition(bool isCurrentlyLockedOn);

    /// <summary>
    /// Rush（突進）中の専用カメラ制御
    /// </summary>
    /// <param name="player">追従対象プレイヤー（nullptr可）</param>
    /// <returns>bool: trueなら専用追従でカメラを確定済み（以降の通常処理を行わない）</returns>
    bool UpdateRushCamera(Player *player);

    /// <summary>
    /// ロックオン中の肩オフセット目標・高さオフセットを更新する（敵方向からヨー角も更新）
    /// </summary>
    /// <param name="player">追従対象プレイヤー</param>
    /// <param name="targetPos">追従対象の位置</param>
    /// <param name="velocity">追従対象の速度</param>
    void UpdateLockOnShoulderAndHeight(Player *player, const Hagine::Vector3 &targetPos, const Hagine::Vector3 &velocity);

    /// <summary>
    /// 肩オフセットを目標値へ補間する（解除時の戻り or 通常追従）
    /// </summary>
    void UpdateShoulderOffset();

    /// <summary>
    /// ロックオン有無に応じて最終的なカメラ位置を算出し、回転を worldTransform_ に設定する
    /// </summary>
    /// <param name="isCurrentlyLockedOn">今フレームのロックオン状態</param>
    /// <param name="player">追従対象プレイヤー</param>
    /// <param name="targetPos">追従対象の位置</param>
    /// <returns>Vector3: 算出したカメラ位置</returns>
    Hagine::Vector3 ComputeCameraTransform(bool isCurrentlyLockedOn, Player *player, const Hagine::Vector3 &targetPos);

    /// <summary>
    /// Rush演出からの復帰補間、または通常時の位置確定を行い行列を更新する
    /// </summary>
    /// <param name="cameraPos">確定先のカメラ位置</param>
    void ApplyCameraPosition(const Hagine::Vector3 &cameraPos);

    /// <summary>
    /// worldTransform_ の位置・回転を ViewProjection へ反映して行列を更新する
    /// </summary>
    void ApplyToViewProjection();

    /// <summary>
    /// 指定した点がロックオン視錐台内にあるか判定
    /// </summary>
    /// <param name="point">判定する座標</param>
    /// <returns>bool: 範囲内であればtrue</returns>
    bool IsPointInLockOnFrustum(const Hagine::Vector3 &point) const;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // カメラ設定
    static constexpr float kFarZ = 1100.0f; // 描画距離の遠面

    // 閾値定数
    static constexpr float kEpsilon = 0.001f;               // 微小値
    static constexpr float kRotationEpsilon = 0.01f;        // 回転誤差許容値
    static constexpr float kParallelThreshold = 0.999f;     // 平行判定しきい値
    static constexpr float kVelocityThreshold = 0.1f;       // 速度判定しきい値
    static constexpr float kShoulderTargetThreshold = 0.1f; // 肩オフセット目標到達しきい値
    static constexpr float kShoulderDiffThreshold = 0.01f;  // 肩オフセット誤差許容値
    static constexpr float kHeightDiffThreshold = 0.01f;    // 高さ誤差許容値

    // ベクトル定数
    static constexpr float kVectorZero = 0.0f; // ゼロ値
    static constexpr float kUpVectorY = 1.0f;   // Y軸上方向
    static constexpr float kRightVectorX = 1.0f; // X軸右方向

    // 初期値
    static constexpr float kInitialYaw = 0.0f; // 初期ヨー角
    static constexpr float kTimerReset = 0.0f; // タイマーリセット値

    // 正規化・クランプ値
    static constexpr float kNormalizedValue = 1.0f; // 正規化基準値
    static constexpr float kMaxBlendValue = 1.0f;   // 最大ブレンド値
    static constexpr float kMaxFollowRate = 1.0f;   // 最大追従率
    static constexpr float kMinClamp = -1.0f;       // 最小クランプ値
    static constexpr float kMaxClamp = 1.0f;        // 最大クランプ値
    static constexpr float kEasingMaxValue = 1.0f;  // イージング最大値

    // Rush関連の倍率
    static constexpr float kHighDistSpeedMultiplier = 3.0f;  // 高速追従倍率
    static constexpr float kMidDistSpeedMultiplier = 2.0f;   // 中速追従倍率
    static constexpr float kRotationSpeedMultiplier = 0.5f;  // 回転速度倍率
    static constexpr float kRushDirectionBlendRatio = 0.3f; // Rush方向ブレンド率

    float manualYawSpeed_ = 0.04f;               ///< 手動回転速度
    float rushEnemyBehindOffset_ = 3.0f;         ///< 敵の背後にとる距離
    float rushHighDistThreshold_ = 35.0f;        ///< 高速追従に切り替わる距離
    float rushMidDistThreshold_ = 25.0f;         ///< 中速追従に切り替わる距離
    float rushPosArrivalThreshold_ = 0.5f;       ///< 到着判定しきい値（位置）
    float rushRotationArrivalThreshold_ = 0.01f; ///< 到着判定しきい値（回転）

    Hagine::ViewProjection viewProjection_; ///< ビュープロジェクション
    Hagine::WorldTransform worldTransform_; ///< ワールドトランスフォーム

    Player *target_ = nullptr; ///< 追従対象のプレイヤー

    Hagine::Quaternion rushCameraRotation_; ///< Rush中の固定回転

    Hagine::Vector3 cameraOffset_ = {0.0f, 5.0f, -25.0f};        ///< ベースのカメラオフセット
    Hagine::Vector3 shoulderOffsetTarget_ = {0.0f, 0.0f, 0.0f};  ///< 肩オフセット目標値
    Hagine::Vector3 shoulderOffsetCurrent_ = {0.0f, 0.0f, 0.0f}; ///< 現在の肩オフセット
    Hagine::Vector3 shoulderOffsetStart_ = {0.0f, 0.0f, 0.0f};   ///< 補間開始時の肩オフセット
    Hagine::Vector3 rushCameraPosition_;                         ///< Rush中の固定位置
    Hagine::Vector3 rushCameraOffset_ = {0.0f, 8.0f, -20.0f};    ///< Rush中のカメラオフセット

    float yaw_{};                            ///< 現在のヨー角
    float shoulderMaxOffset_ = 12.5f;        ///< 肩の最大ズレ幅
    float shoulderLerpSpeed_ = 10.0f;        ///< 肩オフセットの補間速度
    float rushCameraResumeDistance_ = 50.0f; ///< 追従再開距離
    float rushResumeBlendSpeed_ = 8.0f;      ///< Rush復帰時のブレンド速度
    float rushCameraFollowRate_ = 0.3f;      ///< Rush中の追従率
    bool isResumeFromRush_ = false;          ///< Rush復帰中フラグ
    bool isRushCameraActive_ = false;        ///< Rushカメラ有効フラグ

    bool wasLockedOn_ = false; ///< 前フレームのロックオン状態

    bool isResettingShoulderOffset_ = false; ///< 肩オフセットリセット中フラグ
    float shoulderResetTimer_ = 0.0f;        ///< 肩リセット用タイマー
    float shoulderResetDuration_ = 0.5f;     ///< 肩リセット時間

    float lockOnHeightOffsetCurrent_ = 5.0f; ///< 現在の高さオフセット
    float lockOnHeightOffsetTarget_ = 5.0f;  ///< 目標の高さオフセット
    float lockOnGroundedHeight_ = 5.0f;      ///< 接地時の高さオフセット
    float lockOnAirborneHeight_ = 0.0f;      ///< 空中時の高さオフセット
    float lockOnHeightLerpSpeed_ = 5.0f;     ///< 高さオフセット補間速度

    float shoulderLerpTimer_ = 0.0f;      ///< 肩補間用タイマー
    float shoulderLerpStartValue_ = 0.0f; ///< 肩補間開始値
    float rushBlendTimer_ = 0.0f;         ///< Rushブレンド用タイマー
    float rushRotationTimer_ = 0.0f;      ///< Rush回転補間用タイマー
    float rushResumeTimer_ = 0.0f;        ///< Rush復帰補間用タイマー

    Hagine::EasingType shoulderEasingType_ = Hagine::EasingType::InQuad;        ///< 肩補間のイージングタイプ
    Hagine::EasingType shoulderResetEasingType_ = Hagine::EasingType::OutCubic; ///< 肩リセットのイージングタイプ
    Hagine::EasingType rushCameraEasingType_ = Hagine::EasingType::InQuad;      ///< Rushカメラのイージングタイプ
    Hagine::EasingType rushResumeEasingType_ = Hagine::EasingType::OutCubic;    ///< Rush復帰のイージングタイプ

    // 視錐台ロックオン関連
    static constexpr float kDefaultLockOnRange = 150.0f;                            ///< デフォルトのロックオン射程
    static constexpr float kDefaultLockOnHalfFovH = 36.5f * (3.14159265f / 180.0f); ///< 水平視野角の半分
    static constexpr float kDefaultLockOnHalfFovV = 23.0f * (3.14159265f / 180.0f); ///< 垂直視野角の半分
    static constexpr float kFrustumDebugNear = 1.0f;                                 ///< デバッグ描画の近面距離

    float lockOnRange_ = kDefaultLockOnRange;       ///< ロックオン有効距離
    float lockOnHalfFovH_ = kDefaultLockOnHalfFovH; ///< ロックオン水平半角
    float lockOnHalfFovV_ = kDefaultLockOnHalfFovV; ///< ロックオン垂直半角
    bool drawLockOnFrustumDebug_ = false;           ///< 視錐台デバッグ描画フラグ
};
