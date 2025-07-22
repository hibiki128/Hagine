#include "LightGroup.h"
#include "DirectXCommon.h"
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
    dxCommon_ = DirectXCommon::GetInstance();
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
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraForGPUResource->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, pointLightResource->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, spotLightResource->GetGPUVirtualAddress());
}

void LightGroup::imgui() {
    // スタイル設定
    ImGuiStyle &style = ImGui::GetStyle();
    float originalRounding = style.ChildRounding;
    float originalPadding = style.FramePadding.x;

    style.ChildRounding = 6.0f;
    style.FramePadding = ImVec2(8.0f, 4.0f);

    // メインタブバー
    if (ImGui::BeginTabBar("LightTypeTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll)) {

        // 平行光源タブ
        if (ImGui::BeginTabItem("平行光源")) {
            ImGui::Spacing();

            // アクティブ状態
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            ImGui::Checkbox("平行光源を有効にする", &isDirectionalLight);
            ImGui::PopStyleColor();

            if (directionalLightData->active) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 基本設定セクション
                if (ImGui::BeginChild("DirectionalBasic", ImVec2(0, 120), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("基本設定");
                    ImGui::Spacing();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, 200);

                    ImGui::Text("方向");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光の進む方向を指定します");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat3("##direction", &directionalLightData->direction.x, 0.1f);
                    directionalLightData->direction = directionalLightData->direction.Normalize();
                    ImGui::NextColumn();

                    ImGui::Text("輝度");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光の明るさを調整します");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat("##intensity", &directionalLightData->intensity, 0.01f, 0.0f, 10.0f);
                    ImGui::NextColumn();

                    ImGui::Text("色");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光の色を設定します");
                    }
                    ImGui::NextColumn();
                    ImGui::ColorEdit3("##color", &directionalLightData->color.x);

                    ImGui::Columns(1);
                }
                ImGui::EndChild();

                ImGui::Spacing();

                // 光源タイプセクション
                if (ImGui::BeginChild("DirectionalType", ImVec2(0, 80), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("光源タイプ");
                    ImGui::Spacing();

                    const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};
                    int selectedLightingType = directionalLightData->BlinnPhong ? 1 : 0;

                    ImGui::SetNextItemWidth(200);
                    if (ImGui::Combo("##lightingType", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                        directionalLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                        directionalLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                    }

                    ImGui::SameLine();
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光の計算方式を選択します\nHalfLambert: より柔らかい陰影\nBlinnPhong: より鮮明な反射");
                    }
                }
                ImGui::EndChild();

                ImGui::Spacing();

                // セーブボタン
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));

                if (ImGui::Button("設定を保存", ImVec2(150, 30))) {
                    SaveDirectionalLight();
                    std::string message = std::format("DirectionalLight saved.");
                    MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::EndTabItem();
        }

        // 点光源タブ
        if (ImGui::BeginTabItem("点光源")) {
            ImGui::Spacing();

            // アクティブ状態
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            ImGui::Checkbox("点光源を有効にする", &isPointLight);
            ImGui::PopStyleColor();

            if (pointLightData->active) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 位置・基本設定セクション
                if (ImGui::BeginChild("PointBasic", ImVec2(0, 140), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("位置・基本設定");
                    ImGui::Spacing();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, 200);

                    ImGui::Text("位置");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光源の3D空間での位置");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat3("##position", &pointLightData->position.x, 0.1f);
                    ImGui::NextColumn();

                    ImGui::Text("輝度");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光の明るさを調整します");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat("##intensity", &pointLightData->intensity, 0.01f, 0.0f, 10.0f);
                    ImGui::NextColumn();

                    ImGui::Text("色");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光の色を設定します");
                    }
                    ImGui::NextColumn();
                    ImGui::ColorEdit3("##color", &pointLightData->color.x);

                    ImGui::Columns(1);
                }
                ImGui::EndChild();

                ImGui::Spacing();

                // 減衰設定セクション
                if (ImGui::BeginChild("PointAttenuation", ImVec2(0, 100), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("減衰設定");
                    ImGui::Spacing();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, 200);

                    ImGui::Text("減衰率");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("距離による光の減衰の強さ");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat("##decay", &pointLightData->decay, 0.1f, 0.0f, 5.0f);
                    ImGui::NextColumn();

                    ImGui::Text("半径");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光が届く最大距離");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat("##radius", &pointLightData->radius, 0.1f, 0.1f, 100.0f);

                    ImGui::Columns(1);
                }
                ImGui::EndChild();

                ImGui::Spacing();

                // 光源タイプセクション
                if (ImGui::BeginChild("PointType", ImVec2(0, 80), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("光源タイプ");
                    ImGui::Spacing();

                    const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};
                    int selectedLightingType = pointLightData->BlinnPhong ? 1 : 0;

                    ImGui::SetNextItemWidth(200);
                    if (ImGui::Combo("##lightingType", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                        pointLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                        pointLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                    }
                }
                ImGui::EndChild();

                ImGui::Spacing();

                // セーブボタン
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));

                if (ImGui::Button("設定を保存", ImVec2(150, 30))) {
                    SavePointLight();
                    std::string message = std::format("PointLight saved.");
                    MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::EndTabItem();
        }

        // スポットライトタブ
        if (ImGui::BeginTabItem("スポットライト")) {
            ImGui::Spacing();

            // アクティブ状態
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            ImGui::Checkbox("スポットライトを有効にする", &isSpotLight);
            ImGui::PopStyleColor();

            if (spotLightData->active) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 位置・方向セクション
                if (ImGui::BeginChild("SpotPositionDirection", ImVec2(0, 120), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("位置・方向設定");
                    ImGui::Spacing();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, 200);

                    ImGui::Text("位置");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("スポットライトの3D空間での位置");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat3("##position", &spotLightData->position.x, 0.1f);
                    ImGui::NextColumn();

                    ImGui::Text("方向");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("スポットライトが照らす方向");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat3("##direction", &spotLightData->direction.x, 0.1f);
                    spotLightData->direction = spotLightData->direction.Normalize();
                    ImGui::NextColumn();

                    ImGui::Text("輝度");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光の明るさを調整します");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat("##intensity", &spotLightData->intensity, 0.01f, 0.0f, 10.0f);

                    ImGui::Columns(1);
                }
                ImGui::EndChild();

                ImGui::Spacing();

                // 範囲設定セクション
                if (ImGui::BeginChild("SpotRange", ImVec2(0, 140), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("範囲設定");
                    ImGui::Spacing();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, 200);

                    ImGui::Text("減衰率");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("距離による光の減衰の強さ");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat("##decay", &spotLightData->decay, 0.1f, 0.0f, 5.0f);
                    ImGui::NextColumn();

                    ImGui::Text("距離");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光が届く最大距離");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat("##distance", &spotLightData->distance, 0.1f, 0.1f, 100.0f);
                    ImGui::NextColumn();

                    ImGui::Text("余弦");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("スポットライトの円錐角度の余弦値");
                    }
                    ImGui::NextColumn();
                    ImGui::DragFloat("##cosAngle", &spotLightData->cosAngle, 0.01f, -1.0f, 1.0f);
                    ImGui::NextColumn();

                    ImGui::Text("色");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("光の色を設定します");
                    }
                    ImGui::NextColumn();
                    ImGui::ColorEdit3("##color", &spotLightData->color.x);

                    ImGui::Columns(1);
                }
                ImGui::EndChild();

                ImGui::Spacing();

                // 光源タイプセクション
                if (ImGui::BeginChild("SpotType", ImVec2(0, 80), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("光源タイプ");
                    ImGui::Spacing();

                    const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};
                    int selectedLightingType = spotLightData->BlinnPhong ? 1 : 0;

                    ImGui::SetNextItemWidth(200);
                    if (ImGui::Combo("##lightingType", &selectedLightingType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                        spotLightData->HalfLambert = (selectedLightingType == 0) ? 1 : 0;
                        spotLightData->BlinnPhong = (selectedLightingType == 1) ? 1 : 0;
                    }
                }
                ImGui::EndChild();

                ImGui::Spacing();

                // セーブボタン
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));

                if (ImGui::Button("設定を保存", ImVec2(150, 30))) {
                    SaveSpotLight();
                    std::string message = std::format("SpotLight saved.");
                    MessageBoxA(nullptr, message.c_str(), "LightGroup", 0);
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    // スタイルを元に戻す
    style.ChildRounding = originalRounding;
    style.FramePadding = ImVec2(originalPadding, style.FramePadding.y);
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
    pointLightResource = dxCommon_->CreateBufferResource(sizeof(PointLight));
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
    spotLightResource = dxCommon_->CreateBufferResource(sizeof(SpotLight));
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
    directionalLightResource = dxCommon_->CreateBufferResource(sizeof(DirectionLight));
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
    cameraForGPUResource = dxCommon_->CreateBufferResource(sizeof(CameraForGPU));
    cameraForGPUResource->Map(0, nullptr, reinterpret_cast<void **>(&cameraForGPUData));
    cameraForGPUData->worldPosition = {0.0f, 0.0f, -50.0f};
}
