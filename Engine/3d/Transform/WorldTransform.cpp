#include "WorldTransform.h"

WorldTransform::WorldTransform() {
}

WorldTransform::~WorldTransform() {
}

void WorldTransform::Initialize() {
    // スケール、回転、平行移動を初期化
    scale_ = {1.0f, 1.0f, 1.0f};                  // デフォルトのスケール
    rotation_ = Quaternion::IdentityQuaternion(); // デフォルトの回転（単位クォータニオン）
    translation_ = {0.0f, 0.0f, 0.0f};            // デフォルトの位置
    dxCommon_ = DirectXCommon::GetInstance();
    // 行列の初期化
    matWorld_ = MakeIdentity4x4(); // 単位行列で初期化

    CreateConstBuffer();
    Map();
    UpdateMatrix();
}

void WorldTransform::TransferMatrix() {
    // 定数バッファに転送
    if (constMap) {
        constMap->matWorld = matWorld_;
    }
}

void WorldTransform::CreateConstBuffer() {
    const UINT bufferSize = sizeof(ConstBufferDataWorldTransform);
    constBuffer_ = dxCommon_->CreateBufferResource(bufferSize);
}

void WorldTransform::Map() {
    // バッファのマッピング
    HRESULT hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&constMap));
    if (FAILED(hr)) {
        // エラーハンドリング
    }
}

void WorldTransform::UpdateMatrix() {
    // スケール、回転、平行移動を合成して行列を計算する
    matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

    // 親があれば親のワールド行列を掛ける
    if (parent_) {
        matWorld_ = matWorld_ * parent_->matWorld_;
    }

    // 定数バッファに転送する
    TransferMatrix();
}

void WorldTransform::SetRotationEuler(const Vector3 &eulerAngles) {
    rotation_ = Quaternion::FromEulerAngles(eulerAngles);
}

void WorldTransform::SetRotationQuaternion(const Quaternion &quaternion) {
    rotation_ = quaternion.Normalize(); // 正規化して設定
}

Vector3 WorldTransform::GetRotationEuler() const {
    return rotation_.ToEulerAngles();
}

Quaternion WorldTransform::GetRotationQuaternion() const {
    return rotation_;
}

Vector3 WorldTransform::GetWorldRotationEuler() const {
    Quaternion worldRotation = GetWorldRotationQuaternion();
    return worldRotation.ToEulerAngles();
}

Quaternion WorldTransform::GetWorldRotationQuaternion() const {
    // 親がいない場合はローカル回転をそのまま返す
    if (!parent_) {
        return rotation_;
    }

    // 親のワールド回転を取得
    Quaternion parentWorldRotation = parent_->GetWorldRotationQuaternion();

    // 親の回転 * ローカル回転 でワールド回転を計算
    return parentWorldRotation * rotation_;
}