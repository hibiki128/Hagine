#include "Liner.h"
#include "Vector3.h"
#include"Quaternion.h"

Liner::Liner()
{
    obj3d_ = std::make_unique<Object3d>();
    obj3d_->CreateModel("line.obj");
    worldTransform_.Initialize();
}

void Liner::Update(const Vector3& startPoint, const Vector3& endPoint) {
    // 始点から終点へのベクトルを計算
    Vector3 direction = endPoint - startPoint;

    // 距離（線の長さ）を計算
    float length = direction.Length();

    // 方向ベクトルを正規化
    direction.Normalize();

    worldTransform_.translation_ = Lerp(startPoint, endPoint, 0.5f);

    // スケールを線の長さに合わせる（YやZのスケールは固定）
    worldTransform_.scale_ = Vector3(length, 0.01f, 0.01f); // X軸方向にのみ長さを反映

    // 回転をクォータニオンを使用して計算
    Quaternion rotation;

    // デフォルトの前方向（X軸）と現在の方向ベクトルの間の回転を計算
    Vector3 defaultDirection = Vector3(1.0f, 0.0f, 0.0f); // X軸が基準
    rotation.SetFromTo(defaultDirection, direction); // 方向ベクトルに合わせた回転を計算

    // クォータニオンをオイラー角に変換してワールド変換に適用
    worldTransform_.rotation_ = rotation.ToEulerAngles();

    // ワールド行列を更新
    worldTransform_.UpdateMatrix();
}

void Liner::Draw(const Vector3& startPoint, const Vector3& endPoint, const ViewProjection& viewProjection)
{
    Update(startPoint, endPoint);

    // Object3Dの描画を呼び出す
    obj3d_->Draw(worldTransform_, viewProjection);
}