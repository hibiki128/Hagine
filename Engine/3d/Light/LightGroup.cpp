#include "LightGroup.h"
#include "DirectXCommon.h"
#include <Line/DrawLine3D.h>
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
    CreatePointLights();
    CreateDirectionLight();
    CreateSpotLights();
}

void LightGroup::Update(const ViewProjection &viewProjection) {
    cameraForGPUData->worldPosition = viewProjection.translation_;

    if (isDirectionalLight) {
        directionalLightData->active = true;
    } else {
        directionalLightData->active = false;
    }

    // ポイントライトデータ更新
    UpdatePointLightBuffer();

    // スポットライトデータ更新
    UpdateSpotLightBuffer();

    DrawLightVisualization();
}

void LightGroup::Draw() {
    // DirectionalLight用のCBufferの場所を設定
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraForGPUResource->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, pointLightsResource->GetGPUVirtualAddress());

    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, spotLightsResource->GetGPUVirtualAddress());
}

void LightGroup::AddPointLight() {
    if (pointLights_.size() >= MAX_POINT_LIGHTS) {
        return; // 最大数に達している場合は追加しない
    }

    PointLight newLight = {};
    newLight.color = {1.0f, 1.0f, 1.0f, 1.0f};
    newLight.position = {-1.0f, 4.0f, -3.0f};
    newLight.intensity = 1.0f;
    newLight.decay = 1.0f;
    newLight.radius = 2.0f;
    newLight.active = true;
    newLight.HalfLambert = false;
    newLight.BlinnPhong = true;

    pointLights_.push_back(newLight);
}

void LightGroup::RemovePointLight(int index) {
    if (index >= 0 && index < static_cast<int>(pointLights_.size())) {
        pointLights_.erase(pointLights_.begin() + index);
    }
}

void LightGroup::AddSpotLight() {
    if (spotLights_.size() >= MAX_SPOT_LIGHTS) {
        return; // 最大数に達している場合は追加しない
    }

    SpotLight newLight = {};
    newLight.color = {1.0f, 1.0f, 1.0f, 1.0f};
    newLight.position = {0.0f, 2.0f, 0.0f};
    newLight.direction = {0.0f, -1.0f, 0.0f};
    newLight.intensity = 1.0f;
    newLight.active = true;
    newLight.distance = 10.0f;
    newLight.decay = 1.0f;
    newLight.cosAngle = 0.7f; // 約45度
    newLight.HalfLambert = false;
    newLight.BlinnPhong = true;

    spotLights_.push_back(newLight);
}

void LightGroup::RemoveSpotLight(int index) {
    if (index >= 0 && index < static_cast<int>(spotLights_.size())) {
        spotLights_.erase(spotLights_.begin() + index);
    }
}

void LightGroup::UpdatePointLightBuffer() {
    pointLightsData->count = static_cast<int32_t>(pointLights_.size());

    for (size_t i = 0; i < pointLights_.size() && i < MAX_POINT_LIGHTS; ++i) {
        pointLightsData->lights[i] = pointLights_[i];
    }
}

void LightGroup::UpdateSpotLightBuffer() {
    spotLightsData->count = static_cast<int32_t>(spotLights_.size());

    for (size_t i = 0; i < spotLights_.size() && i < MAX_SPOT_LIGHTS; ++i) {
        spotLightsData->lights[i] = spotLights_[i];
    }
}

void LightGroup::CreatePointLights() {
    // サイズを明示的に計算
    size_t bufferSize = sizeof(PointLights);
    pointLightsResource = dxCommon_->CreateBufferResource(bufferSize);
    pointLightsResource->Map(0, nullptr, reinterpret_cast<void **>(&pointLightsData));

    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        pointLightsData->lights[i].color = {1.0f, 1.0f, 1.0f, 1.0f};
        pointLightsData->lights[i].position = {-1.0f, 4.0f, -3.0f};
        pointLightsData->lights[i].intensity = 1.0f;
        pointLightsData->lights[i].decay = 1.0f;
        pointLightsData->lights[i].radius = 2.0f;
        pointLightsData->lights[i].active = false;
        pointLightsData->lights[i].HalfLambert = false;
        pointLightsData->lights[i].BlinnPhong = true;
    }

    pointLightsData->count = 0;
}

