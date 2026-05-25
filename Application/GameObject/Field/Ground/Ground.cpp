#include "Ground.h"

void Ground::Init(const std::string className) {
    BaseObject::Init(className);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Plane);
    BaseObject::SetTexture("debug/ground.png");

    // 地面の初期トランスフォーム設定
    transform_->translation_.y = -1.0f; // 少し下に配置
    transform_->scale_ = {1000.0f, 1000.0f, 1000.0f}; // 広大なサイズに設定

    // 平面モデルを地面として使うためにX軸を回転
    transform_->quateRotation_ = Quaternion::FromEulerAngles(Vector3(degreesToRadians(-90.0f), 0.0f, 0.0f));
}

void Ground::Update() {
    // 基底クラスの更新処理
    BaseObject::Update();
}

void Ground::Draw(const ViewProjection &viewProjection) {
    // 基底クラスの描画処理
    BaseObject::Draw(viewProjection);
}
