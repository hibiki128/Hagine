#pragma once
#include "DirectXCommon.h"
#include "Easing.h"
#include "d3d12.h"
#include "numbers"
#include "type/Matrix4x4.h"
#include "type/Quaternion.h"
#include "type/Vector3.h"
#include "wrl.h"

/// <summary>
/// ビュープロジェクション定数バッファデータ
/// </summary>
struct ConstBufferDataViewProjection {
    Matrix4x4 view;       // ビュー行列
    Matrix4x4 projection; // 射影行列
    Vector3 cameraPos;    // カメラ座標
};

/// <summary>
/// ビュープロジェクションクラス
/// カメラのビュー行列と射影行列を管理し、イージング機能を提供する
/// </summary>
class ViewProjection {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    ViewProjection() = default;
    ~ViewProjection() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="jsonFile">設定ファイル名</param>
    void Initialize(std::string jsonFile = "");

    /// <summary>
    /// 定数バッファ生成
    /// </summary>
    void CreateConstBuffer();

    /// <summary>
    /// マッピング
    /// </summary>
    void Map();

    /// <summary>
    /// 行列を更新
    /// </summary>
    void UpdateMatrix();

    /// <summary>
    /// 行列を転送
    /// </summary>
    void TransferMatrix();

    /// <summary>
    /// ビュー行列を更新
    /// </summary>
    void UpdateViewMatrix();

    /// <summary>
    /// 射影行列を更新
    /// </summary>
    void UpdateProjectionMatrix();

    /// <summary>
    /// カメラをイージング移動
    /// </summary>
    /// <param name="easeType">イージングタイプ</param>
    /// <param name="jsonName">目標値のJSON名</param>
    /// <param name="duration">イージング時間</param>
    void EaseCameraMove(EasingType easeType, const std::string &jsonName, float duration = 2.0f);

    /// <summary>
    /// デバッグ情報表示
    /// </summary>
    void ShowDebugInfo();

    /// <summary>
    /// Getter
    /// </summary>
    const Microsoft::WRL::ComPtr<ID3D12Resource> &GetConstBuffer() const { return constBuffer_; }
    bool GetIsCameraMove() { return isEasing_; }

  public:
    /// ===================================================
    /// public varians
    /// ===================================================

    bool isUseQuaternion_ = false;                                                  // 回転モード（trueならクォータニオン、falseならオイラー角）
    Quaternion quateRotation_ = Quaternion::IdentityQuaternion();                   // クォータニオン回転
    Vector3 eulerRotation_ = {0.0f, 0.0f, 0.0f};                                    // オイラー角回転（ラジアン）
    Vector3 translation_ = {0.0f, 0.0f, -10.0f};                                    // カメラ座標
    float fovAngleY = 45.0f * std::numbers::pi_v<float> / 180.0f;                   // 垂直方向視野角
    float aspectRatio = float(WinApp::kClientWidth) / float(WinApp::kClientHeight); // アスペクト比
    float nearZ = 0.1f;                                                             // 深度限界（手前側）
    float farZ = 1000.0f;                                                           // 深度限界（奥側）
    Matrix4x4 matView_{};                                                           // ビュー行列
    Matrix4x4 matProjection_{};                                                     // 射影行列
    Matrix4x4 matWorld_{};                                                          // ワールド行列

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 設定を保存
    /// </summary>
    /// <param name="jsonFile">保存先ファイル名</param>
    void Save(std::string jsonFile);

    /// <summary>
    /// 設定を読み込み
    /// </summary>
    /// <param name="jsonFile">読み込み元ファイル名</param>
    void Load(std::string jsonFile);

    // コピー禁止
    ViewProjection(const ViewProjection &) = delete;
    ViewProjection &operator=(const ViewProjection &) = delete;

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    DirectXCommon *dxCommon_ = nullptr; // DirectX共通クラス

    // イージング関連
    bool isEasing_ = false;                              // イージング中フラグ
    float easingTime_ = 0.0f;                            // イージング経過時間
    float easingDuration_ = 2.0f;                        // イージング時間
    EasingType currentEasingType_ = EasingType::OutQuad; // イージングタイプ

    // 開始時の値
    Vector3 startTranslation_{};           // 開始座標
    Vector3 startEulerRotation_{};         // 開始オイラー角
    Quaternion startQuaternionRotation_{}; // 開始クォータニオン

    // 目標値（JSONから読み込み）
    Vector3 targetTranslation_{};         // 目標座標
    Vector3 targetEulerRotation_{};       // 目標オイラー角
    Quaternion targetQuaternionRotation_{}; // 目標クォータニオン

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_{}; // 定数バッファ
    ConstBufferDataViewProjection *constMap = nullptr;   // マッピング済みアドレス
};

static_assert(!std::is_copy_assignable_v<ViewProjection>);