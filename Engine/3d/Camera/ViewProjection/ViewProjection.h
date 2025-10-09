#pragma once
#include "DirectXCommon.h"
#include "Easing.h"
#include "d3d12.h"
#include "numbers"
#include "type/Matrix4x4.h"
#include "type/Quaternion.h"
#include "type/Vector3.h"
#include "wrl.h"

struct ConstBufferDataViewProjection {
    Matrix4x4 view;
    Matrix4x4 projection;
    Vector3 cameraPos;
};

class ViewProjection {
  public:
    // 回転モード切り替えフラグ（trueならクォータニオン、falseならオイラー角）
    bool isUseQuaternion_ = false;

    // クォータニオン回転
    Quaternion quateRotation_ = Quaternion::IdentityQuaternion();

    // オイラー角回転（ラジアン）
    Vector3 eulerRotation_ = {0.0f, 0.0f, 0.0f};

    Vector3 translation_ = {0.0f, 0.0f, -10.0f};

    // 垂直方向視野角
    float fovAngleY = 45.0f * std::numbers::pi_v<float> / 180.0f;
    // ビューポートのアスペクト比
    float aspectRatio = float(WinApp::kClientWidth) / float(WinApp::kClientHeight);
    // 深度限界(手前側)
    float nearZ = 0.1f;
    // 深度限界(奥側)
    float farZ = 1000.0f;

    // ビュー行列
    Matrix4x4 matView_;
    // 射影行列
    Matrix4x4 matProjection_;

    Matrix4x4 matWorld_;

    ViewProjection() = default;
    ~ViewProjection() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(std::string jsonFile = "");

    /// <summary>
    /// 定数バッファ生成
    /// </summary>
    void CreateConstBuffer();

    /// <summary>
    /// マッピングする
    /// </summary>
    void Map();

    /// <summary>
    /// 行列を更新する
    /// </summary>
    void UpdateMatrix();

    /// <summary>
    /// 行列を転送する
    /// </summary>
    void TransferMatrix();

    /// <summary>
    /// ビュー行列を更新する
    /// </summary>
    void UpdateViewMatrix();

    /// <summary>
    /// 射影行列を更新する
    /// </summary>
    void UpdateProjectionMatrix();

    void EaseCameraMove(EasingType easeType, const std::string &jsonName, float duration = 2.0f);

    /// <summary>
    /// 定数バッファの取得
    /// </summary>
    /// <returns>定数バッファ</returns>
    const Microsoft::WRL::ComPtr<ID3D12Resource> &GetConstBuffer() const { return constBuffer_; }

    void ShowDebugInfo();

    bool GetIsCameraMove() { return isEasing_; }

  private:
    DirectXCommon *dxCommon_ = nullptr;
    // イージング関連
    bool isEasing_ = false;
    float easingTime_ = 0.0f;
    float easingDuration_ = 2.0f; // デフォルト2秒
    EasingType currentEasingType_ = EasingType::OutQuad;

    // 開始時の値
    Vector3 startTranslation_;
    Vector3 startEulerRotation_;
    Quaternion startQuaternionRotation_;

    // 目標値（JSONから読み込み）
    Vector3 targetTranslation_;
    Vector3 targetEulerRotation_;
    Quaternion targetQuaternionRotation_;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    // マッピング済みアドレス
    ConstBufferDataViewProjection *constMap = nullptr;
    // コピー禁止
    ViewProjection(const ViewProjection &) = delete;
    ViewProjection &operator=(const ViewProjection &) = delete;

    void Save(std::string jsonFile);
    void Load(std::string jsonFile);
};

static_assert(!std::is_copy_assignable_v<ViewProjection>);
