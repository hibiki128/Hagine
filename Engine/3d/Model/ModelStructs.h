#pragma once
#include "WorldTransform.h"
#include "array"
#include "wrl.h"
#include <type/Matrix4x4.h>
#include <type/Quaternion.h>
#include <type/Vector2.h>
#include <type/Vector3.h>
#include <type/Vector4.h>
#include <d3d12.h>
#include <list>
#include <map>
#include <optional>
#include <span>
#include <string>
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
    Vector4 color;
    int32_t enableLighting;
    float padding[3];
    Matrix4x4 uvTransform;
    float shininess;
    std::string textureFilePath;
    uint32_t textureIndex = 0;
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
    Vector3 velocity; // 速度
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
    //std::weak_ptr<Particle> parent;                  // 親パーティクルへの弱参照
    //std::vector<std::shared_ptr<Particle>> children; // 子パーティクルのリスト
    Vector3 relativePosition;                        // 親からの相対位置
    Vector3 parentOffset;                            // 親に対するオフセット
    bool isChild;                                    // 子パーティクルかどうか
    bool createTrail;                                // 軌跡を作成するか
    float trailSpawnTimer;                           // 軌跡生成のタイマー
    float trailSpawnInterval;                        // 軌跡生成間隔
    int maxChildren;                                 // 最大子供数
    float childLifeScale;                            // 子の寿命スケール（親より短く）

    Particle() : isChild(false), createTrail(false), trailSpawnTimer(0.0f),
                 trailSpawnInterval(0.1f), maxChildren(10), childLifeScale(0.8f) {}
};

struct ParticleGroupData {
    // マテリアルデータ
    std::vector<MaterialData> materials;
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
