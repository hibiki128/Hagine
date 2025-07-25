#include "WorldTransform.h"

WorldTransform::WorldTransform() {
}

WorldTransform::~WorldTransform() {
}

void WorldTransform::Initialize() {
    // スケール、回転、平行移動を初期化
    scale_ = {1.0f, 1.0f, 1.0f};
    eulerRotation_ = {0.0f, 0.0f, 0.0f};
    quateRotation_ = Quaternion::IdentityQuaternion();
    translation_ = {0.0f, 0.0f, 0.0f};
    preRotate_ = {0.0f, 0.0f, 0.0f};

    dxCommon_ = DirectXCommon::GetInstance();
    matWorld_ = MakeIdentity4x4();

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
    // クォータニオンorオイラー
    isUseQuaternion_ ? UpdateQuaternion() : UpdateEuler();

    // 親があれば親のワールド行列を掛ける
    if (parent_) {
        matWorld_ = matWorld_ * parent_->matWorld_;
    }

    // 定数バッファに転送する
    TransferMatrix();
}

void WorldTransform::SetRotationEuler(const Vector3 &eulerAngles) {
    eulerRotation_ = eulerAngles;
    if (isUseQuaternion_) {
        quateRotation_ = Quaternion::FromEulerAngles(eulerAngles);
    }
}

void WorldTransform::SetRotationQuaternion(const Quaternion &quaternion) {
    quateRotation_ = quaternion.Normalize();
    if (!isUseQuaternion_) {
        eulerRotation_ = quateRotation_.ToEulerAngles();
    }
}

Vector3 WorldTransform::GetRotationEuler() const {
    return isUseQuaternion_ ? quateRotation_.ToEulerAngles() : eulerRotation_;
}

Quaternion WorldTransform::GetRotationQuaternion() const {
    return quateRotation_;
}

Vector3 WorldTransform::GetWorldRotationEuler() const {
    Quaternion worldRotation = GetWorldRotationQuaternion();
    return worldRotation.ToEulerAngles();
}

Quaternion WorldTransform::GetWorldRotationQuaternion() const {
    // 親がいない場合はローカル回転をそのまま返す
    if (!parent_) {
        return quateRotation_;
    }

    // 親のワールド回転を取得
    Quaternion parentWorldRotation = parent_->GetWorldRotationQuaternion();

    // 親の回転 * ローカル回転 でワールド回転を計算
    return parentWorldRotation * quateRotation_;
}

void WorldTransform::UpdateEuler() {
    // オイラー角から行列を作成
    matWorld_ = MakeAffineMatrix(scale_, eulerRotation_, translation_);
}

void WorldTransform::UpdateQuaternion() {
    // 回転処理（オイラー角が変更された場合にクォータニオンを更新）
    if (eulerRotation_.x != preRotate_.x || eulerRotation_.y != preRotate_.y || eulerRotation_.z != preRotate_.z) {
        RotateQuaternion();
    }
    // クォータニオンから行列を作成
    matWorld_ = MakeAffineMatrix(scale_, quateRotation_, translation_);
    // 回転量計算用変数に挿入
    preRotate_ = eulerRotation_;
}

void WorldTransform::RotateQuaternion() {
    if (eulerRotation_.x == 0.0f && eulerRotation_.y == 0.0f && eulerRotation_.z == 0.0f) {
        quateRotation_ = Quaternion::IdentityQuaternion();
    } else {
        // オイラー角からクォータニオンに変換
        quateRotation_ = Quaternion::FromEulerAngles(eulerRotation_);
    }
}