#include "Model.h"
#include "Engine/Frame/Frame.h"
#include "Graphics/Texture/TextureManager.h"
#include "Object/Object3dCommon.h"
#include "fstream"
#include "myMath.h"
#include "sstream"
#include <SkyBox/SkyBox.h>

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

void Model::CreatePrimitiveModel(const PrimitiveType &type, std::string texPath) {
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
    materials_[0]->Initialize();
    materials_[0]->PrimitiveInitialize(type);
    materials_[0]->GetMaterialData().textureFilePath = texPath;
    materials_[0]->LoadTexture();

    // modelDataに反映
    modelData.materials[0] = materials_[0]->GetMaterialData();
    modelData.meshes[0] = meshes_[0]->GetMeshData();

    // メッシュのマテリアルインデックスを設定
    modelData.meshes[0].materialIndex = 0;
}

void Model::Update() {
    if (isGltf && animator_ && animator_->HaveAnimation()) {
        // 1. 入力頂点データ更新
        skin_->UpdateInputVertices(modelData);

        // 2. コンピュートシェーダ実行のためのバリア
        ID3D12GraphicsCommandList *commandList = modelCommon_->GetDxCommon()->GetCommandList().Get();

        // UAV -> SRV バリア (前回の結果があれば)
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = skin_->GetOutputVertexResource();
        commandList->ResourceBarrier(1, &barrier);

        // 3. スキニング実行
        skin_->ExecuteSkinning(commandList);

        // 4. UAV -> VBV バリア
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = skin_->GetOutputVertexResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        commandList->ResourceBarrier(1, &barrier);
    }
}

void Model::Draw(const Vector4 &color, bool lighting, bool reflect) {
    ID3D12GraphicsCommandList *commandList = modelCommon_->GetDxCommon()->GetCommandList().Get();

    INT vertexOffset = 0;

    for (size_t meshIndex = 0; meshIndex < meshes_.size(); ++meshIndex) {
        Mesh *currentMesh = meshes_[meshIndex].get();
        uint32_t materialIndex = modelData.meshes[meshIndex].materialIndex;
        if (materialIndex >= materials_.size()) {
            materialIndex = 0;
        }
        Material *currentMaterial = materials_[materialIndex].get();

        // インデックスバッファ設定
        D3D12_INDEX_BUFFER_VIEW indexBufferView = currentMesh->GetIndexBufferView();
        commandList->IASetIndexBuffer(&indexBufferView);

        // 頂点バッファ設定 - アニメーション有無で使用するバッファを切り替え
        if (isGltf && animator_ && animator_->HaveAnimation()) {
            // スキニング後の頂点バッファのみを使用
            D3D12_VERTEX_BUFFER_VIEW vbv = skin_->GetOutputVertexBufferView();
            commandList->IASetVertexBuffers(0, 1, &vbv);

            // パレット情報をシェーダーに渡す（必要に応じて）
            srvManager_->SetGraphicsRootDescriptorTable(8, skin_->GetPaletteSrvIndex());
            vertexOffset = static_cast<INT>(skin_->GetMeshVertexOffset(meshIndex));
        } else {
            // 元の頂点バッファを使用
            D3D12_VERTEX_BUFFER_VIEW vbv = currentMesh->GetVertexBufferView();
            commandList->IASetVertexBuffers(0, 1, &vbv);
        }

        commandList->SetGraphicsRootDescriptorTable(7, srvManager_->GetGPUDescriptorHandle(SkyBox::GetInstance()->GetTextureIndex()));
        if (reflect) {
            // 環境係数を有効化
            SetEnvironmentCoefficients(1.0f);
        } else {
            SetEnvironmentCoefficients(0.0f); // 環境係数を無効化
        }

        // マテリアル描画
        currentMaterial->Draw(color, lighting);

        // 描画コール
        commandList->DrawIndexedInstanced(
            UINT(modelData.meshes[meshIndex].indices.size()), 1, 0, vertexOffset, 0);
    }
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