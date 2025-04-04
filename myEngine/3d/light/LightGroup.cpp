#include "LightGroup.h"
#include <filesystem>
#include <fstream>

LightGroup *LightGroup::instance = nullptr;

LightGroup *LightGroup::GetInstance() {
    if (instance == nullptr) {
        instance = new LightGroup();
    }
    return instance;
}

void LightGroup::Finalize() {
    delete instance;
    instance = nullptr;
}

void LightGroup::Initialize() {
    obj3dCommon = Object3dCommon::GetInstance();
    CreateCamera();
    CreatePointLight();
    CreateDirectionLight();
    CreateSpotLight();

    LoadDirectionalLight();
    LoadPointLight();
    LoadSpotLight();
}

void LightGroup::Update(const ViewProjection &viewProjection) {
    cameraForGPUData->worldPosition = viewProjection.translation_;
    if (isDirectionalLight) {
        directionalLightData->active = true;
    } else {
        directionalLightData->active = false;
    }
    if (isPointLight) {
        pointLightData->active = true;
    } else {
        pointLightData->active = false;
    }
    if (isSpotLight) {
        spotLightData->active = true;
    } else {
        spotLightData->active = false;
    }
}

void LightGroup::Draw() {
    // DirectionalLight用のCBufferの場所を設定
    obj3dCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

    obj3dCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraForGPUResource->GetGPUVirtualAddress());

    obj3dCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(5, pointLightResource->GetGPUVirtualAddress());

    obj3dCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(6, spotLightResource->GetGPUVirtualAddress());
}

