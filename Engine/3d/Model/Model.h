#pragma once
#include "type/Matrix4x4.h"
#include "ModelCommon.h"
#include "type/Quaternion.h"
#include "Srv/SrvManager.h"
#include "type/Vector2.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include "animation/Animator.h"
#include "animation/Bone.h"
#include "animation/Skin.h"
#include "array"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "map"
#include "span"

#include "Material/Material.h"
#include "Mesh/Mesh.h"
#include "Object/Object3dCommon.h"
#include <Primitive/PrimitiveModel.h>
#include <unordered_set>

class Model {
  private:
    ModelCommon *modelCommon_;

    // Objファイルのデータ
    ModelData modelData;
    SrvManager *srvManager_;

    std::string filename_;
    std::string directorypath_;

    bool isGltf;

    Matrix4x4 localMatrix;

    // マルチメッシュ対応
    std::vector<std::unique_ptr<Mesh>> meshes_;
    // マルチマテリアル対応
    std::vector<std::unique_ptr<Material>> materials_;

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

    void CreatePrimitiveModel(const PrimitiveType &type);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const Vector4 &color = {1.0f, 1.0f, 1.0f, 1.0f}, bool lighting = true);
   
    // Setter methods
    void SetSrv(SrvManager *srvManager) { srvManager_ = srvManager; }
    void SetAnimator(Animator *animator) { animator_ = animator; }
    void SetSkin(Skin *skin) { skin_ = skin; }
    void SetBone(Bone *bone) { bone_ = bone; }

    // マテリアル関連
    void SetMaterialData(const std::vector<MaterialData> &materialData) { modelData.materials = materialData; }
    std::vector<MaterialData> &GetMaterialData() { return modelData.materials; }
    void SetTexture(const std::string &filePath, uint32_t index) {
        materials_[index]->SetTexture(filePath);
    }

    // Getter methods
    ModelData GetModelData() { return modelData; }
    bool IsGltf() { return isGltf; }

    // マルチメッシュ・マルチマテリアル情報取得
    size_t GetMeshCount() const { return meshes_.size(); }
    size_t GetMaterialCount() const { return materials_.size(); }

    Mesh *GetMesh(uint32_t index) {
        return (index < meshes_.size()) ? meshes_[index].get() : nullptr;
    }

    Material *GetMaterial(uint32_t index) {
        return (index < materials_.size()) ? materials_[index].get() : nullptr;
    }

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