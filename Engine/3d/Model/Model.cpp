#include "Model.h"
#include "Engine/Frame/Frame.h"
#include "Object/Object3dCommon.h"
#include "Texture/TextureManager.h"
#include "fstream"
#include "myMath.h"
#include "sstream"

std::unordered_set<std::string> Model::jointNames = {};

void Model::Initialize(ModelCommon *modelCommon) {
    modelCommon_ = modelCommon;
    srvManager_ = SrvManager::GetInstance();
    material_ = std::make_unique<Material>();
    mesh_ = std::make_unique<Mesh>();

}

void Model::CreateModel(const std::string &directorypath, const std::string &filename) {
    // 引数で受け取ってメンバ変数に記録する
    directorypath_ = directorypath;
    filename_ = filename;

    // モデル読み込み
    modelData = LoadModelFile(directorypath_, filename_);
    mesh_->Initialize();
    material_->GetMaterialData() = modelData.material;
    material_->LoadTexture();
    modelData.material.textureIndex = material_->GetMaterialData().textureIndex;
}

void Model::CreatePrimitiveModel(const PrimitiveType &type) {
    mesh_->PrimitiveInitialize(type);
    material_->PrimitiveInitialize(type);
    material_->LoadTexture();
    mesh_->Initialize();
    modelData.material = material_->GetMaterialData();
    modelData.mesh = mesh_->GetMeshData();
}

void Model::Draw(Object3dCommon *objCommon) {
    D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = mesh_->GetVertexBufferView();
    D3D12_INDEX_BUFFER_VIEW indexBufferView = mesh_->GetIndexBufferView();

    uint32_t SrvIndex;
    if (isGltf) {
        influenceBufferView = skin_->GetSkinCluster().influenceBufferView;
        SrvIndex = skin_->GetSrvIndex();
    } else {
        influenceBufferView = {};
        SrvIndex = {};
    }
    D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
        vertexBufferView,
        influenceBufferView
    };
    modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);
    if (isGltf) {
        if (!animator_->HaveAnimation()) {
            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, vbvs); // VBVを設定
        } else {
            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 2, vbvs); // VBVを設定
            srvManager_->SetGraphicsRootDescriptorTable(6, SrvIndex);
        }
    } else {
        modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, vbvs); // VBVを設定
    }
    // 描画！（DrawCall/ドローコール）
    modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(modelData.mesh.indices.size()), 1, 0, 0, 0);
    if (animator_) {
        if (animator_->HaveAnimation()) {
            objCommon->DrawCommonSetting();
        }
    }
}

void Model::SetTextureIndex(const std::string &filePath) {
    TextureManager::GetInstance()->LoadTexture(filePath);
    modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);
}

// void Model::CreateVartexData() {
//     // Sprite用の頂点リソースを作る
//     vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
//     // リソースの先頭のアドレスから使う
//     vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
//     // 使用するリソースのサイズは頂点6つ分のサイズ
//     vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
//     // 1頂点あたりのサイズ
//     vertexBufferView.StrideInBytes = sizeof(VertexData);
//
//     // 頂点データの設定
//     vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
//
//     std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
// }

// void Model::CreateIndexResource() {
//     indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * modelData.indices.size());
//     indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
//     indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * modelData.indices.size());
//     indexBufferView.Format = DXGI_FORMAT_R32_UINT;
//     indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
//     std::memcpy(indexData, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());
// }

ModelData Model::LoadModelFile(const std::string &directoryPath, const std::string &filename) {
    ModelData modelData;

    // 拡張子に応じたisGltfフラグの設定
    isGltf = false;
    if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".gltf") {
        isGltf = true;
    } else if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".obj") {
        isGltf = false;
    } else {
        assert(false && "Unsupported file format"); // サポート外のフォーマットの場合にアサート
    }

    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;
    const aiScene *scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);

    // メッシュが存在しない場合
    if (!scene || !scene->HasMeshes()) {
        // デフォルトのテクスチャを設定
        modelData.material.textureFilePath = "resources/images/debug/white1x1.png";
        return modelData;
    }

    // メッシュの処理
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh *mesh = scene->mMeshes[meshIndex];
        assert(mesh->HasNormals());        // 法線がないMeshは今回は非対応
        assert(mesh->HasTextureCoords(0)); // TexcoordがないMeshは今回は非対応
        modelData.mesh.vertices.resize(mesh->mNumVertices);
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            aiVector3D &position = mesh->mVertices[vertexIndex];
            aiVector3D &normal = mesh->mNormals[vertexIndex];
            aiVector3D &texcoord = mesh->mTextureCoords[0][vertexIndex];
            // 右手系->左手系への変換を忘れずに
            modelData.mesh.vertices[vertexIndex].position = {-position.x, position.y, position.z, 1.0f};
            modelData.mesh.vertices[vertexIndex].normal = {-normal.x, normal.y, normal.z};
            modelData.mesh.vertices[vertexIndex].texcoord = {texcoord.x, texcoord.y};
        }
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            aiFace &face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3);
            for (uint32_t element = 0; element < face.mNumIndices; ++element) {
                uint32_t vertexIndex = face.mIndices[element];
                modelData.mesh.indices.push_back(vertexIndex);
            }
        }

        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            aiBone *bone = mesh->mBones[boneIndex];
            std::string jointName = bone->mName.C_Str();

            // ジョイント名の重複確認
            assert(jointNames.find(jointName) == jointNames.end() && "Duplicate joint name detected!");
            jointNames.insert(jointName);

            JointWeightData &jointWeightData = modelData.skinClusterData[jointName];

            // バインドポーズ行列の逆行列の計算
            aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
            aiVector3D scale, translate;
            aiQuaternion rotate;
            bindPoseMatrixAssimp.Decompose(scale, rotate, translate);

            Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
                {scale.x, scale.y, scale.z},
                {rotate.x, -rotate.y, -rotate.z, rotate.w},
                {-translate.x, translate.y, translate.z});

            jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

            // ウェイト情報の格納
            for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                jointWeightData.vertexWeights.push_back({bone->mWeights[weightIndex].mWeight,
                                                         bone->mWeights[weightIndex].mVertexId});
            }
        }
    }
    // 処理後にクリア
    jointNames.clear();

    // マテリアルの処理
    for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
        aiMaterial *material = scene->mMaterials[materialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
            aiString textureFilePath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
            modelData.material.textureFilePath = textureFilePath.C_Str();
        }
    }
    if (modelData.material.textureFilePath.empty()) {
        // テクスチャがない場合はデフォルトのテクスチャを設定
        modelData.material.textureFilePath = "debug/white1x1.png";
    }
    modelData.rootNode = ReadNode(scene->mRootNode);
    return modelData;
}

Node Model::ReadNode(aiNode *node) {
    Node result;

    aiVector3D scale, translate;
    aiQuaternion rotate;
    node->mTransformation.Decompose(scale, rotate, translate);
    result.transform.scale = {scale.x, scale.y, scale.z};
    result.transform.rotate = {rotate.x, -rotate.y, -rotate.z, rotate.w};
    result.transform.translate = {-translate.x, translate.y, translate.z};

    result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

    result.name = node->mName.C_Str();          // node名を格納
    result.children.resize(node->mNumChildren); // 子供の数だけ確保
    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        // 再帰的に読んで階層構造を作っていく
        result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
    }
    return result;
}