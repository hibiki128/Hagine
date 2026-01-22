#pragma once
#include "DirectXCommon.h"
#include "type/Quaternion.h"
#include "d3d12.h"
#include "myMath.h"
#include "wrl.h"
namespace Hagine::Transform {
// 定数バッファ用データ構造体
struct ConstBufferDataWorldTransform {
    Math::Matrix4x4 matWorld; // ローカル → ワールド変換行列
};

class WorldTransform {
  public:
    // クォータニオン角を使うかどうか（falseの場合はオイラー角）
    bool isUseQuaternion_ = true;

    // ローカルスケール
    Math::Vector3 scale_ = {1.0f, 1.0f, 1.0f};
    // ローカル回転（オイラー角用）
    Math::Vector3 eulerRotation_ = {0.0f, 0.0f, 0.0f};
    // ローカル回転（クォータニオン）
    Math::Quaternion quateRotation_ = Math::Quaternion::IdentityQuaternion();
    // ローカル座標
    Math::Vector3 translation_ = {0.0f, 0.0f, 0.0f};

    // ローカルからワールド変換行列
    Math::Matrix4x4 matWorld_;
    // 親となるワールド変換へのポインタ
    const WorldTransform *parent_ = nullptr;

    WorldTransform();
    ~WorldTransform();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 行列の転送
    /// </summary>
    void TransferMatrix();

    /// <summary>
    /// 行列を計算・転送する
    /// </summary>
    void UpdateMatrix();

    /// <summary>
    /// オイラー角で回転を設定
    /// </summary>
    /// <param name="eulerAngles">オイラー角（ラジアン）</param>
    void SetRotationEuler(const Math::Vector3 &eulerAngles);

    /// <summary>
    /// クォータニオンで回転を設定
    /// </summary>
    /// <param name="quaternion">回転クォータニオン</param>
    void SetRotationQuaternion(const Math::Quaternion &quaternion);

    /// <summary>
    /// 現在の回転をオイラー角で取得
    /// </summary>
    /// <returns>オイラー角（ラジアン）</returns>
    Math::Vector3 GetRotationEuler() const;

    /// <summary>
    /// 現在の回転をクォータニオンで取得
    /// </summary>
    /// <returns>回転クォータニオン</returns>
    Math::Quaternion GetRotationQuaternion() const;

    /// <summary>
    /// ワールド座標での回転をオイラー角で取得
    /// </summary>
    /// <returns>ワールド回転（オイラー角、ラジアン）</returns>
    Math::Vector3 GetWorldRotationEuler() const;

    /// <summary>
    /// ワールド座標での回転をクォータニオンで取得
    /// </summary>
    /// <returns>ワールド回転（クォータニオン）</returns>
    Math::Quaternion GetWorldRotationQuaternion() const;

    // ローカル座標の取得
    Math::Vector3 GetLocalPosition() const { return translation_; }
    Math::Quaternion GetLocalRotation() const { return quateRotation_; }
    Math::Vector3 GetLocalScale() const { return scale_; }

    // ワールド座標の取得（位置、回転、スケール）
    Math::Vector3 GetWorldPosition() const;
    Math::Quaternion GetWorldRotation() const;
    Math::Vector3 GetWorldScale() const;

    /// <summary>
    /// 定数バッファの取得
    /// </summary>
    /// <returns></returns>
    const Microsoft::WRL::ComPtr<ID3D12Resource> &GetConstBuffer() const { return constBuffer_; }

  private:
    /// <summary>
    /// 定数バッファ生成
    /// </summary>
    void CreateConstBuffer();

    /// <summary>
    /// マッピングする
    /// </summary>
    void Map();

    // オイラー角→クォータニオン変換用
    Math::Vector3 preRotate_ = {0.0f, 0.0f, 0.0f};

    void UpdateEuler();
    void UpdateQuaternion();
    void RotateQuaternion();

  private:
    Core::DirectXCommon *dxCommon_ = nullptr;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    // マッピング済み
    ConstBufferDataWorldTransform *constMap = nullptr;
    //// コピー禁止
    // WorldTransform(const WorldTransform&) = delete;
    // WorldTransform& operator=(const WorldTransform&) = delete;
};

} // namespace Hagine::Graphics