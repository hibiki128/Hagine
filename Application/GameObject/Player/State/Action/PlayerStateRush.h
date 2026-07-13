#pragma once
#include "../Base/PlayerBaseState.h"
#include "Application/Utility/Shake/Shake.h"
#include <Particle/ParticleEmitter.h>
#include <type/Quaternion.h>
#include <type/Vector3.h>

/// <summary>
/// プレイヤーの突撃状態を管理するクラス
/// 弧を描く軌道で敵に向かって移動する
/// </summary>
class PlayerStateRush : public PlayerBaseState
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    PlayerStateRush() = default;

    /// <summary>
    /// 状態開始時の処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void Enter(Player &player) override;

    /// <summary>
    /// 状態更新処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void Update(Player &player) override;

    /// <summary>
    /// 状態終了時の処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void Exit(Player &player) override;

    /// <summary>
    /// パーティクル描画処理
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(Player &player, const Hagine::ViewProjection &viewProjection) override;

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 指定方向を向くクォータニオンを計算
    /// </summary>
    /// <param name="forward">前方向ベクトル</param>
    /// <param name="up">上方向ベクトル</param>
    /// <returns>Quaternion: 計算されたクォータニオン</returns>
    Hagine::Quaternion LookRotation(const Hagine::Vector3 &forward, const Hagine::Vector3 &up);

    /// <summary>
    /// 弧形の移動経路を計算
    /// </summary>
    /// <param name="startPos">開始位置</param>
    /// <param name="targetPos">目標位置</param>
    /// <param name="player">プレイヤー参照</param>
    void CalculateArcPath(const Hagine::Vector3 &startPos, const Hagine::Vector3 &targetPos, Player &player);

    /// <summary>
    /// 進捗度合いから弧上の位置を取得
    /// </summary>
    /// <param name="progress">進捗度合い（0.0～1.0）</param>
    /// <returns>Vector3: 弧上の位置座標</returns>
    Hagine::Vector3 GetArcPosition(float progress);

    /// <summary>
    /// 終了入力をチェック
    /// </summary>
    /// <returns>bool: 終了入力があったかどうか</returns>
    bool CheckExitInput();

    /// <summary>
    /// 目標位置到達をチェック
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    /// <returns>bool: 到達したかどうか</returns>
    bool CheckReachedTarget(Player &player);

    /// <summary>
    /// 突撃を終了
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void FinishRush(Player &player);

    /// <summary>
    /// 移動を更新
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void UpdateMovement(Player &player);

    /// <summary>
    /// 移動方向を計算
    /// </summary>
    /// <param name="progress">進捗度合い</param>
    /// <param name="player">プレイヤー参照</param>
    /// <returns>Vector3: 計算された移動方向</returns>
    Hagine::Vector3 CalculateMovementDirection(float progress, Player &player);

    /// <summary>
    /// 回転を更新
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void UpdateRotation(Player &player);

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    static constexpr float kMidPointFactor = 0.5f;         // 中点計算係数
    static constexpr float kArcHeightFactor = 0.25f;       // 弧の高さ係数
    static constexpr float kSideOffsetFactor = 0.4f;       // 横オフセット係数
    static constexpr float kVelocityZero = 0.0f;           // 速度ゼロ
    static constexpr float kUpDotHighThreshold = 0.3f;     // 上方向内積閾値（高）
    static constexpr float kUpDotLowThreshold = -0.3f;     // 上方向内積閾値（低）
    static constexpr float kUpHeightLowMultiplier = 0.3f;  // 低位置時の高さ倍率
    static constexpr float kUpHeightHighMultiplier = 1.2f; // 高位置時の高さ倍率
    static constexpr float kForwardOffsetFactor = 0.2f;    // 前方オフセット係数
    static constexpr float kTwoFactor = 2.0f;              // ベジェ曲線の2倍係数
    static constexpr float kProgressIncrement = 0.01f;     // 進捗増分
    static constexpr float kMaxProgress = 1.0f;            // 最大進捗
    static constexpr float kUpVectorY = 1.0f;              // 上方向ベクトルY成分
    static constexpr float kVectorZero = 0.0f;             // ベクトル成分ゼロ

    // 行列インデックス定数
    static constexpr int kMatrixRow0 = 0;
    static constexpr int kMatrixRow1 = 1;
    static constexpr int kMatrixRow2 = 2;
    static constexpr int kMatrixCol0 = 0;
    static constexpr int kMatrixCol1 = 1;
    static constexpr int kMatrixCol2 = 2;

    Hagine::Vector3 targetPosition_;  // 目標位置
    Hagine::Vector3 rushDirection_;   // 突撃方向
    Hagine::Vector3 startPosition_;   // 開始位置
    Hagine::Vector3 arcControlPoint_; // 弧制御点

    float distance_ = 3.0f;           // 移動距離
    float rushSpeed_ = 200.0f;        // 突撃速度
    float elapsedTime_ = 0.0f;        // 経過時間
    float rotationSpeed_ = 10.0f;     // 回転速度
    float arcLength_ = 0.0f;          // 弧の長さ
    float arrivalDistance_ = 4.0f;    // 到達判定距離
    float blendStartProgress_ = 0.7f; // ブレンド開始進捗

    bool isRushing_ = false; // 突撃中フラグ

    std::unique_ptr<Shake> shake_;
    std::unique_ptr<Hagine::ParticleEmitter> rushEmitter_; // 突撃パーティクル
};