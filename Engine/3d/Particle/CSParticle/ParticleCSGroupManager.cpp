#include "ParticleCSGroupManager.h"

ParticleCSGroupManager *ParticleCSGroupManager::instance = nullptr;

ParticleCSGroupManager *ParticleCSGroupManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ParticleCSGroupManager();
    }
    return instance;
}

void ParticleCSGroupManager::Initialize() {
    const std::string directoryPath = "resources/jsons/ParticleCSGroup/";

    // ディレクトリが存在しない場合は何もしない
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return;
    }

    for (const auto &entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                json jsonData;
                file >> jsonData;

                // グループ名がなければスキップ
                if (!jsonData.contains("groupName") || !jsonData["groupName"].is_string()) {
                    continue;
                }

                std::string groupName = jsonData["groupName"];

                // テクスチャは存在チェックだけする
                std::string texturePath = jsonData.value("textureName", "");

                // モデルパスが空でないなら CreateParticleGroup を使う
                std::string modelPath = jsonData.value("modelfilePath", "");

                uint32_t maxParticleCount = jsonData.value("maxParticleCount", 10000);

                BlendMode blendMode = static_cast<BlendMode>(jsonData.value("blendMode", 2));

                if (!modelPath.empty()) {
                    CreateParticleCSGroup(groupName, modelPath, maxParticleCount, texturePath,blendMode);
                } else if (jsonData.contains("primitiveType")) {
                    int primitiveValue = jsonData["primitiveType"].get<int>();

                    // enum が有効範囲（0以上）であることをチェック
                    if (primitiveValue >= 0) {
                        PrimitiveType type = static_cast<PrimitiveType>(primitiveValue);
                        CreatePrimitiveParticleCSGroup(groupName, type, maxParticleCount, texturePath,blendMode);
                    }
                }

                file.close();
            }
        }
    }
}

void ParticleCSGroupManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void ParticleCSGroupManager::CreateParticleCSGroup(const std::string &groupName, const std::string &fileName, uint32_t maxParticleCount, const std::string &texturePath, BlendMode blendMode) {
    auto particleGroup = std::make_unique<ParticleCSGroup>();
    particleGroup->CreateParticleGroup(groupName, fileName, maxParticleCount, texturePath,blendMode);
    AddParticleCSGroup(std::move(particleGroup));
}

void ParticleCSGroupManager::CreatePrimitiveParticleCSGroup(const std::string &groupName, PrimitiveType type, uint32_t maxParticleCount, const std::string &texturePath, BlendMode blendMode) {
    auto particleGroup = std::make_unique<ParticleCSGroup>();
    particleGroup->CreatePrimitiveParticleGroup(groupName, type, maxParticleCount, texturePath,blendMode);
    AddParticleCSGroup(std::move(particleGroup));
}

void ParticleCSGroupManager::AddParticleCSGroup(std::unique_ptr<ParticleCSGroup> particleCSGroup) {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleCSGroup", particleCSGroup->GetGroupName());
    data->Save("groupName", particleCSGroup->GetGroupName());
    // materialがvectorになったため、最初のmaterialのtextureFilePathを保存
    const auto &materials = particleCSGroup->GetParticleGroupData().materials;
    std::string textureFilePath = (!materials.empty()) ? materials[0].textureFilePath : "";
    data->Save("textureName", textureFilePath);
    data->Save("modelfilePath", particleCSGroup->GetModelPath());
    data->Save("primitiveType", particleCSGroup->GetPrimitiveType());
    data->Save("maxParticleCount", particleCSGroup->GetSettingsData()->maxParticleCount);
    data->Save("blendMode", particleCSGroup->GetParticleGroupData().blendMode);
    particleGroups_.emplace_back(std::move(particleCSGroup));
}
