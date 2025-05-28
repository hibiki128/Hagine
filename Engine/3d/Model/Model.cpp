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
}

void Model::CreateModel(const std::string &directorypath, const std::string &filename) {
    // 引数で受け取ってメンバ変数に記録する
    directorypath_ = directorypath;
    filename_ = filename;

    // モデル読み込み
    modelData = LoadModelFile(directorypath_, filename_);

    // メッシュ配列のサイズを調整
    meshes_.resize(modelData.meshes.size());

    // 各メッシュの初期化
    for (size_t i = 0; i < modelData.meshes.size(); ++i) {
        meshes_[i] = std::make_unique<Mesh>();
        meshes_[i]->GetMeshData() = modelData.meshes[i];
        meshes_[i]->Initialize();
    }

    // マテリアル配列のサイズを調整
    materials_.resize(modelData.materials.size());

    // 各マテリアルの初期化
    for (size_t i = 0; i < modelData.materials.size(); ++i) {
        materials_[i] = std::make_unique<Material>();
        materials_[i]->Initialize();
        materials_[i]->GetMaterialData() = modelData.materials[i];
        materials_[i]->LoadTexture();

        // テクスチャインデックスを元のmodelDataにも反映
        modelData.materials[i].textureIndex = materials_[i]->GetMaterialData().textureIndex;
    }
}

void Model::CreatePrimitiveModel(const PrimitiveType &type) {
    // プリミティブモデルは通常単一メッシュ・単一マテリアルなので、
    // 配列サイズを1に設定
    meshes_.resize(1);
    materials_.resize(1);
    modelData.meshes.resize(1);
    modelData.materials.resize(1);

    // メッシュの初期化
    meshes_[0] = std::make_unique<Mesh>();
    meshes_[0]->PrimitiveInitialize(type);
    meshes_[0]->Initialize();

    // マテリアルの初期化
    materials_[0] = std::make_unique<Material>();
    materials_[0]->PrimitiveInitialize(type);
    materials_[0]->LoadTexture();

    // modelDataに反映
    modelData.materials[0] = materials_[0]->GetMaterialData();
    modelData.meshes[0] = meshes_[0]->GetMeshData();

    // メッシュのマテリアルインデックスを設定
    modelData.meshes[0].materialIndex = 0;
}

void Model::Draw(Object3dCommon *objCommon, std::vector<Material> materials) {
    // 各メッシュを描画
    for (size_t meshIndex = 0; meshIndex < meshes_.size(); ++meshIndex) {
        // 現在のメッシュとそれに対応するマテリアルを取得
        Mesh *currentMesh = meshes_[meshIndex].get();
        uint32_t materialIndex = modelData.meshes[meshIndex].materialIndex;

        // マテリアルインデックスが有効範囲内かチェック
        if (materialIndex >= materials_.size()) {
            materialIndex = 0; // デフォルトマテリアルを使用
        }
       
        Material *currentMaterial = &materials[materialIndex];

        // バッファビューの取得
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = currentMesh->GetVertexBufferView();
        D3D12_INDEX_BUFFER_VIEW indexBufferView = currentMesh->GetIndexBufferView();
        D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
        uint32_t SrvIndex;

        // スキニング情報の設定
        if (isGltf) {
            influenceBufferView = skin_->GetSkinCluster().influenceBufferView;
            SrvIndex = skin_->GetSrvIndex();
        } else {
            influenceBufferView = {};
            SrvIndex = {};
        }

        // バーテックスバッファビューの配列
        D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
            vertexBufferView,
            influenceBufferView};

        // インデックスバッファの設定
        modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);

        // バーテックスバッファの設定
        if (isGltf) {
            if (!animator_->HaveAnimation()) {
                modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, vbvs);
            } else {
                modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 2, vbvs);
                srvManager_->SetGraphicsRootDescriptorTable(7, SrvIndex);
            }
        } else {
            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, vbvs);
        }

        // マテリアルの設定（描画前に現在のマテリアルを設定）
        Vector4 materialColor = modelData.materials[materialIndex].color;
        bool enableLighting = true; // 必要に応じて調整
        currentMaterial->Draw(materialColor, enableLighting);

        // 描画！（DrawCall/ドローコール） - 現在のメッシュのインデックス数を使用
        modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(
            UINT(modelData.meshes[meshIndex].indices.size()), 1, 0, 0, 0);
    }

    // アニメーション関連の処理
    if (animator_) {
        if (animator_->HaveAnimation()) {
            objCommon->DrawCommonSetting();
        }
    }
}

