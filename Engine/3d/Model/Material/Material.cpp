#include "Material.h"

#include <Texture/TextureManager.h>
#include "fstream"

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

void Material::Draw(Vector4 &color,bool &lighting) {
    ID3D12GraphicsCommandList *commandList = dxCommon_->GetCommandList().Get();
    materialDataGPU_->color = color;
    materialDataGPU_->enableLighting = lighting;
    commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
    SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(2, materialDataGPU_->textureIndex);
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
    // Sprite用のマテリアルリソースをつくる
    materialResource = dxCommon_->CreateBufferResource(sizeof(Material));
    // 書き込むためのアドレスを取得
    materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialDataGPU_));
    // 色の設定
    materialDataGPU_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    // Lightingの設定
    materialDataGPU_->enableLighting = true;
    materialDataGPU_->uvTransform = MakeIdentity4x4();
    materialDataGPU_->shininess = 20.0f;
}
