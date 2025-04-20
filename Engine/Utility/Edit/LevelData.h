#include <vector>
#include <string>
#include <memory>
#include "externals/nlohmann/json.hpp"
#include "Object/Object3d.h"
#include "WorldTransform.h" 
#include "ViewProjection/ViewProjection.h"
#include "Vector3.h" 

using json = nlohmann::json;

class LevelData {
private:
    // メンバ変数
    std::vector<std::unique_ptr<WorldTransform>> worldTransforms;
    std::vector<std::unique_ptr<Object3d>> object3dList;    // Object3dのリスト
    std::vector<std::string> objectNames;                   // 読み込んだオブジェクトの名前リスト
    std::string directoryPath_ = "resources/jsons";
    std::string fullpath;

public:
    // JSONファイルを読み込む関数
    void LoadJson(const std::string& jsonFileName);

    // 指定した名前のオブジェクトのtranslation, rotation, scaleを取得する
    Vector3 GetTranslationByName(const std::string& name) const;
    Vector3 GetRotationByName(const std::string& name) const;
    Vector3 GetScaleByName(const std::string& name) const;

    // Draw関数
    void Draw(const ViewProjection& viewProjection);
};
