#pragma once
#include "Easing.h"
#include "Transform/WorldTransform.h"

class FollowCamera;
class Player;

/// <summary>
/// フォローカメラの Rush（突進）専用カメラパーツ
/// 突進中の遠距離追従と、通常カメラへの復帰補間を担当する
/// </summary>
class CameraRush
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="pOwner">所有者のフォローカメラ</param>
    void Init(FollowCamera *pOwner) { pOwner_ = pOwner; }

    /// <summary>
    /// Rush（突進）中の専用カメラ制御
    /// </summary>
    /// <param name="pPlayer">追従対象プレイヤー（nullptr可）</param>
    /// <returns>bool: trueなら専用追従でカメラを確定済み（以降の通常処理を行わない）</returns>
    bool UpdateRushCamera(Player *pPlayer);

    /// <summary>
    /// Rush演出からの復帰補間、または通常時の位置確定を行い行列を更新する
    /// </summary>
    /// <param name="cameraPos">確定先のカメラ位置</param>
    void ApplyCameraPosition(const Hagine::Vector3 &cameraPos);

    /// <summary>
    /// Rush関連のImGui表示（イージング設定）
    /// </summary>
    void DrawImGui();

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    // 閾値・ベクトル定数
    static constexpr float kVectorZero = 0.0f;          ///< ゼロ値
    static constexpr float kUpVectorY = 1.0f;           ///< Y軸上方向
    static constexpr float kParallelThreshold = 0.999f; ///< 平行判定しきい値
    static constexpr float kTimerReset = 0.0f;          ///< タイマーリセット値
    static constexpr float kNormalizedValue = 1.0f;     ///< 正規化基準値
    static constexpr float kMaxBlendValue = 1.0f;       ///< 最大ブレンド値
    static constexpr float kMaxFollowRate = 1.0f;       ///< 最大追従率
    static constexpr float kEasingMaxValue = 1.0f;      ///< イージング最大値

    // Rush関連の倍率
    static constexpr float kHighDistSpeedMultiplier = 3.0f; ///< 高速追従倍率
    static constexpr float kMidDistSpeedMultiplier = 2.0f;  ///< 中速追従倍率
    static constexpr float kRotationSpeedMultiplier = 0.5f; ///< 回転速度倍率
    static constexpr float kRushDirectionBlendRatio = 0.3f; ///< Rush方向ブレンド率

    FollowCamera *pOwner_ = nullptr; ///< 所有者のフォローカメラ

    Hagine::Quaternion rushCameraRotation_;                   ///< Rush中の固定回転
    Hagine::Vector3 rushCameraPosition_;                      ///< Rush中の固定位置
    Hagine::Vector3 rushCameraOffset_ = {0.0f, 8.0f, -20.0f}; ///< Rush中のカメラオフセット

    float rushEnemyBehindOffset_ = 3.0f;         ///< 敵の背後にとる距離
    float rushHighDistThreshold_ = 35.0f;        ///< 高速追従に切り替わる距離
    float rushMidDistThreshold_ = 25.0f;         ///< 中速追従に切り替わる距離
    float rushPosArrivalThreshold_ = 0.5f;       ///< 到着判定しきい値（位置）
    float rushRotationArrivalThreshold_ = 0.01f; ///< 到着判定しきい値（回転）
    float rushCameraResumeDistance_ = 50.0f;     ///< 追従再開距離
    float rushResumeBlendSpeed_ = 8.0f;          ///< Rush復帰時のブレンド速度
    float rushCameraFollowRate_ = 0.3f;          ///< Rush中の追従率

    bool isResumeFromRush_ = false;   ///< Rush復帰中フラグ
    bool isRushCameraActive_ = false; ///< Rushカメラ有効フラグ

    float rushBlendTimer_ = 0.0f;    ///< Rushブレンド用タイマー
    float rushRotationTimer_ = 0.0f; ///< Rush回転補間用タイマー
    float rushResumeTimer_ = 0.0f;   ///< Rush復帰補間用タイマー

    Hagine::EasingType rushCameraEasingType_ = Hagine::EasingType::InQuad;   ///< Rushカメラのイージングタイプ
    Hagine::EasingType rushResumeEasingType_ = Hagine::EasingType::OutCubic; ///< Rush復帰のイージングタイプ
};
