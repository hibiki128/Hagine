#pragma once
#include "DirectXCommon.h"
#include "type/Quaternion.h"
#include "d3d12.h"
#include "myMath.h"
#include "wrl.h"

// 定数バッファ用データ構造体
struct ConstBufferDataWorldTransform {
    Matrix4x4 matWorld; // ローカル → ワールド変換行列
};

class WorldTransform {
  public:
    // ローカルスケール
    Vector3 scale_ = {1.0f, 1.0f, 1.0f};
    // ローカル回転（クォータニオン）
    Quaternion rotation_ = Quaternion::IdentityQuaternion();
    // ローカル座標
    Vector3 translation_ = {0.0f, 0.0f, 0.0f};
    // ローカルからワールド変換行列
    Matrix4x4 matWorld_;
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
    /// 定数バッファ生成
    /// </summary>
    void CreateConstBuffer();

    /// <summary>
    /// マッピングする
    /// </summary>
    void Map();

    /// <summary>
    /// 行列を計算・転送する
    /// </summary>
    void UpdateMatrix();

    /// <summary>
    /// オイラー角で回転を設定
    /// </summary>
    /// <param name="eulerAngles">オイラー角（ラジアン）</param>
    void SetRotationEuler(const Vector3 &eulerAngles);

    /// <summary>
    /// クォータニオンで回転を設定
    /// </summary>
    /// <param name="quaternion">回転クォータニオン</param>
    void SetRotationQuaternion(const Quaternion &quaternion);

    /// <summary>
    /// 現在の回転をオイラー角で取得
    /// </summary>
    /// <returns>オイラー角（ラジアン）</returns>
    Vector3 GetRotationEuler() const;

    /// <summary>
    /// 現在の回転をクォータニオンで取得
    /// </summary>
    /// <returns>回転クォータニオン</returns>
    Quaternion GetRotationQuaternion() const;

    /// <summary>
    /// ワールド座標での回転をオイラー角で取得
    /// </summary>
    /// <returns>ワールド回転（オイラー角、ラジアン）</returns>
    Vector3 GetWorldRotationEuler() const;

    /// <summary>
    /// ワールド座標での回転をクォータニオンで取得
    /// </summary>
    /// <returns>ワールド回転（クォータニオン）</returns>
    Quaternion GetWorldRotationQuaternion() const;

    /// <summary>
    /// 定数バッファの取得
    /// </summary>
    /// <returns></returns>
    const Microsoft::WRL::ComPtr<ID3D12Resource> &GetConstBuffer() const { return constBuffer_; }

  private:
    DirectXCommon *dxCommon_ = nullptr;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
    // マッピング済み
    ConstBufferDataWorldTransform *constMap = nullptr;
    //// コピー禁止
    // WorldTransform(const WorldTransform&) = delete;
    // WorldTransform& operator=(const WorldTransform&) = delete;
};
// static_assert(!std::is_copy_assignable_v<WorldTransform>);