void LightGroup::CreateSpotLights() {
    spotLightsResource = dxCommon_->CreateBufferResource(sizeof(SpotLights));
    // 書き込むためのアドレスを取得
    spotLightsResource->Map(0, nullptr, reinterpret_cast<void **>(&spotLightsData));

    for (int i = 0; i < MAX_SPOT_LIGHTS; i++) {
        spotLightsData->lights[i].color = {1.0f, 1.0f, 1.0f, 1.0f};
        spotLightsData->lights[i].position = {0.0f, -4.0f, -3.0f};
        spotLightsData->lights[i].direction = {0.0f, -1.0f, 0.0f};
        spotLightsData->lights[i].intensity = 1.0f;
        spotLightsData->lights[i].distance = 10.0f;
        spotLightsData->lights[i].decay = 1.0f;
        spotLightsData->lights[i].cosAngle = 3.0f;
        spotLightsData->lights[i].active = false;
        spotLightsData->lights[i].HalfLambert = false;
        spotLightsData->lights[i].BlinnPhong = true;
    }

    spotLightsData->count = 0;
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


void LightGroup::imgui() {
#ifdef USE_IMGUI

    // スタイル設定
    ImGuiStyle &style = ImGui::GetStyle();
    float originalRounding = style.ChildRounding;
    float originalPadding = style.FramePadding.x;

    style.ChildRounding = 6.0f;
    style.FramePadding = ImVec2(8.0f, 4.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("デバッグ設定");
    ImGui::Spacing();

    ImGui::Checkbox("光源可視化を表示", &showLightVisualization_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("光源の位置、方向、範囲を線で表示します");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

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
                }
                ImGui::EndChild();

                ImGui::Spacing();

            }
            ImGui::EndTabItem();
        }

        // 点光源タブ
        if (ImGui::BeginTabItem("点光源")) {
            ImGui::Spacing();

            // 追加・削除ボタン
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button("点光源を追加") && pointLights_.size() < MAX_POINT_LIGHTS) {
                AddPointLight();
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::Text("(%d/%d)", static_cast<int>(pointLights_.size()), MAX_POINT_LIGHTS);

            ImGui::Separator();
            ImGui::Spacing();

            // 各点光源の設定
            for (int i = 0; i < static_cast<int>(pointLights_.size()); ++i) {
                ImGui::PushID(i);

                std::string headerLabel = std::format("点光源 #{}", i + 1);
                if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

                    // 削除ボタン
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    if (ImGui::Button("削除", ImVec2(60, 25))) {
                        RemovePointLight(i);
                        ImGui::PopStyleColor();
                        ImGui::PopID();
                        break;
                    }
                    ImGui::PopStyleColor();

                    ImGui::Spacing();

                    if (ImGui::BeginChild(("PointLight" + std::to_string(i)).c_str(), ImVec2(0, 200), true)) {

                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, 150);

                        // アクティブ状態
                        ImGui::Text("有効");
                        ImGui::NextColumn();
                        bool active = pointLights_[i].active;
                        if (ImGui::Checkbox("##active", &active)) {
                            pointLights_[i].active = active;
                        }
                        ImGui::NextColumn();

                        // 位置
                        ImGui::Text("位置");
                        ImGui::NextColumn();
                        ImGui::DragFloat3("##position", &pointLights_[i].position.x, 0.1f);
                        ImGui::NextColumn();

                        // 色
                        ImGui::Text("色");
                        ImGui::NextColumn();
                        ImGui::ColorEdit3("##color", &pointLights_[i].color.x);
                        ImGui::NextColumn();

                        // 輝度
                        ImGui::Text("輝度");
                        ImGui::NextColumn();
                        ImGui::DragFloat("##intensity", &pointLights_[i].intensity, 0.01f, 0.0f, 10.0f);
                        ImGui::NextColumn();

                        // 半径
                        ImGui::Text("半径");
                        ImGui::NextColumn();
                        ImGui::DragFloat("##radius", &pointLights_[i].radius, 0.1f, 0.1f, 100.0f);
                        ImGui::NextColumn();

                        // 減衰率
                        ImGui::Text("減衰率");
                        ImGui::NextColumn();
                        ImGui::DragFloat("##decay", &pointLights_[i].decay, 0.1f, 0.0f, 5.0f);
                        ImGui::NextColumn();

                        // ライティングタイプ
                        ImGui::Text("ライティング");
                        ImGui::NextColumn();
                        const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};
                        int selectedType = pointLights_[i].BlinnPhong ? 1 : 0;
                        if (ImGui::Combo("##lighting", &selectedType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                            pointLights_[i].HalfLambert = (selectedType == 0) ? 1 : 0;
                            pointLights_[i].BlinnPhong = (selectedType == 1) ? 1 : 0;
                        }

                        ImGui::Columns(1);
                    }
                    ImGui::EndChild();
                }

                ImGui::PopID();
            }

            ImGui::Spacing();

            ImGui::EndTabItem();
        }

        // スポットライトタブ
        if (ImGui::BeginTabItem("スポットライト")) {
            ImGui::Spacing();

            // 追加・削除ボタン
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button("スポットライトを追加") && spotLights_.size() < MAX_SPOT_LIGHTS) {
                AddSpotLight();
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::Text("(%d/%d)", static_cast<int>(spotLights_.size()), MAX_SPOT_LIGHTS);

            ImGui::Separator();
            ImGui::Spacing();

            // 各スポットライトの設定
            for (int i = 0; i < static_cast<int>(spotLights_.size()); ++i) {
                ImGui::PushID(i);

                std::string headerLabel = std::format("スポットライト #{}", i + 1);
                if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

                    // 削除ボタン
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    if (ImGui::Button("削除", ImVec2(60, 25))) {
                        RemoveSpotLight(i);
                        ImGui::PopStyleColor();
                        ImGui::PopID();
                        break;
                    }
                    ImGui::PopStyleColor();

                    ImGui::Spacing();

                    if (ImGui::BeginChild(("SpotLight" + std::to_string(i)).c_str(), ImVec2(0, 280), true)) {

                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, 150);

                        // アクティブ状態
                        ImGui::Text("有効");
                        ImGui::NextColumn();
                        bool active = spotLights_[i].active;
                        if (ImGui::Checkbox("##active", &active)) {
                            spotLights_[i].active = active;
                        }
                        ImGui::NextColumn();

                        // 位置
                        ImGui::Text("位置");
                        ImGui::NextColumn();
                        ImGui::DragFloat3("##position", &spotLights_[i].position.x, 0.1f);
                        ImGui::NextColumn();

                        // 方向
                        ImGui::Text("方向");
                        ImGui::NextColumn();
                        ImGui::DragFloat3("##direction", &spotLights_[i].direction.x, 0.1f);
                        spotLights_[i].direction = spotLights_[i].direction.Normalize();
                        ImGui::NextColumn();

                        // 色
                        ImGui::Text("色");
                        ImGui::NextColumn();
                        ImGui::ColorEdit3("##color", &spotLights_[i].color.x);
                        ImGui::NextColumn();

                        // 輝度
                        ImGui::Text("輝度");
                        ImGui::NextColumn();
                        ImGui::DragFloat("##intensity", &spotLights_[i].intensity, 0.01f, 0.0f, 10.0f);
                        ImGui::NextColumn();

                        // 距離
                        ImGui::Text("距離");
                        ImGui::NextColumn();
                        ImGui::DragFloat("##distance", &spotLights_[i].distance, 0.1f, 0.1f, 100.0f);
                        ImGui::NextColumn();

                        // 減衰率
                        ImGui::Text("減衰率");
                        ImGui::NextColumn();
                        ImGui::DragFloat("##decay", &spotLights_[i].decay, 0.1f, 0.0f, 5.0f);
                        ImGui::NextColumn();

                        // 余弦
                        ImGui::Text("余弦");
                        ImGui::NextColumn();
                        ImGui::DragFloat("##cosAngle", &spotLights_[i].cosAngle, 0.01f, -1.0f, 1.0f);
                        ImGui::NextColumn();

                        // ライティングタイプ
                        ImGui::Text("ライティング");
                        ImGui::NextColumn();
                        const char *lightingTypes[] = {"HalfLambert", "BlinnPhong"};
                        int selectedType = spotLights_[i].BlinnPhong ? 1 : 0;
                        if (ImGui::Combo("##lighting", &selectedType, lightingTypes, IM_ARRAYSIZE(lightingTypes))) {
                            spotLights_[i].HalfLambert = (selectedType == 0) ? 1 : 0;
                            spotLights_[i].BlinnPhong = (selectedType == 1) ? 1 : 0;
                        }

                        ImGui::Columns(1);
                    }
                    ImGui::EndChild();
                }

                ImGui::PopID();
            }

            ImGui::Spacing();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    // セーブ・ロード機能を追加
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("セーブ・ロード");
    ImGui::Spacing();

    // ファイル名入力
    static char saveFileName[256] = "DefaultLightSetting";
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("ファイル名", saveFileName, sizeof(saveFileName));

    ImGui::SameLine();

    // セーブボタン
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
    if (ImGui::Button("セーブ", ImVec2(80, 25))) {
        SaveLightData(std::string(saveFileName));
        saveMessage_ = std::format("「{}」にセーブしました！", saveFileName);
        saveMessageTimer_ = 180; // 3秒間表示（60FPSの場合）
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // ロードボタン
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.8f, 0.2f, 1.0f));
    if (ImGui::Button("ロード", ImVec2(80, 25))) {
        LoadLightData(std::string(saveFileName));
        saveMessage_ = std::format("「{}」からロードしました！", saveFileName);
        saveMessageTimer_ = 180;
    }
    ImGui::PopStyleColor();

    // セーブ・ロードメッセージ表示
    if (saveMessageTimer_ > 0) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::Text("%s", saveMessage_.c_str());
        ImGui::PopStyleColor();
        saveMessageTimer_--;
    }

    // スタイルを元に戻す
    style.ChildRounding = originalRounding;
    style.FramePadding = ImVec2(originalPadding, style.FramePadding.y);
