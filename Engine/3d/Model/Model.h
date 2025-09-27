#pragma once
#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "ModelCommon.h"
#include "Object/Object3dCommon.h"
#include "animation/Animator.h"
#include "animation/Bone.h"
#include "animation/Skin.h"
#include "array"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "map"
#include "span"
#include "type/Matrix4x4.h"
#include "type/Quaternion.h"
#include "type/Vector2.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include <Graphics/Srv/SrvManager.h>
#include <Primitive/PrimitiveModel.h>
#include <unordered_set>
#include <Transform/ObjColor.h>

class Model {
  private:
    ModelCommon *modelCommon_;

    // Objファイルのデータ
    ModelData modelData_;
    SrvManager *srvManager_;

    std::string filename_;
    std::string directorypath_;

    bool isGltf;

    Matrix4x4 localMatrix;

    // マルチメッシュ対応
    std::vector<std::unique_ptr<Mesh>> meshes_;

    Animator *animator_;
    Skin *skin_;
    Bone *bone_;
    static std::unordered_set<std::string> jointNames;

  public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="modelCommon"></param>
    void Initialize(ModelCommon *modelCommon);

    void CreateModel(const std::string &directorypath, const std::string &filename);

    void CreatePrimitiveModel(const PrimitiveType &type, std::string texPath);

    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const std::vector<std::unique_ptr<Material>> &materials, std::vector<ObjColor> &color, bool lighting, bool reflect);

    // Setter methods
    void SetSrv(SrvManager *srvManager) { srvManager_ = srvManager; }
    void SetAnimator(Animator *animator) {
        animator_ = animator;
        animator_->SetModelData(modelData_);
    }
    void SetSkin(Skin *skin) { skin_ = skin; }
    void SetBone(Bone *bone) { bone_ = bone; }

    // Getter methods
    ModelData GetModelData() { return modelData_; }
    bool IsGltf() { return isGltf; }

    // マルチメッシュ・マルチマテリアル情報取得
    size_t GetMeshCount() const { return meshes_.size(); }

    Mesh *GetMesh(uint32_t index) {
        return (index < meshes_.size()) ? meshes_[index].get() : nullptr;
    }

    Animator *GetAnimator() { return animator_; }

  private:
    /// <summary>
    ///  .objファイルの読み取り
    /// </summary>
    /// <param name="directoryPath"></param>
    /// <param name="filename"></param>
    /// <returns></returns>
    ModelData LoadModelFile(const std::string &directoryPath, const std::string &filename);

    /// <summary>
    /// ノード読み取り
    /// </summary>
    /// <param name="node"></param>
    /// <returns></returns>
    static Node ReadNode(aiNode *node);
};