void LightGroup::imgui() {

    if (ImGui::BeginTabBar("Direction")) {
        if (ImGui::BeginTabItem("Direction")) {
            ImGui::Checkbox("directionalLight", &isDirectionalLight);
            if (directionalLightData->active) {
                ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.1f);
                directionalLightData->direction = directionalLightData->direction.Normalize();
                ImGui::DragFloat("Intensity", &directionalLightData->intensity, 0.01f);
                ImGui::ColorEdit3("Color", &directionalLightData->color.x);
                // "HalfLambert", "BlinnPhong" の2つの選択肢を用意
                const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};

                int selectedLightingType = directionalLightData->BlinnPhong ? 1 : 0; // 初期値はBlinnPhong

                // Comboで選択されたインデックスに基づいてフラグを設定
                if (ImGui::Combo("Lighting Type", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                    // フラグの設定
                    directionalLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                    directionalLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                }
            }
            ImGui::EndTabItem();
            if (ImGui::Button("Save")) {
                SaveDirectionalLight();
                std::string message = std::format("DirectionalLight saved.");
                MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
            }
        }
        ImGui::EndTabBar();
    }

    if (ImGui::BeginTabBar("Point")) {
        if (ImGui::BeginTabItem("Point")) {
            ImGui::Checkbox("pointLight", &isPointLight);
            if (pointLightData->active) {
                ImGui::DragFloat3("Position", &pointLightData->position.x, 0.1f);
                ImGui::DragFloat("Intensity", &pointLightData->intensity, 0.01f);
                ImGui::DragFloat("Decay", &pointLightData->decay, 0.1f);
                ImGui::DragFloat("Radius", &pointLightData->radius, 0.1f);
                ImGui::ColorEdit3("Color", &pointLightData->color.x);
                // "HalfLambert", "BlinnPhong" の2つの選択肢を用意
                const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};

                int selectedLightingType = pointLightData->BlinnPhong ? 1 : 0; // 初期値はBlinnPhong

                // Comboで選択されたインデックスに基づいてフラグを設定
                if (ImGui::Combo("Lighting Type", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                    // フラグの設定
                    pointLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                    pointLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                }
            }
            if (ImGui::Button("Save")) {
                SavePointLight();
                std::string message = std::format("PointLight saved.");
                MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (ImGui::BeginTabBar("Spot")) {
        if (ImGui::BeginTabItem("Spot")) {
            ImGui::Checkbox("spotLight", &isSpotLight);
            if (spotLightData->active) {
                ImGui::DragFloat3("Position", &spotLightData->position.x, 0.1f);
                ImGui::DragFloat("Intensity", &spotLightData->intensity, 0.01f);
                ImGui::DragFloat3("LightDirection", &spotLightData->direction.x, 0.1f);
                spotLightData->direction = spotLightData->direction.Normalize();
                ImGui::DragFloat("Decay", &spotLightData->decay, 0.1f);
                ImGui::DragFloat("Distance", &spotLightData->distance, 0.1f);
                ImGui::DragFloat("cosAngle", &spotLightData->cosAngle, 0.1f);
                ImGui::ColorEdit3("Color", &spotLightData->color.x);
                // "HalfLambert", "BlinnPhong" の2つの選択肢を用意
                const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};

                int selectedLightingType = spotLightData->BlinnPhong ? 1 : 0; // 初期値はBlinnPhong

                // Comboで選択されたインデックスに基づいてフラグを設定
                if (ImGui::Combo("Lighting Type", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                    // フラグの設定
                    spotLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                    spotLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                }
            }
            if (ImGui::Button("Save")) {
                SaveSpotLight();
                std::string message = std::format("SpotLight saved.");
                MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void LightGroup::SaveDirectionalLight() {
    // 保存先の固定パス
    const std::string filePath = "resources/jsons/LightGroup/directionalLightData.json";

    // 必要なフォルダを作成
    std::filesystem::create_directories("resources/jsons/LightGroup");

    // JSONデータ作成
    nlohmann::json directionalLightJson = {
        {"active", isDirectionalLight},
        {"direction", {directionalLightData->direction.x, directionalLightData->direction.y, directionalLightData->direction.z}},
        {"intensity", directionalLightData->intensity},
        {"color", {directionalLightData->color.x, directionalLightData->color.y, directionalLightData->color.z}},
        {"HalfLambert", directionalLightData->HalfLambert}, // 追加
        {"BlinnPhong", directionalLightData->BlinnPhong}    // 追加
    };

    // ファイルに書き込み
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << directionalLightJson.dump(4); // インデントを4スペースに設定
        outFile.close();
    }
}

void LightGroup::SavePointLight() {
    // 保存先の固定パス
    const std::string filePath = "resources/jsons/LightGroup/pointLightData.json";

    // 必要なフォルダを作成
    std::filesystem::create_directories("resources/jsons/LightGroup");

    // JSONデータ作成
    nlohmann::json pointLightJson = {
        {"active", isPointLight},
        {"position", {pointLightData->position.x, pointLightData->position.y, pointLightData->position.z}},
        {"intensity", pointLightData->intensity},
        {"decay", pointLightData->decay},
        {"radius", pointLightData->radius},
        {"color", {pointLightData->color.x, pointLightData->color.y, pointLightData->color.z}},
        {"HalfLambert", pointLightData->HalfLambert}, // 追加
        {"BlinnPhong", pointLightData->BlinnPhong}    // 追加
    };

    // ファイルに書き込み
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << pointLightJson.dump(4); // インデントを4スペースに設定
        outFile.close();
    }
}

void LightGroup::SaveSpotLight() {
    // 保存先の固定パス
    const std::string filePath = "resources/jsons/LightGroup/spotLightData.json";

    // 必要なフォルダを作成
    std::filesystem::create_directories("resources/jsons/LightGroup");

    // JSONデータ作成
    nlohmann::json spotLightJson = {
        {"active", isSpotLight},
        {"direction", {spotLightData->direction.x, spotLightData->direction.y, spotLightData->direction.z}},
        {"intensity", spotLightData->intensity},
        {"color", {spotLightData->color.x, spotLightData->color.y, spotLightData->color.z}},
        {"decay", spotLightData->decay},
        {"distance", spotLightData->distance},
        {"cosAngle", spotLightData->cosAngle},
        {"HalfLambert", spotLightData->HalfLambert}, // 追加
        {"BlinnPhong", spotLightData->BlinnPhong}    // 追加
    };

    // ファイルに書き込み
    std::ofstream outFile(filePath);
    if (outFile.is_open()) {
        outFile << spotLightJson.dump(4); // インデントを4スペースに設定
        outFile.close();
    }
}

void LightGroup::LoadDirectionalLight() {
    // 読み込み元の固定パス
    const std::string filePath = "resources/jsons/LightGroup/directionalLightData.json";

    // ファイルが存在しない場合は早期リターン
    std::ifstream inFile(filePath);
    if (!inFile.is_open())
        return;

    // JSONデータ読み込み
    nlohmann::json directionalLightJson;
    inFile >> directionalLightJson;
    inFile.close();

    // Directional Lightの情報を設定
    if (directionalLightJson.contains("active"))
        isDirectionalLight = directionalLightJson["active"];
    if (directionalLightJson.contains("direction"))
        directionalLightData->direction = {
            directionalLightJson["direction"][0],
            directionalLightJson["direction"][1],
            directionalLightJson["direction"][2]};
    if (directionalLightJson.contains("intensity"))
        directionalLightData->intensity = directionalLightJson["intensity"];
    if (directionalLightJson.contains("color"))
        directionalLightData->color = {
            directionalLightJson["color"][0],
            directionalLightJson["color"][1],
            directionalLightJson["color"][2]};

    // HalfLambert, BlinnPhong の値を読み込む
    if (directionalLightJson.contains("HalfLambert"))
        directionalLightData->HalfLambert = directionalLightJson["HalfLambert"];
    if (directionalLightJson.contains("BlinnPhong"))
        directionalLightData->BlinnPhong = directionalLightJson["BlinnPhong"];
}

void LightGroup::LoadPointLight() {
    // 読み込み元の固定パス
    const std::string filePath = "resources/jsons/LightGroup/pointLightData.json";

    // ファイルが存在しない場合は早期リターン
    std::ifstream inFile(filePath);
    if (!inFile.is_open())
        return;

    // JSONデータ読み込み
    nlohmann::json pointLightJson;
    inFile >> pointLightJson;
    inFile.close();

    // Point Lightの情報を設定
    if (pointLightJson.contains("active"))
        isPointLight = pointLightJson["active"];
    if (pointLightJson.contains("position"))
        pointLightData->position = {
            pointLightJson["position"][0],
            pointLightJson["position"][1],
            pointLightJson["position"][2]};
    if (pointLightJson.contains("intensity"))
        pointLightData->intensity = pointLightJson["intensity"];
    if (pointLightJson.contains("decay"))
        pointLightData->decay = pointLightJson["decay"];
    if (pointLightJson.contains("radius"))
        pointLightData->radius = pointLightJson["radius"];
    if (pointLightJson.contains("color"))
        pointLightData->color = {
            pointLightJson["color"][0],
            pointLightJson["color"][1],
            pointLightJson["color"][2]};

    // HalfLambert, BlinnPhong の値を読み込む
    if (pointLightJson.contains("HalfLambert"))
        pointLightData->HalfLambert = pointLightJson["HalfLambert"];
    if (pointLightJson.contains("BlinnPhong"))
        pointLightData->BlinnPhong = pointLightJson["BlinnPhong"];
}

void LightGroup::LoadSpotLight() {
    // 読み込み元の固定パス
    const std::string filePath = "resources/jsons/LightGroup/spotLightData.json";

    // ファイルが存在しない場合は早期リターン
    std::ifstream inFile(filePath);
    if (!inFile.is_open())
        return;

    // JSONデータ読み込み
    nlohmann::json spotLightJson;
    inFile >> spotLightJson;
    inFile.close();

    // Spot Lightの情報を設定
    if (spotLightJson.contains("active"))
        isSpotLight = spotLightJson["active"];
    if (spotLightJson.contains("direction"))
        spotLightData->direction = {
            spotLightJson["direction"][0],
            spotLightJson["direction"][1],
            spotLightJson["direction"][2]};
    if (spotLightJson.contains("intensity"))
        spotLightData->intensity = spotLightJson["intensity"];
    if (spotLightJson.contains("color"))
        spotLightData->color = {
            spotLightJson["color"][0],
            spotLightJson["color"][1],
            spotLightJson["color"][2]};
    if (spotLightJson.contains("decay"))
        spotLightData->decay = spotLightJson["decay"];
    if (spotLightJson.contains("distance"))
        spotLightData->distance = spotLightJson["distance"];
    if (spotLightJson.contains("cosAngle"))
        spotLightData->cosAngle = spotLightJson["cosAngle"];

    // HalfLambert, BlinnPhong の値を読み込む
    if (spotLightJson.contains("HalfLambert"))
        spotLightData->HalfLambert = spotLightJson["HalfLambert"];
    if (spotLightJson.contains("BlinnPhong"))
        spotLightData->BlinnPhong = spotLightJson["BlinnPhong"];
}

void LightGroup::CreatePointLight() {
    pointLightResource = obj3dCommon->GetDxCommon()->CreateBufferResource(sizeof(PointLight));
    // 書き込むためのアドレスを取得
    pointLightResource->Map(0, nullptr, reinterpret_cast<void **>(&pointLightData));
    // デフォルト値
    pointLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
    pointLightData->position = {-1.0f, 4.0f, -3.0f};
    pointLightData->intensity = 1.0f;
    pointLightData->decay = 1.0f;
    pointLightData->radius = 2.0f;
    pointLightData->active = false;
    pointLightData->HalfLambert = false;
    pointLightData->BlinnPhong = true;
}

void LightGroup::CreateSpotLight() {
    spotLightResource = obj3dCommon->GetDxCommon()->CreateBufferResource(sizeof(SpotLight));
    // 書き込むためのアドレスを取得
    spotLightResource->Map(0, nullptr, reinterpret_cast<void **>(&spotLightData));
    // デフォルト値
    spotLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
    spotLightData->position = {0.0f, -4.0f, -3.0f};
    spotLightData->direction = {0.0f, -1.0f, 0.0f};
    spotLightData->intensity = 1.0f;
    spotLightData->distance = 10.0f;
    spotLightData->decay = 1.0f;
    spotLightData->cosAngle = 3.0f;
    spotLightData->active = false;
    spotLightData->HalfLambert = false;
    spotLightData->BlinnPhong = true;
}

void LightGroup::CreateDirectionLight() {
    directionalLightResource = obj3dCommon->GetDxCommon()->CreateBufferResource(sizeof(DirectionLight));
    // 書き込むためのアドレスを取得
    directionalLightResource->Map(0, nullptr, reinterpret_cast<void **>(&directionalLightData));
    // デフォルト値
    directionalLightData->color = {1.0f, 1.0f, 1.0f, 1.0f};
    directionalLightData->direction = {0.0f, -1.0f, 0.0f};
    directionalLightData->intensity = 1.0f;
    directionalLightData->active = true;
    directionalLightData->HalfLambert = false;
    directionalLightData->BlinnPhong = true;
}

void LightGroup::CreateCamera() {
    cameraForGPUResource = obj3dCommon->GetDxCommon()->CreateBufferResource(sizeof(CameraForGPU));
    cameraForGPUResource->Map(0, nullptr, reinterpret_cast<void **>(&cameraForGPUData));
    cameraForGPUData->worldPosition = {0.0f, 0.0f, -50.0f};
}