#endif // USE_IMGUI
}

void LightGroup::SaveLightData(const std::string &fileName) {
    auto dataHandler = std::make_unique<DataHandler>("LightGroup", fileName);

    // Directional Light
    dataHandler->Save<bool>("directional_active", isDirectionalLight);
    dataHandler->Save<Vector3>("directional_direction", directionalLightData->direction);
    dataHandler->Save<float>("directional_intensity", directionalLightData->intensity);
    dataHandler->Save<Vector4>("directional_color", directionalLightData->color);
    dataHandler->Save<int32_t>("directional_HalfLambert", directionalLightData->HalfLambert);
    dataHandler->Save<int32_t>("directional_BlinnPhong", directionalLightData->BlinnPhong);

    // Point Lights
    dataHandler->Save<int32_t>("pointLight_count", static_cast<int32_t>(pointLights_.size()));
    for (size_t i = 0; i < pointLights_.size(); ++i) {
        std::string prefix = std::format("pointLight_{:02d}_", i);
        dataHandler->Save<bool>(prefix + "active", pointLights_[i].active);
        dataHandler->Save<Vector4>(prefix + "color", pointLights_[i].color);
        dataHandler->Save<Vector3>(prefix + "position", pointLights_[i].position);
        dataHandler->Save<int32_t>(prefix + "HalfLambert", pointLights_[i].HalfLambert);
        dataHandler->Save<int32_t>(prefix + "BlinnPhong", pointLights_[i].BlinnPhong);
        dataHandler->Save<float>(prefix + "intensity", pointLights_[i].intensity);
        dataHandler->Save<float>(prefix + "radius", pointLights_[i].radius);
        dataHandler->Save<float>(prefix + "decay", pointLights_[i].decay);
    }

    // Spot Lights
    dataHandler->Save<int32_t>("spotLight_count", static_cast<int32_t>(spotLights_.size()));
    for (size_t i = 0; i < spotLights_.size(); ++i) {
        std::string prefix = std::format("spotLight_{:02d}_", i);
        dataHandler->Save<bool>(prefix + "active", spotLights_[i].active);
        dataHandler->Save<Vector4>(prefix + "color", spotLights_[i].color);
        dataHandler->Save<Vector3>(prefix + "position", spotLights_[i].position);
        dataHandler->Save<Vector3>(prefix + "direction", spotLights_[i].direction);
        dataHandler->Save<int32_t>(prefix + "HalfLambert", spotLights_[i].HalfLambert);
        dataHandler->Save<int32_t>(prefix + "BlinnPhong", spotLights_[i].BlinnPhong);
        dataHandler->Save<float>(prefix + "intensity", spotLights_[i].intensity);
        dataHandler->Save<float>(prefix + "distance", spotLights_[i].distance);
        dataHandler->Save<float>(prefix + "cosAngle", spotLights_[i].cosAngle);
        dataHandler->Save<float>(prefix + "decay", spotLights_[i].decay);
    }
}

