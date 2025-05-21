#include "ModelManager.h"
#include <fstream>
#include <functional>
#include <sstream>
ModelManager *ModelManager::instance = nullptr;

ModelManager *ModelManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ModelManager;
    }
    return instance;
}

void ModelManager::LoadModel(const std::string &filePath) {

    // .gltfファイルの場合、内容に基づくハッシュを生成しない（毎回新しいモデルを作成）
    if (filePath.substr(filePath.find_last_of(".") + 1) == "gltf") {
        // 新しいユニークな識別子を生成する（例えば、インデックスなど）
        static int modelIndex = 0;
        std::string uniqueKey = filePath + "_" + std::to_string(modelIndex++);

        // モデルの生成とファイル読み込み、初期化
        std::unique_ptr<Model> model = std::make_unique<Model>();
        model->Initialize(modelCommon);
        model->CreateModel("resources/models/", filePath);
        model->SetSrv(srvManager);

        // モデルをmapコンテナに格納する
        models.insert(std::make_pair(uniqueKey, std::move(model)));
        return;
    }

    // .gltf以外のファイルは元のパスで検索（重複チェック）
    if (models.contains(filePath)) {
        return;
    }

    std::unique_ptr<Model> model = std::make_unique<Model>();
    model->Initialize(modelCommon);
    model->CreateModel("resources/models/", filePath);
    model->SetSrv(srvManager);
    models.insert(std::make_pair(filePath, std::move(model)));
}

std::string ModelManager::CreatePrimitiveModel(PrimitiveType type) {
    std::unique_ptr<Model> model = std::make_unique<Model>();
    model->Initialize(modelCommon);
    model->CreatePrimitiveModel(type);
    model->SetSrv(srvManager);
    // モデルのユニークな識別子を生成
    static int modelIndex = 0;
    std::string uniqueKey = "PrimitiveModel_" + std::to_string(modelIndex++);
    // モデルをmapコンテナに格納する
    models.insert(std::make_pair(uniqueKey, std::move(model)));
    return uniqueKey;
}

Model *ModelManager::FindModel(const std::string &filePath) {
    // .gltfファイルの場合はファイルパスにユニークな識別子を使って検索
    if (filePath.substr(filePath.find_last_of(".") + 1) == "gltf") {
        // 同じファイルパスで複数のモデルがある可能性があるので、それを確認
        std::vector<Model *> matchedModels;

        // キーがファイルパスを含むモデルをすべて収集
        for (const auto &[key, model] : models) {
            if (key.find(filePath) != std::string::npos) {
                matchedModels.push_back(model.get());
            }
        }

        // 一致するモデルがあれば、必要に応じて最も新しいモデルなどを選んで返す
        if (!matchedModels.empty()) {
            // 例えば、最も新しい（インデックスが最大）ものを返す
            return matchedModels.back();
        }
    } else {
        // .gltf以外のファイルはファイルパスそのもので検索
        if (models.contains(filePath)) {
            return models.at(filePath).get();
        }
    }

    return nullptr;
}

void ModelManager::Initialize(SrvManager *srvManager) {
    modelCommon = new ModelCommon;
    modelCommon->Initialize();
    this->srvManager = srvManager;
}

void ModelManager::Finalize() {
    delete instance;
    instance = nullptr;
}
