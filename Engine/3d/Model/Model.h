#pragma once
#include "Matrix4x4.h"
#include "ModelCommon.h"
#include "Quaternion.h"
#include "Srv/SrvManager.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
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

    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<Material> material_;

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
    void Draw(Object3dCommon *objCommon);

    void SetSrv(SrvManager *srvManager) { srvManager_ = srvManager; }
    void SetAnimator(Animator *animator) { animator_ = animator; }
    void SetSkin(Skin *skin) { skin_ = skin; }
    void SetBone(Bone *bone) { bone_ = bone; }
    void SetTextureIndex(const std::string &filePath);
    void SetMaterialData(const MaterialData &materialData) { modelData.material = materialData; }
    MaterialData GetMaterialData() { return modelData.material; }

    ModelData GetModelData() { return modelData; }

    bool IsGltf() { return isGltf; }

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
