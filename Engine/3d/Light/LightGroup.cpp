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

    //obj3dCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(6, spotLightResource->GetGPUVirtualAddress());
}

void LightGroup::imgui() {
    ImGui::Begin("ライト設定");

    // 共通のタブバーを一つだけ作成
    if (ImGui::BeginTabBar("LightTypeTabs")) {
        // 平行光源タブ
        if (ImGui::BeginTabItem("平行光源")) {
            ImGui::Checkbox("平行光源アクティブ", &isDirectionalLight);
            if (directionalLightData->active) {
                ImGui::DragFloat3("方向", &directionalLightData->direction.x, 0.1f);
                directionalLightData->direction = directionalLightData->direction.Normalize();
                ImGui::DragFloat("輝度", &directionalLightData->intensity, 0.01f);
                ImGui::ColorEdit3("色", &directionalLightData->color.x);

                // 光源タイプ選択
                const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};
                int selectedLightingType = directionalLightData->BlinnPhong ? 1 : 0;
                if (ImGui::Combo("光源タイプ", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                    directionalLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                    directionalLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                }

                // セーブボタン
                if (ImGui::Button("セーブ")) {
                    SaveDirectionalLight();
                    std::string message = std::format("DirectionalLight saved.");
                    MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
                }
            }
            ImGui::EndTabItem();
        }

        // 点光源タブ
        if (ImGui::BeginTabItem("点光源")) {
            ImGui::Checkbox("点光源アクティブ", &isPointLight);
            if (pointLightData->active) {
                ImGui::DragFloat3("位置", &pointLightData->position.x, 0.1f);
                ImGui::DragFloat("輝度", &pointLightData->intensity, 0.01f);
                ImGui::DragFloat("減衰率", &pointLightData->decay, 0.1f);
                ImGui::DragFloat("半径", &pointLightData->radius, 0.1f);
                ImGui::ColorEdit3("色", &pointLightData->color.x);

                // 光源タイプ選択
                const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};
                int selectedLightingType = pointLightData->BlinnPhong ? 1 : 0;
                if (ImGui::Combo("光源タイプ", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                    pointLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                    pointLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                }

                // セーブボタン
                if (ImGui::Button("セーブ")) {
                    SavePointLight();
                    std::string message = std::format("PointLight saved.");
                    MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
                }
            }
            ImGui::EndTabItem();
        }

        // スポットライトタブ
        if (ImGui::BeginTabItem("スポットライト")) {
            ImGui::Checkbox("スポットライトアクティブ", &isSpotLight);
            if (spotLightData->active) {
                ImGui::DragFloat3("位置", &spotLightData->position.x, 0.1f);
                ImGui::DragFloat("輝度", &spotLightData->intensity, 0.01f);
                ImGui::DragFloat3("方向", &spotLightData->direction.x, 0.1f);
                spotLightData->direction = spotLightData->direction.Normalize();
                ImGui::DragFloat("減衰率", &spotLightData->decay, 0.1f);
                ImGui::DragFloat("距離", &spotLightData->distance, 0.1f);
                ImGui::DragFloat("余弦", &spotLightData->cosAngle, 0.1f);
                ImGui::ColorEdit3("色", &spotLightData->color.x);

                // 光源タイプ選択
                const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};
                int selectedLightingType = spotLightData->BlinnPhong ? 1 : 0;
                if (ImGui::Combo("光源タイプ", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                    spotLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                    spotLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                }

                // セーブボタン
                if (ImGui::Button("セーブ")) {
                    SaveSpotLight();
                    std::string message = std::format("SpotLight saved.");
                    MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void LightGroup::SaveDirectionalLight() {
    DLightData_->Save<bool>("active", isDirectionalLight);
    DLightData_->Save<Vector3>("direction", directionalLightData->direction);
    DLightData_->Save<float>("intensity", directionalLightData->intensity);
    DLightData_->Save<Vector4>("color", directionalLightData->color);
    DLightData_->Save<int32_t>("HalfLambert", directionalLightData->HalfLambert);
    DLightData_->Save<int32_t>("BlinnPghong", directionalLightData->BlinnPhong);
}

void LightGroup::SavePointLight() {
    PLightData_->Save<bool>("active", isPointLight);
    PLightData_->Save<Vector4>("color", pointLightData->color);
    PLightData_->Save<Vector3>("position", pointLightData->position);
    PLightData_->Save<int32_t>("HalfLambert", pointLightData->HalfLambert);
    PLightData_->Save<int32_t>("BlinnPhong", pointLightData->BlinnPhong);
    PLightData_->Save<float>("intensity", pointLightData->intensity);
    PLightData_->Save<float>("radius", pointLightData->radius);
    PLightData_->Save<float>("decay", pointLightData->decay);
}

void LightGroup::SaveSpotLight() {
    SLightData_->Save<bool>("active", isSpotLight);
    SLightData_->Save<Vector4>("color", spotLightData->color);
    SLightData_->Save<Vector3>("position", spotLightData->position);
    SLightData_->Save<Vector3>("direction", spotLightData->direction);
    SLightData_->Save<int32_t>("HalfLambert", spotLightData->HalfLambert);
    SLightData_->Save<int32_t>("BlinnPhong", spotLightData->BlinnPhong);
    SLightData_->Save<float>("intensity", spotLightData->intensity);
    SLightData_->Save<float>("distance", spotLightData->distance);
    SLightData_->Save<float>("cosAngle", spotLightData->cosAngle);
    SLightData_->Save<float>("decay", spotLightData->decay);
}

void LightGroup::LoadDirectionalLight() {
    DLightData_ = std::make_unique<DataHandler>("LightData", "DirectionalLight");
    isDirectionalLight = DLightData_->Load<bool>("active", true);
    directionalLightData->color = DLightData_->Load<Vector4>("color", {1.0f, 1.0f, 1.0f, 1.0f});
    directionalLightData->direction = DLightData_->Load<Vector3>("direction", {0.0f, -1.0f, 0.0f});
    directionalLightData->HalfLambert = DLightData_->Load<int32_t>("HalfLambert", false);
    directionalLightData->BlinnPhong = DLightData_->Load<int32_t>("BlinnPghong", true);
    directionalLightData->intensity = DLightData_->Load<float>("intensity", 1.0f);
}

void LightGroup::LoadPointLight() {
    PLightData_ = std::make_unique<DataHandler>("LightData", "PointLight");
    isPointLight = PLightData_->Load<bool>("active", false);
    pointLightData->color = PLightData_->Load<Vector4>("color", {1.0f, 1.0f, 1.0f, 1.0f});
    pointLightData->position = PLightData_->Load<Vector3>("position", {-1.0f, 4.0f, -3.0f});
    pointLightData->HalfLambert = PLightData_->Load<int32_t>("HalfLambert", false);
    pointLightData->BlinnPhong = PLightData_->Load<int32_t>("BlinnPhong", true);
    pointLightData->intensity = PLightData_->Load<float>("intensity", 1.0f);
    pointLightData->radius = PLightData_->Load<float>("radius", 2.0f);
    pointLightData->decay = PLightData_->Load<float>("decay", 1.0f);
}

void LightGroup::LoadSpotLight() {
    SLightData_ = std::make_unique<DataHandler>("LightData", "SpotLight");
    isSpotLight = SLightData_->Load<bool>("active", false);
    spotLightData->color = SLightData_->Load<Vector4>("color", {1.0f, 1.0f, 1.0f, 1.0f});
    spotLightData->position = SLightData_->Load<Vector3>("position", {0.0f, -4.0f, -3.0f});
    spotLightData->direction = SLightData_->Load<Vector3>("direction", {0.0f, -1.0f, 0.0f});
    spotLightData->HalfLambert = SLightData_->Load<int32_t>("HalfLambert", false);
    spotLightData->BlinnPhong = SLightData_->Load<int32_t>("BlinnPhong", true);
    spotLightData->intensity = SLightData_->Load<float>("intensity", 1.0f);
    spotLightData->distance = SLightData_->Load<float>("distance", 10.0f);
    spotLightData->cosAngle = SLightData_->Load<float>("cosAngle", 3.0f);
    spotLightData->decay = SLightData_->Load<float>("decay", 1.0f);
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
