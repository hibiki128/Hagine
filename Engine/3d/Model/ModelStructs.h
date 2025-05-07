#pragma once
#include "array"
#include "wrl.h"
#include <Matrix4x4.h>
#include <Quaternion.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>
#include <d3d12.h>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <list>
#include"WorldTransform.h"

struct QuaternionTransform {
    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;
};

// 頂点データ
struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

struct MaterialData {
    std::string textureFilePath;
    uint32_t textureIndex = 0;
    std::string modelFilePath;
    Matrix4x4 uvTransform;
    Vector4 color;
};

struct Node {
    QuaternionTransform transform;
    Matrix4x4 localMatrix;
    std::string name;
    std::vector<Node> children;
};

struct Joint {
    QuaternionTransform transform;
    Matrix4x4 localMatrix;
    Matrix4x4 skeletonSpaceMatrix;
    std::string name;
    std::vector<int32_t> children;
    int32_t index;
    std::optional<int32_t> parent;
};

struct Skeleton {
    int32_t root;
    std::map<std::string, int32_t> jointMap;
    std::vector<Joint> joints;
};

struct VertexWeightData {
    float weight;
    uint32_t vertexIndex;
};

struct JointWeightData {
    Matrix4x4 inverseBindPoseMatrix;
    std::vector<VertexWeightData> vertexWeights;
};

struct ModelData {
    std::map<std::string, JointWeightData> skinClusterData;
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    MaterialData material;
    Node rootNode;
};

static const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence {
    std::array<float, kNumMaxInfluence> weights;
    std::array<int32_t, kNumMaxInfluence> jointIndices;
};

struct WellForGPU {
    Matrix4x4 skeletonSpaceMatrix;
    Matrix4x4 skeletonSpaceInverseTransposeMatrix;
};

struct SkinCluster {
    std::vector<Matrix4x4> inverseBindPoseMatrices;
    Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
    D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
    std::span<VertexInfluence> mappedInfluence;
    Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
    std::span<WellForGPU> mappedPalette;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};

struct KeyframeVector3 {
    Vector3 value;
    float time;
};

struct KeyframeQuaternion {
    Quaternion value;
    float time;
};

struct NodeAnimation {
    std::vector<KeyframeVector3> translate;
    std::vector<KeyframeQuaternion> rotate;
    std::vector<KeyframeVector3> scale;
};

struct Animation {
    float duration;
    std::map<std::string, NodeAnimation> nodeAnimations;
};

struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
};

struct Particle {
    WorldTransform transform; // 位置
    Vector3 emitterPosition;
    Vector3 velocity;         // 速度
    Vector3 Acce;
    Vector3 startScale;
    Vector3 endScale;
    Vector3 startAcce;
    Vector3 endAcce;
    Vector3 startRote;
    Vector3 endRote;
    Vector3 rotateVelocity;
    Vector3 fixedDirection;
    Vector4 color;     // 色
    float lifeTime;    // ライフタイム
    float currentTime; // 現在の時間
    float initialAlpha;
};

// マテリアルデータ
struct Material {
    Vector4 color;
    Matrix4x4 uvTransform;
    float padding[3];
};

struct ParticleGroupData {
    // マテリアルデータ
    MaterialData material;
    // パーティクルのリスト (std::list<Particle> 型)
    std::list<Particle> particles;
    // インスタンシングデータ用SRVインデックス
    uint32_t instancingSRVIndex = 0;
    // インスタンシングリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = nullptr;
    // インスタンス数
    uint32_t instanceCount = 0;
    // インスタンシングデータを書き込むためのポインタ
    ParticleForGPU *instancingData = nullptr;
    // グループ名
    std::string groupName;
};
