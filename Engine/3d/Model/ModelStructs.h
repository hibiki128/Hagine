#pragma once
#include "Graphics/PipeLine/PipeLineManager.h"
#include "Transform/WorldTransform.h"
#include "array"
#include "wrl.h"
#include <Transform/WorldTransform.h>
#include <d3d12.h>
#include <list>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <type/Matrix4x4.h>
#include <type/Quaternion.h>
#include <type/Vector2.h>
#include <type/Vector3.h>
#include <type/Vector4.h>
#include <vector>

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
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    bool enableLighting = true;
    Matrix4x4 uvTransform = MakeIdentity4x4();
    float shininess = 20.0f;
    std::string textureFilePath;
    uint32_t textureIndex = 0;
    float environmentCoefficient = 1.0f;
    Vector2 uvPosition{};
    Vector2 uvSize = {1.0f, 1.0f};
    float uvRotate = 0.0f;
};

struct MaterialDataGPU {
    Vector4 color;
    int32_t enableLighting;
    float padding[3]; // この部分が問題
    Matrix4x4 uvTransform;
    float shininess;
    float environmentCoefficient;
    float padding2[2]; // 追加のパディングが必要
};

struct MeshData {
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    uint32_t materialIndex = 0;
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
    uint32_t meshIndex;
};

struct JointWeightData {
    Matrix4x4 inverseBindPoseMatrix;
    std::vector<VertexWeightData> vertexWeights;
};

struct ModelData {
    std::vector<MeshData> meshes;
    std::vector<MaterialData> materials;
    std::map<std::string, JointWeightData> skinClusterData;
    Node rootNode;
    bool hasBones;
    bool hasAnimations;
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

struct SkinningInformationForGPU {
    uint32_t numVertices;
};

struct SkinCluster {
    std::vector<Matrix4x4> inverseBindPoseMatrices;
    //
    Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
    std::span<VertexInfluence> mappedInfluence;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> influenceSrvHandle;
    //
    Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
    std::span<WellForGPU> mappedPalette;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
    //
    Microsoft::WRL::ComPtr<ID3D12Resource> inputVertexResource;
    std::span<VertexData> mappedVertex;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> inputVertexSrvHandle;
    //
    Microsoft::WRL::ComPtr<ID3D12Resource> outputVertexResource;
    D3D12_VERTEX_BUFFER_VIEW outputVertexBufferView;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> outputVertexSrvHandle;
    //
    Microsoft::WRL::ComPtr<ID3D12Resource> skinningInformationResource;
    SkinningInformationForGPU *SkinningInfomationData = nullptr;
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
