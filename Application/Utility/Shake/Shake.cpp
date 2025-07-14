#include "Shake.h"
#include <random>
#include <filesystem>
#include <myMath.h>


void Shake::Initialize(ViewProjection *viewProjection) {
    viewProjection_ = viewProjection;
    LoadSettings();
}

void Shake::Update() {
    if (!isShaking_)
        return;

    if (currentFrame_ % shakeInterval_ == 0 && currentFrame_ < shakeDuration_) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distX(shakeMin_.x, shakeMax_.x);
        std::uniform_real_distribution<float> distY(shakeMin_.y, shakeMax_.y);
        std::uniform_real_distribution<float> distRot(rotationShakeMin_, rotationShakeMax_);

        Vector3 shakeOffset = {distX(gen), distY(gen), 0.0f};
        float rotationOffset = distRot(gen);

        viewProjection_->matView_.m[3][0] += shakeOffset.x;
        viewProjection_->matView_.m[3][1] += shakeOffset.y;
        viewProjection_->matView_.m[3][2] += shakeOffset.z;
        viewProjection_->matView_ = MakeRotateXMatrix(rotationOffset) * viewProjection_->matView_;
    }

    currentFrame_++;
    if (currentFrame_ >= shakeDuration_) {
        isShaking_ = false;
    }
}

void Shake::StartShake() {
    isShaking_ = true;
    currentFrame_ = 0;
}

void Shake::LoadSettings() {
    std::ifstream file("resources/jsons/Shake/shake.json");
    if (!file)
        return;

    nlohmann::json json;
    file >> json;
    file.close();

    shakeMin_ = {json["shakeMinX"], json["shakeMinY"]};
    shakeMax_ = {json["shakeMaxX"], json["shakeMaxY"]};
    rotationShakeMin_ = json["rotationShakeMin"];
    rotationShakeMax_ = json["rotationShakeMax"];
    shakeInterval_ = json["shakeInterval"];
    shakeDuration_ = json["shakeDuration"];
}

void Shake::SaveSettings() {
    // フォルダが存在しなければ作成
    std::filesystem::create_directories("resources/jsons/Shake");

    nlohmann::json json;
    json["shakeMinX"] = shakeMin_.x;
    json["shakeMinY"] = shakeMin_.y;
    json["shakeMaxX"] = shakeMax_.x;
    json["shakeMaxY"] = shakeMax_.y;
    json["rotationShakeMin"] = rotationShakeMin_;
    json["rotationShakeMax"] = rotationShakeMax_;
    json["shakeInterval"] = shakeInterval_;
    json["shakeDuration"] = shakeDuration_;

    std::ofstream file("resources/jsons/Shake/shake.json");
    if (file) {
        file << json.dump(4);
        file.close();
    }
}

void Shake::imgui() {
#ifdef _DEBUG
    if (ImGui::Begin("シェイク設定")) {
        ImGui::DragFloat2("揺れの最小値", &shakeMin_.x, 0.01f);
        ImGui::DragFloat2("揺れの最大値", &shakeMax_.x, 0.01f);
        ImGui::DragFloat("回転の最小値", &rotationShakeMin_, 0.01f);
        ImGui::DragFloat("回転の最大値", &rotationShakeMax_, 0.01f);
        ImGui::DragInt("揺れの間隔 (フレーム)", &shakeInterval_, 1, 1, 10);
        ImGui::DragInt("揺れの持続時間 (フレーム)", &shakeDuration_, 1, 1, 300);

        if (ImGui::Button("セーブ")) {
            SaveSettings();
            std::string message = std::format("Shake saved.");
            MessageBoxA(nullptr, message.c_str(), "Effect", 0);
        }

        if (ImGui::Button("シェイク開始")) {
            StartShake();
        }
    }
    ImGui::End();
#endif
}