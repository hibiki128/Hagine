#include "LevelData.h"
#include <fstream> // ファイルストリーム
#include <stdexcept> // 例外処理用

// JSONファイルを読み込む関数
void LevelData::LoadJson(const std::string& jsonFileName) {
    fullpath = directoryPath_ + "/" + jsonFileName;
    // JSONファイルを開く処理
    std::ifstream file(fullpath);
    json sceneData;
    file >> sceneData;

    // "objects" 配列を読み込む
    for (const auto& obj : sceneData["objects"]) {
        // Object3dの生成
        std::string name = obj["name"];
        std::unique_ptr<Object3d> object3d = std::make_unique<Object3d>();

        // ファイル名を "name.obj" にする
        std::string filePath = name + ".obj";
        object3d->CreateModel(filePath); // Initializeでファイル名を使う

        // WorldTransformの生成（unique_ptrで管理）
        std::unique_ptr<WorldTransform> worldTransform = std::make_unique<WorldTransform>();

        // WorldTransformの初期化
        worldTransform->Initialize();

        // translation_, rotation_, scale_ を設定（Blender座標の変換を考慮）
        auto translation = obj["transform"]["translation"];
        auto rotation = obj["transform"]["rotation"];
        auto scaling = obj["transform"]["scaling"];

        // Blenderの座標系に合わせて変換 (x = x, y = z, z = y)
        worldTransform->translation_ = { translation[1], translation[2], translation[0] };
        worldTransform->rotation_ = { rotation[0], rotation[2], rotation[1] };
        worldTransform->scale_ = { scaling[0], scaling[2], scaling[1] };

        // 行列更新
        worldTransform->UpdateMatrix();

        // リストに格納
        worldTransforms.push_back(std::move(worldTransform)); // unique_ptrでmove
        object3dList.push_back(std::move(object3d)); // unique_ptrなのでmoveで管理
        objectNames.push_back(name); // 名前を格納
    }
}

// 指定した名前のオブジェクトのtranslationを取得
Vector3 LevelData::GetTranslationByName(const std::string& name) const {
    for (size_t i = 0; i < objectNames.size(); i++) {
        if (objectNames[i] == name) {
            return worldTransforms[i]->translation_;
        }
    }
    throw std::runtime_error("Object with name " + name + " not found.");
}

// 指定した名前のオブジェクトのrotationを取得
Vector3 LevelData::GetRotationByName(const std::string& name) const {
    for (size_t i = 0; i < objectNames.size(); i++) {
        if (objectNames[i] == name) {
            return worldTransforms[i]->rotation_;
        }
    }
    throw std::runtime_error("Object with name " + name + " not found.");
}

// 指定した名前のオブジェクトのscaleを取得
Vector3 LevelData::GetScaleByName(const std::string& name) const {
    for (size_t i = 0; i < objectNames.size(); i++) {
        if (objectNames[i] == name) {
            return worldTransforms[i]->scale_;
        }
    }
    throw std::runtime_error("Object with name " + name + " not found.");
}

// Draw関数
void LevelData::Draw(const ViewProjection& viewProjection) {
    // object3dList内の全てのオブジェクトを描画
    for (size_t i = 0; i < object3dList.size(); i++) {
        object3dList[i]->Draw(*worldTransforms[i], viewProjection); // 描画関数の呼び出し
    }
}