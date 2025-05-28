#include "ParticleGroupManager.h"

ParticleGroupManager *ParticleGroupManager::instance = nullptr;

ParticleGroupManager *ParticleGroupManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ParticleGroupManager();
    }
    return instance;
}

void ParticleGroupManager::Initialize() {
    const std::string directoryPath = "resources/jsons/ParticleGroup/";

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
                std::string texturePath = jsonData.value("textrueName", "");

                // モデルパスが空でないなら CreateParticleGroup を使う
                std::string modelPath = jsonData.value("modelfilePath", "");

                if (!modelPath.empty()) {
                    CreateParticleGroup(groupName, modelPath, texturePath);
                } else if (jsonData.contains("primitiveType")) {
                    int primitiveValue = jsonData["primitiveType"].get<int>();

                    // enum が有効範囲（0以上）であることをチェック
                    if (primitiveValue >= 0) {
                        PrimitiveType type = static_cast<PrimitiveType>(primitiveValue);
                        CreatePrimitiveParticleGroup(groupName, type, texturePath);
                    }
                }

                file.close();
            }
        }
    }
}


void ParticleGroupManager::Finalize() {
    delete instance;
    instance = nullptr;
}
void ParticleGroupManager::AddParticleGroup(std::unique_ptr<ParticleGroup> particleGroup) {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleGroup", particleGroup->GetGroupName());
    data->Save("groupName", particleGroup->GetGroupName());
    // materialがvectorになったため、最初のmaterialのtextureFilePathを保存
    const auto& materials = particleGroup->GetParticleGroupData().materials;
    std::string textureFilePath = (!materials.empty()) ? materials[0].textureFilePath : "";
    data->Save("textrueName", textureFilePath);
    data->Save("modelfilePath", particleGroup->GetModelPath());
    data->Save("primitiveType", particleGroup->GetPrimitiveType());
    particleGroups_.emplace_back(std::move(particleGroup));
}

void ParticleGroupManager::CreateParticleGroup(const std::string &groupName, const std::string &filename, const std::string &texturePath) {
    auto particleGroup = std::make_unique<ParticleGroup>();
    particleGroup->CreateParticleGroup(groupName, filename, texturePath);
    AddParticleGroup(std::move(particleGroup));
}

void ParticleGroupManager::CreatePrimitiveParticleGroup(const std::string &groupName, PrimitiveType type, const std::string &texturePath) {
    auto particleGroup = std::make_unique<ParticleGroup>();
    particleGroup->CreatePrimitiveParticleGroup(groupName, type, texturePath);
    AddParticleGroup(std::move(particleGroup));
}
