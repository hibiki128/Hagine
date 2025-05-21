#pragma once
#include "ViewProjection/ViewProjection.h"
#include "WorldTransform.h"
class Player;
class FollowCamera {
  public:
    /// ===================================================
    /// public method
    /// ===================================================
    void Init();

    void Update();

    void imgui();

    float GetYaw() { return yaw_; }

    void SetPlayer(Player *target) { target_ = target; }
    ViewProjection &GetViewProjection() { return viewProjection_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    void Move();

  private:
    // ビュープロジェクション
    ViewProjection viewProjection_;

    WorldTransform worldTransform_;
    // 追従対象
    Player *target_ = nullptr;


    Vector3 cameraOffset_ = {0.0f, 3.0f, -25.0f};        // ベースのカメラオフセット
    Vector3 shoulderOffsetTarget_ = {0.0f, 0.0f, 0.0f};  // ターゲット肩オフセット
    Vector3 shoulderOffsetCurrent_ = {0.0f, 0.0f, 0.0f}; // 現在の補間値

    float yaw_;
    float distanceFromTarget_;
    float heightOffset_;
    float shoulderMaxOffset_ = 7.5f; // 肩のズレ最大距離（左右）
    float shoulderLerpSpeed_ = 10.0f; // 補間速度（大きいほど速く追従）
};