void LightGroup::LoadLightData(const std::string &fileName) {
    auto dataHandler = std::make_unique<DataHandler>("LightGroup", fileName);

    // Directional Light
    isDirectionalLight = dataHandler->Load<bool>("directional_active", true);
    directionalLightData->color = dataHandler->Load<Vector4>("directional_color", {1.0f, 1.0f, 1.0f, 1.0f});
    directionalLightData->direction = dataHandler->Load<Vector3>("directional_direction", {0.0f, -1.0f, 0.0f});
    directionalLightData->HalfLambert = dataHandler->Load<int32_t>("directional_HalfLambert", false);
    directionalLightData->BlinnPhong = dataHandler->Load<int32_t>("directional_BlinnPhong", true);
    directionalLightData->intensity = dataHandler->Load<float>("directional_intensity", 1.0f);

    // Point Lights
    pointLights_.clear();
    int32_t pointLightCount = dataHandler->Load<int32_t>("pointLight_count", 0);
    for (int32_t i = 0; i < pointLightCount && i < MAX_POINT_LIGHTS; ++i) {
        std::string prefix = std::format("pointLight_{:02d}_", i);
        PointLight light = {};
        light.active = dataHandler->Load<bool>(prefix + "active", true);
        light.color = dataHandler->Load<Vector4>(prefix + "color", {1.0f, 1.0f, 1.0f, 1.0f});
        light.position = dataHandler->Load<Vector3>(prefix + "position", {0.0f, 2.0f, 0.0f});
        light.HalfLambert = dataHandler->Load<int32_t>(prefix + "HalfLambert", false);
        light.BlinnPhong = dataHandler->Load<int32_t>(prefix + "BlinnPhong", true);
        light.intensity = dataHandler->Load<float>(prefix + "intensity", 1.0f);
        light.radius = dataHandler->Load<float>(prefix + "radius", 5.0f);
        light.decay = dataHandler->Load<float>(prefix + "decay", 1.0f);
        pointLights_.push_back(light);
    }

    // Spot Lights
    spotLights_.clear();
    int32_t spotLightCount = dataHandler->Load<int32_t>("spotLight_count", 0);
    for (int32_t i = 0; i < spotLightCount && i < MAX_SPOT_LIGHTS; ++i) {
        std::string prefix = std::format("spotLight_{:02d}_", i);
        SpotLight light = {};
        light.active = dataHandler->Load<bool>(prefix + "active", true);
        light.color = dataHandler->Load<Vector4>(prefix + "color", {1.0f, 1.0f, 1.0f, 1.0f});
        light.position = dataHandler->Load<Vector3>(prefix + "position", {0.0f, 2.0f, 0.0f});
        light.direction = dataHandler->Load<Vector3>(prefix + "direction", {0.0f, -1.0f, 0.0f});
        light.HalfLambert = dataHandler->Load<int32_t>(prefix + "HalfLambert", false);
        light.BlinnPhong = dataHandler->Load<int32_t>(prefix + "BlinnPhong", true);
        light.intensity = dataHandler->Load<float>(prefix + "intensity", 1.0f);
        light.distance = dataHandler->Load<float>(prefix + "distance", 10.0f);
        light.cosAngle = dataHandler->Load<float>(prefix + "cosAngle", 0.7f);
        light.decay = dataHandler->Load<float>(prefix + "decay", 1.0f);
        spotLights_.push_back(light);
    }
}

