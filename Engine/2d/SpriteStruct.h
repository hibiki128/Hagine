#pragma once
#include <type/Matrix4x4.h>
#include <type/Vector2.h>
#include <type/Vector3.h>
#include <type/Vector4.h>

namespace Hagine::Graphics {

struct SpriteTransform {
    Vector3 scale;
    Vector3 rotate;
    Vector3 translate;
};

// 頂点データ
struct SpriteVertexData {
    Vector4 position;
    Vector2 texcoord;
};

// マテリアルデータ
struct SpriteMaterial {
    Vector4 color;
    Matrix4x4 uvTransform;
    float padding[3];
};

// 座標変換行列データ
struct SpriteTransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
};

} // namespace Hagine