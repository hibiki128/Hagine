#pragma once
#include "../Base/PlayerBaseState.h"
#include "Application/Utility/Shake/Shake.h"
#include <type/Quaternion.h>
#include <type/Vector3.h>

/// <summary>
/// プレイヤーの突撃状態を管理するクラス
/// 弧を描く軌道で敵に向かって移動する
/// </summary>
class PlayerStateRush : public PlayerBaseState {
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
    Quaternion LookRotation(const Vector3 &forward, const Vector3 &up);

    /// <summary>
    /// 弧形の移動経路を計算
    /// </summary>
    /// <param name="startPos">開始位置</param>
    /// <param name="targetPos">目標位置</param>
    /// <param name="player">プレイヤー参照</param>
    void CalculateArcPath(const Vector3 &startPos, const Vector3 &targetPos, Player &player);

    /// <summary>
    /// 進捗度合いから弧上の位置を取得
    /// </summary>
    /// <param name="progress">進捗度合い（0.0～1.0）</param>
    /// <returns>Vector3: 弧上の位置座標</returns>
    Vector3 GetArcPosition(float progress);

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
    Vector3 CalculateMovementDirection(float progress, Player &player);

    /// <summary>
    /// 回転を更新
    /// </summary>
    /// <param name="player">プレイヤー参照</param>
    void UpdateRotation(Player &player);

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    Vector3 targetPosition_;  // 目標位置
    Vector3 rushDirection_;   // 突撃方向
    Vector3 startPosition_;   // 開始位置
    Vector3 arcControlPoint_; // 弧制御点

    float distance_ = 3.0f;           // 移動距離
    float rushSpeed_ = 200.0f;        // 突撃速度
    float elapsedTime_ = 0.0f;        // 経過時間
    float rotationSpeed_ = 10.0f;     // 回転速度
    float arcLength_ = 0.0f;          // 弧の長さ
    float arrivalDistance_ = 4.0f;    // 到達判定距離
    float blendStartProgress_ = 0.7f; // ブレンド開始進捗

    bool isRushing_ = false; // 突撃中フラグ

    std::unique_ptr<Shake> shake_;
};