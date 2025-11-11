#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "type/Matrix4x4.h"
#include "type/Vector2.h"
#include "type/Vector3.h"

/// <summary>
/// デバッグカメラクラス
/// 開発時のカメラ操作とデバッグ用の機能を提供する
/// </summary>
class DebugCamera {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void Initialize(ViewProjection *viewProjection);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// ImGui表示
    /// </summary>
    void imgui();

    /// <summary>
    /// Getter
    /// </summary>
    bool GetActive() { return isActive_; }

  public:
    /// ===================================================
    /// public varians
    /// ===================================================

    Vector3 rotation_ = {0.0f, 0.0f, 0.0f};      // X,Y,Z軸回りのローカル回転角
    Vector3 translation_ = {0.0f, 0.0f, -50.0f}; // ローカル座標
    Matrix4x4 matRot_;                           // 回転行列

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// カメラ移動処理
    /// </summary>
    /// <param name="cameraRotate">カメラ回転</param>
    /// <param name="cameraTranslate">カメラ座標</param>
    /// <param name="clickPosition">クリック位置</param>
    void CameraMove(Vector3 &cameraRotate, Vector3 &cameraTranslate, Vector2 &clickPosition);

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    ViewProjection *viewProjection_{};                            // ビュープロジェクション
    Vector2 mouse{};                                              // マウス座標
    Vector3 eulerRotation_ = {0.0f, 0.0f, 0.0f};                  // オイラー角回転
    Quaternion quateRotation_ = Quaternion::IdentityQuaternion(); // クォータニオン回転
    Matrix4x4 rotateXYZMatrix{};                                  // XYZ回転行列
    Matrix4x4 matRotDelta{};                                      // 回転差分行列
    float mouseSensitivity = 0.003f;                              // マウス感度
    float moveZspeed = 0.005f;                                    // Z軸移動速度
    bool lockCamera_ = true;                                      // カメラロックフラグ
    bool useKey_ = true;                                          // キー操作有効フラグ
    bool useMouse_ = false;                                       // マウス操作有効フラグ
    bool isActive_ = false;                                       // アクティブフラグ
    bool isUseQuaternion_ = false;                                // クォータニオン使用フラグ
};