void Model::SetTextureIndex(const std::string &filePath, uint32_t materialIndex) {
    // 指定されたマテリアルインデックスが有効範囲内かチェック
    if (!IsValidMaterialIndex(materialIndex)) {
        return; // 無効なインデックスの場合は何もしない
    }

    // テクスチャを読み込み
    TextureManager::GetInstance()->LoadTexture(filePath);
    uint32_t textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);

    // 指定されたマテリアルのテクスチャインデックスを更新
    modelData.materials[materialIndex].textureIndex = textureIndex;
    materials_[materialIndex]->GetMaterialData().textureIndex = textureIndex;
    materials_[materialIndex]->GetMaterialData().textureFilePath = filePath;
}

void Model::SetAllTexturesIndex(const std::string &filePath) {
    for (size_t i = 0; i < materials_.size(); ++i) {
        SetTextureIndex(filePath, static_cast<uint32_t>(i));
    }
}

void Model::SetMaterialColor(uint32_t materialIndex, const Vector4 &color) {
    if (!IsValidMaterialIndex(materialIndex)) {
        return;
    }

    modelData.materials[materialIndex].color = color;
    materials_[materialIndex]->GetMaterialData().color = color;
}

void Model::SetAllMaterialsColor(const Vector4 &color) {
    for (size_t i = 0; i < materials_.size(); ++i) {
        SetMaterialColor(static_cast<uint32_t>(i), color);
    }
}

void Model::SetMaterialShininess(uint32_t materialIndex, float shininess) {
    if (!IsValidMaterialIndex(materialIndex)) {
        return;
    }

    modelData.materials[materialIndex].shininess = shininess;
    materials_[materialIndex]->GetMaterialData().shininess = shininess;
}

void Model::SetAllMaterialsShininess(float shininess) {
    for (size_t i = 0; i < materials_.size(); ++i) {
        SetMaterialShininess(static_cast<uint32_t>(i), shininess);
    }
}

void Model::SetMaterialUVTransform(uint32_t materialIndex, const Matrix4x4 &uvTransform) {
    if (!IsValidMaterialIndex(materialIndex)) {
        return;
    }

    modelData.materials[materialIndex].uvTransform = uvTransform;
    materials_[materialIndex]->GetMaterialData().uvTransform = uvTransform;
}

void Model::SetAllMaterialsUVTransform(const Matrix4x4 &uvTransform) {
    for (size_t i = 0; i < materials_.size(); ++i) {
        SetMaterialUVTransform(static_cast<uint32_t>(i), uvTransform);
    }
}

void Model::SetMeshMaterial(uint32_t meshIndex, uint32_t materialIndex) {
    if (!IsValidMeshIndex(meshIndex) || !IsValidMaterialIndex(materialIndex)) {
        return;
    }

    modelData.meshes[meshIndex].materialIndex = materialIndex;
}

bool Model::IsValidMaterialIndex(uint32_t materialIndex) const {
    return materialIndex < materials_.size();
}

bool Model::IsValidMeshIndex(uint32_t meshIndex) const {
    return meshIndex < meshes_.size();
}

