#pragma once
#include "../Base/PlayerBaseState.h"
#include <type/Quaternion.h>
#include <type/Vector3.h>

class PlayerStateRush : public PlayerBaseState {
  public:
    PlayerStateRush() = default;
    void Enter(Player &player) override;
    void Update(Player &player) override;
    void Exit(Player &player) override;

  private:
    Quaternion LookRotation(const Vector3 &forward, const Vector3 &up);

    Vector3 targetPosition_;
    Vector3 rushDirection_;
    Vector3 startPosition_;
    Vector3 arcControlPoint_; // ベジエ曲線の制御点
    float distance_ = 3.0f;
    float rushSpeed_ = 200.0f; // 急接近時の速度
    float elapsedTime_ = 0.0f;
    float rotationSpeed_ = 10.0f; // 回転速度
    float arcLength_ = 0.0f;
    float arrivalDistance_ = 4.0f;    // 到達判定距離
    float blendStartProgress_ = 0.7f; // 直線ブレンド開始進行度
    bool isRushing_ = false;

    void CalculateArcPath(const Vector3 &startPos, const Vector3 &targetPos, Player &player);
    Vector3 GetArcPosition(float progress);

    bool CheckExitInput();
    bool CheckReachedTarget(Player &player);
    void FinishRush(Player &player);
    void UpdateMovement(Player &player);
    Vector3 CalculateMovementDirection(float progress, Player &player);
    void UpdateRotation(Player &player);
};