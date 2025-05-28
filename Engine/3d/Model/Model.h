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
    void Draw(Object3dCommon *objCommon,std::vector<Material> materials);

    // Setter methods
    void SetSrv(SrvManager *srvManager) { srvManager_ = srvManager; }
    void SetAnimator(Animator *animator) { animator_ = animator; }
    void SetSkin(Skin *skin) { skin_ = skin; }
    void SetBone(Bone *bone) { bone_ = bone; }

    // マルチマテリアル対応のテクスチャ設定
    void SetTextureIndex(const std::string &filePath, uint32_t materialIndex);
    void SetAllTexturesIndex(const std::string &filePath);

    // マテリアル関連
    void SetMaterialData(const std::vector<MaterialData> &materialData) { modelData.materials = materialData; }
    std::vector<MaterialData> &GetMaterialData() { return modelData.materials; }

    // マテリアル色設定
    void SetMaterialColor(uint32_t materialIndex, const Vector4 &color);
    void SetAllMaterialsColor(const Vector4 &color);

    // マテリアルの光沢度設定
    void SetMaterialShininess(uint32_t materialIndex, float shininess);
    void SetAllMaterialsShininess(float shininess);

    // UV変換設定
    void SetMaterialUVTransform(uint32_t materialIndex, const Matrix4x4 &uvTransform);
    void SetAllMaterialsUVTransform(const Matrix4x4 &uvTransform);

    // メッシュとマテリアルの関連付け
    void SetMeshMaterial(uint32_t meshIndex, uint32_t materialIndex);

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

    /// <summary>
    /// マテリアルインデックス検証
    /// </summary>
    /// <param name="materialIndex"></param>
    /// <returns></returns>
    bool IsValidMaterialIndex(uint32_t materialIndex) const;

    /// <summary>
    /// メッシュインデックス検証
    /// </summary>
    /// <param name="meshIndex"></param>
    /// <returns></returns>
    bool IsValidMeshIndex(uint32_t meshIndex) const;
};