void LightGroup::DrawLightVisualization() {
    if (!showLightVisualization_)
        return;

    DrawLine3D *drawLine = DrawLine3D::GetInstance();

    // 平行光源の可視化
    if (isDirectionalLight && directionalLightData->active) {
        Vector4 dirColor = {directionalLightData->color.x, directionalLightData->color.y, directionalLightData->color.z, 0.8f};

        // 複数の平行線で方向を表示
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                Vector3 startPos = {i * 5.0f, 20.0f, j * 5.0f};
                Vector3 endPos = startPos + directionalLightData->direction * 15.0f;
                drawLine->SetPoints(startPos, endPos, dirColor);
            }
        }
    }

    // ポイントライトの可視化
    for (size_t i = 0; i < pointLights_.size(); ++i) {
        if (!pointLights_[i].active)
            continue;

        const PointLight &light = pointLights_[i];
        Vector4 lightColor = {light.color.x, light.color.y, light.color.z, 0.8f};

        // ライト位置に球体を描画
        drawLine->DrawSphere(light.position, lightColor, 0.3f, 8);

        // 光の範囲を表示（ワイヤーフレーム球体）
        drawLine->DrawSphere(light.position, {lightColor.x, lightColor.y, lightColor.z, 0.3f}, light.radius, 16);

        // 放射線を描画（8方向）
        for (int dir = 0; dir < 8; dir++) {
            float angle = (float)dir * (3.14159f * 2.0f) / 8.0f;
            Vector3 rayDirection = {cosf(angle), 0.0f, sinf(angle)};
            Vector3 rayEnd = light.position + rayDirection * light.radius;
            drawLine->SetPoints(light.position, rayEnd, {lightColor.x, lightColor.y, lightColor.z, 0.5f});
        }
    }

    // スポットライトの可視化
    for (size_t i = 0; i < spotLights_.size(); ++i) {
        if (!spotLights_[i].active)
            continue;

        const SpotLight &light = spotLights_[i];
        Vector4 lightColor = {light.color.x, light.color.y, light.color.z, 0.8f};

        // ライト位置に小さな球体
        drawLine->DrawSphere(light.position, lightColor, 0.3f, 8);

        // スポットライトの方向線（中央）
        Vector3 centerRay = light.position + light.direction * light.distance;
        drawLine->SetPoints(light.position, centerRay, lightColor);

        // スポットライトのコーン形状を描画
        float coneAngle = acosf(light.cosAngle); // cosから角度を計算
        float coneRadius = light.distance * tanf(coneAngle);

        // コーンの外周を8本の線で表現
        for (int edge = 0; edge < 8; edge++) {
            float angle = (float)edge * (3.14159f * 2.0f) / 8.0f;

            // 方向ベクトルに垂直な2つのベクトルを計算
            Vector3 right;
            if (abs(light.direction.y) < 0.9f) {
                right = Vector3(0, 1, 0).Cross(light.direction).Normalize();
            } else {
                right = Vector3(1, 0, 0).Cross(light.direction).Normalize();
            }
            Vector3 up = light.direction.Cross(right).Normalize();

            // コーンの端点を計算
            Vector3 coneOffset = (right * cosf(angle) + up * sinf(angle)) * coneRadius;
            Vector3 coneEnd = centerRay + coneOffset;

            // ライト位置からコーンの端点への線
            drawLine->SetPoints(light.position, coneEnd, {lightColor.x, lightColor.y, lightColor.z, 0.6f});

            // コーンの円周
            int nextEdge = (edge + 1) % 8;
            float nextAngle = (float)nextEdge * (3.14159f * 2.0f) / 8.0f;
            Vector3 nextConeOffset = (right * cosf(nextAngle) + up * sinf(nextAngle)) * coneRadius;
            Vector3 nextConeEnd = centerRay + nextConeOffset;
            drawLine->SetPoints(coneEnd, nextConeEnd, {lightColor.x, lightColor.y, lightColor.z, 0.4f});
        }
    }
}