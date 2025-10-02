#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Transform/WorldTransform.h"
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
    void SetCameraFov(float fov) { 
        viewProjection_.fovAngleY = fov * std::numbers::pi_v<float> / 180.0f;
    }
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
    float shoulderMaxOffset_ = 12.5f;  // 肩のズレ最大距離（左右）
    float shoulderLerpSpeed_ = 10.0f; // 補間速度（大きいほど速く追従）
    float rushCameraResumeDistance_ = 50.0f; // この距離以下になったらカメラ追従を再開
    bool isResumeFromRush_ = false;          // Rush状態からの復帰中かどうか
    float rushResumeBlendSpeed_ = 8.0f;      // Rush復帰時の補間速度（通常より高速）
    Vector3 rushCameraPosition_;             // Rush中に固定されていたカメラ位置
    Quaternion rushCameraRotation_;          // Rush中に固定されていたカメラ回転
    bool isRushCameraActive_ = false;
    Vector3 rushCameraOffset_ = {0.0f, 8.0f, -20.0f};
    float rushCameraFollowRate_ = 0.3f; // Rush中の追従率（0.0-1.0）
};
