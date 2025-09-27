#pragma once
#include "type/Matrix4x4.h"
#include "type/Vector2.h"
#include "type/Vector3.h"
#include "Camera/ViewProjection/ViewProjection.h"
class DebugCamera {
  public:
    // X,Y,Z軸回りのローカル回転角
    Vector3 rotation_ = {0.0f, 0.0f, 0.0f};
    // ローカル座標
    Vector3 translation_ = {0.0f, 0.0f, -50.0f};
    Matrix4x4 matRot_;
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(ViewProjection *viewProjection);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    void imgui();

    bool GetActive() { return isActive_; }

  private:
    void CameraMove(Vector3 &cameraRotate, Vector3 &cameraTranslate, Vector2 &clickPosition);

  private:
    ViewProjection *viewProjection_;
    Vector2 mouse;
    Vector3 eulerRotation_ = {0.0f, 0.0f, 0.0f}; // rotation_の代わり
    // カメラの移動速度
    float mouseSensitivity = 0.003f;
    float moveZspeed = 0.005f;
    bool lockCamera_ = true;
    bool useKey_ = true;
    bool useMouse_ = false;
    bool isActive_ = false;
    bool isUseQuaternion_ = false; // デフォルトはオイラー角
    Quaternion quateRotation_ = Quaternion::IdentityQuaternion();
    Matrix4x4 rotateXYZMatrix;
    Matrix4x4 matRotDelta;
};
