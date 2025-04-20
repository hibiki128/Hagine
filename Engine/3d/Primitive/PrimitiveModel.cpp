#include "PrimitiveModel.h"
#include <DirectXMath.h>
#include <myMath.h>
using namespace DirectX;

void PrimitiveModel::Initialize() {
    CreateSphere();
}

void PrimitiveModel::CreateSphere() {
    // Sphereの頂点データ
    PrimitiveData primitiveData{};
    // 球分割数
    const uint32_t kSubdivision = 32;
    const float kLonEvery = std::numbers::pi_v<float> * 2.0f / static_cast<float>(kSubdivision);
    const float kLatEvery = std::numbers::pi_v<float> / static_cast<float>(kSubdivision);

    // 頂点の生成
    for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
        float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;
        float sinLat = std::sinf(lat);
        float cosLat = std::cosf(lat);

        for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
            float lon = kLonEvery * lonIndex;
            float sinLon = std::sinf(lon);
            float cosLon = std::cosf(lon);

            VertexData vertex{};
            vertex.position = {cosLat * cosLon, sinLat, cosLat * sinLon, 1.0f};
            vertex.normal = {cosLat * cosLon, sinLat, cosLat * sinLon};
            vertex.texcoord = {static_cast<float>(lonIndex) / static_cast<float>(kSubdivision),
                               1.0f - static_cast<float>(latIndex) / static_cast<float>(kSubdivision)};
            primitiveData.vertices.push_back(vertex);
        }
    }

    // インデックスの生成
    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            uint32_t first = latIndex * (kSubdivision + 1) + lonIndex;
            uint32_t second = first + kSubdivision + 1;

            // 三角形1
            primitiveData.indices.push_back(first);
            primitiveData.indices.push_back(second);
            primitiveData.indices.push_back(first + 1);

            // 三角形2
            primitiveData.indices.push_back(second);
            primitiveData.indices.push_back(second + 1);
            primitiveData.indices.push_back(first + 1);
        }
    }

    primitiveData.color = {1.0f, 1.0f, 1.0f, 1.0f};
    primitiveData.uvMatrix = MakeIdentity4x4();

    // コンテナに挿入
    primitiveDataMap_.insert(std::make_pair(PrimitiveType::Sphere, primitiveData));
}

void PrimitiveModel::CreatePlane() {
    // Planeの頂点データ
    PrimitiveData primitiveData{};
    // 頂点の生成
    primitiveData.vertices = {
        {{-1.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{1.0f, 0.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}};
    // インデックスの生成
    primitiveData.indices = {2, 3, 1,
                             2, 1, 3};
    primitiveData.color = {1.5f, 1.f, 1.f};
    primitiveDataMap_.insert(std::make_pair(PrimitiveType::Plane, primitiveData));
}

void PrimitiveModel::CreateCube() {

}

void PrimitiveModel::CreateCylinder() {

}

void PrimitiveModel::CreateRing() {
    const uint32_t kRingDivide = 32;
    const float kOuterRadius = 1.0f;
    const float kInnerRadius = 0.2f;
    const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);
}
