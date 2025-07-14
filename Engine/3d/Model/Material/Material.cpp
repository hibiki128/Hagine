#include "Material.h"

#include "fstream"
#include <Graphics/Texture/TextureManager.h>
#include <Graphics/Srv/SrvManager.h>

void Material::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    CreateMaterial();
}

void Material::LoadTexture() {
    // テクスチャの読み込み
    TextureManager::GetInstance()->LoadTexture(materialData_.textureFilePath);
    materialData_.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(materialData_.textureFilePath);
}

void Material::PrimitiveInitialize(const PrimitiveType &type) {
    materialData_.color = PrimitiveModel::GetInstance()->GetPrimitiveData(type).color;
    materialData_.uvTransform = PrimitiveModel::GetInstance()->GetPrimitiveData(type).uvMatrix;
    materialData_.textureFilePath = "debug/uvChecker.png";
}

// デバッグ用: Material::Draw()でテクスチャインデックスを確認
void Material::Draw(const Vector4 &color, bool lighting) {
    // 描画時の一時的な値設定
    materialDataGPU_->color = color;
    materialDataGPU_->enableLighting = lighting ? 1 : 0;

    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, materialData_.textureIndex);
}


void Material::SetTexture(const std::string &texturePath) {
    if (materialData_.textureFilePath == texturePath)
        return;

    // テクスチャを読み込み
    TextureManager::GetInstance()->LoadTexture(texturePath);

    // MaterialDataを更新
    materialData_.textureFilePath = texturePath;
    materialData_.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(texturePath);

    // GPU側のデータも更新（色や他のパラメータも含む）
    UpdateGPUData();
}

void Material::SetEnvironmentCoefficients(float environmentCoefficients) {
    materialData_.environmentCoefficient = environmentCoefficients;
    UpdateGPUData();
}

MaterialData Material::LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename) {
    MaterialData materialData;                          // 構築するMaterialData
    std::string line;                                   // ファイルから読んだ1行を格納するもの
    std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
    assert(file.is_open());                             // 開けなかったら止める
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        // identifierに応じた処理
        if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            // 連結してファイルパスにする
            materialData.textureFilePath = directoryPath + "../images/" + textureFilename;
        }
    }

    // テクスチャが張られていない場合の処理
    if (materialData.textureFilePath.empty()) {
        materialData.textureFilePath = directoryPath + "/" + "white1x1.png";
    }

    return materialData;
}

void Material::CreateMaterial() {
    // GPUバッファの作成
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(MaterialDataGPU));
    materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialDataGPU_));

    // 初期値設定
    UpdateGPUData();
}

void Material::UpdateGPUData() {
    if (materialDataGPU_) {
        materialDataGPU_->color = materialData_.color;
        materialDataGPU_->enableLighting = materialData_.enableLighting ? 1 : 0;
        materialDataGPU_->uvTransform = materialData_.uvTransform;
        materialDataGPU_->shininess = materialData_.shininess;
        materialDataGPU_->environmentCoefficient = materialData_.environmentCoefficient;
    }
}
