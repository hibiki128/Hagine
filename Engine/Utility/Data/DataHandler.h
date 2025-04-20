#pragma once
#include <Vector2.h>
#include <Vector3.h>
#include"Vector4.h"
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include"iostream"
#include <memory>
#include <cstdint>

using json = nlohmann::json;
namespace fs = std::filesystem;

class DataHandler {
  private:
    std::string basePath = "resources/jsons"; // 固定の基準パス
    std::string folderPath = "";              // インスタンスごとのフォルダ
    std::string fileName = "data.json";       // インスタンスごとのファイル名

  public:
    // コンストラクタ
    DataHandler(const std::string &folder, const std::string &file);

    // JSONデータを保存
    template <typename T>
    void Save(const std::string &key, const T &value);

    // JSONデータをロード
    template <typename T>
    T Load(const std::string &key, const T &defaultValue);
};

// JSON変換の定義 (Vector2)
inline void to_json(json &j, const Vector2 &v) {
    j = json{{"x", v.x}, {"y", v.y}};
}

inline void from_json(const json &j, Vector2 &v) {
    v.x = j.at("x").get<float>();
    v.y = j.at("y").get<float>();
}

// JSON変換の定義 (Vector3)
inline void to_json(json &j, const Vector3 &v) {
    j = json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

inline void from_json(const json &j, Vector3 &v) {
    v.x = j.at("x").get<float>();
    v.y = j.at("y").get<float>();
    v.z = j.at("z").get<float>();
}

// JSON変換の定義 (Vector3)
inline void to_json(json &j, const Vector4 &v) {
    j = json{{"x", v.x}, {"y", v.y}, {"z", v.z}, {"w",v.w}};
}

inline void from_json(const json &j, Vector4 &v) {
    v.x = j.at("x").get<float>();
    v.y = j.at("y").get<float>();
    v.z = j.at("z").get<float>();
    v.w = j.at("w").get<float>();
}

// Save (テンプレート関数はここに書く)
template <typename T>
void DataHandler::Save(const std::string &key, const T &value) {
    std::string filePath = folderPath + "/" + fileName;
    json j;

    std::ifstream inFile(filePath);
    if (inFile.is_open()) {
        inFile >> j;
        inFile.close();
    }

    j[key] = value; // 変数名をキーにして保存

    std::ofstream outFile(filePath);
    outFile << j.dump(4); // インデント付きで保存
    outFile.close();
}

// Load (テンプレート関数はここに書く)
template <typename T>
T DataHandler::Load(const std::string &key, const T &defaultValue) {
    std::string filePath = folderPath + "/" + fileName;
    json j;

    std::ifstream inFile(filePath);
    if (inFile.is_open()) {
        inFile >> j;
        inFile.close();
    } else {
        return defaultValue; // ファイルがない場合
    }

    if (j.contains(key)) {
        try {
            return j[key].get<T>(); // from_json が自動適用
        } catch (const json::exception &e) {
            std::cerr << "JSON Load Error: " << e.what() << " (Key: " << key << ")" << std::endl;
        }
    }

    return defaultValue; // 失敗した場合はデフォルト値
}