ModelData Model::LoadModelFile(const std::string &directoryPath, const std::string &filename) {
    ModelData modelData;

    // 拡張子に応じたisGltfフラグの設定
    isGltf = false;
    if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".gltf") {
        isGltf = true;
    } else if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".obj") {
        isGltf = false;
    } else {
        assert(false && "Unsupported file format");
    }

    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;
    const aiScene *scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);

    // メッシュが存在しない場合
    if (!scene || !scene->HasMeshes()) {
        // デフォルトのメッシュとマテリアルを作成
        MeshData defaultMesh;
        MaterialData defaultMaterial;
        defaultMaterial.textureFilePath = "resources/images/debug/white1x1.png";

        modelData.meshes.push_back(defaultMesh);
        modelData.materials.push_back(defaultMaterial);
        return modelData;
    }

    // メッシュ配列のサイズを事前に確保
    modelData.meshes.resize(scene->mNumMeshes);

    // メッシュの処理
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh *mesh = scene->mMeshes[meshIndex];
        assert(mesh->HasNormals()); // 法線がないMeshは今回は非対応（これは残す）

        MeshData &currentMesh = modelData.meshes[meshIndex];
        currentMesh.vertices.resize(mesh->mNumVertices);

        bool hasTexcoord = mesh->HasTextureCoords(0); // Texcoordの有無を確認

        // 頂点データの処理
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            aiVector3D &position = mesh->mVertices[vertexIndex];
            aiVector3D &normal = mesh->mNormals[vertexIndex];

            // 右手系→左手系変換
            currentMesh.vertices[vertexIndex].position = {-position.x, position.y, position.z, 1.0f};
            currentMesh.vertices[vertexIndex].normal = {-normal.x, normal.y, normal.z};

            if (hasTexcoord) {
                aiVector3D &texcoord = mesh->mTextureCoords[0][vertexIndex];
                currentMesh.vertices[vertexIndex].texcoord = {texcoord.x, texcoord.y};
            } else {
                // Texcoord が無い場合は (0.0, 0.0) を代入
                currentMesh.vertices[vertexIndex].texcoord = {0.0f, 0.0f};
            }
        }

        // インデックスの処理
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            aiFace &face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3); // トライアングルのみ対応
            for (uint32_t element = 0; element < face.mNumIndices; ++element) {
                uint32_t vertexIndex = face.mIndices[element];
                currentMesh.indices.push_back(vertexIndex);
            }
        }

        // スキニング情報の処理（各メッシュごとに）
        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            aiBone *bone = mesh->mBones[boneIndex];
            std::string jointName = bone->mName.C_Str();

            // ジョイント名の重複確認（グローバルで管理）
            if (jointNames.find(jointName) == jointNames.end()) {
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
            }

            // ウェイト情報の格納
            JointWeightData &jointWeightData = modelData.skinClusterData[jointName];
            for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                uint32_t globalVertexIndex = bone->mWeights[weightIndex].mVertexId;

                jointWeightData.vertexWeights.push_back({bone->mWeights[weightIndex].mWeight,
                                                         globalVertexIndex,
                                                         meshIndex});
            }
        }

        // メッシュに関連するマテリアルインデックスを保存
        currentMesh.materialIndex = mesh->mMaterialIndex;
    }

    // jointNamesクリア
    jointNames.clear();

    // マテリアル配列のサイズを事前に確保
    modelData.materials.resize(scene->mNumMaterials);

    // マテリアルの処理（すべてのマテリアルを処理）
    for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
        aiMaterial *material = scene->mMaterials[materialIndex];
        MaterialData &currentMaterial = modelData.materials[materialIndex];

        // ディフューズテクスチャの取得
        if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
            aiString textureFilePath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
            currentMaterial.textureFilePath = textureFilePath.C_Str();
        } else {
            // テクスチャがない場合はデフォルトのテクスチャを設定
            currentMaterial.textureFilePath = "debug/white1x1.png";
        }

        // その他のマテリアルプロパティの取得
        aiColor3D color(1.0f, 1.0f, 1.0f);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        currentMaterial.color = {color.r, color.g, color.b, 1.0f};

        // スペキュラー色の取得
        aiColor3D specular(1.0f, 1.0f, 1.0f);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specular);

        // 光沢度の取得
        float shininess = 32.0f;
        material->Get(AI_MATKEY_SHININESS, shininess);
        currentMaterial.shininess = shininess;

        // UV変換行列の初期化
        currentMaterial.uvTransform = MakeIdentity4x4();
    }

    // マテリアルが存在しない場合のフォールバック
    if (modelData.materials.empty()) {
        MaterialData defaultMaterial;
        defaultMaterial.textureFilePath = "debug/white1x1.png";
        defaultMaterial.color = {1.0f, 1.0f, 1.0f, 1.0f};
        defaultMaterial.shininess = 32.0f;
        defaultMaterial.uvTransform = MakeIdentity4x4();
        modelData.materials.push_back(defaultMaterial);
    }

    // ここでgltfのときだけjointIndicesを +1 する調整
    if (isGltf) {
        // skinClusterDataをコピーし、dummy jointを先頭に追加する例
        std::map<std::string, JointWeightData> newSkinClusterData;

        // ダミーのJointWeightDataは空で良い
        newSkinClusterData["dummy_joint"] = JointWeightData{};

        for (const auto &[jointName, jointWeightData] : modelData.skinClusterData) {
            newSkinClusterData[jointName] = jointWeightData;
        }

        // skinClusterDataを置き換える
        modelData.skinClusterData = std::move(newSkinClusterData);
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