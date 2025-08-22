#include "Shake.h"
#include <filesystem>
#include <myMath.h>
#include <random>

void Shake::Initialize(ViewProjection *viewProjection, std::string jsonName) {
    viewProjection_ = viewProjection;
    if (!jsonName.empty()) {
        LoadSettings(jsonName);
    }
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

void Shake::LoadSettings(std::string jsonName) {
    dataHandler_ = std::make_unique<DataHandler>("Shake", jsonName);

    shakeMin_ = dataHandler_->Load<Vector2>("shakeMin", {0, 0});
    shakeMax_ = dataHandler_->Load<Vector2>("shakeMax", {0, 0});
    rotationShakeMin_ = dataHandler_->Load<float>("rotationShakeMin", 0);
    rotationShakeMax_ = dataHandler_->Load<float>("rotationShakeMax", 0);
    shakeInterval_ = dataHandler_->Load<int>("shakeInterval", 0);
    shakeDuration_ = dataHandler_->Load<int>("shakeDuration", 0);
}

void Shake::SaveSettings(std::string jsonName) {
    dataHandler_ = std::make_unique<DataHandler>("Shake", jsonName);

    dataHandler_->Save("shakeMin", shakeMin_);
    dataHandler_->Save("shakeMax", shakeMax_);
    dataHandler_->Save("rotationShakeMin", rotationShakeMin_);
    dataHandler_->Save("rotationShakeMax", rotationShakeMax_);
    dataHandler_->Save("shakeInterval", shakeInterval_);
    dataHandler_->Save("shakeDuration", shakeDuration_);
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

        // セーブ名入力用のバッファ（静的変数として保持）
        static char saveNameBuffer[256] = "";
        ImGui::InputText("セーブ名", saveNameBuffer, sizeof(saveNameBuffer));

        if (ImGui::Button("セーブ")) {
            std::string saveName = saveNameBuffer;
            if (!saveName.empty()) {
                SaveSettings(saveName);
                std::string message = std::format("Shake saved as: {}", saveName);
                MessageBoxA(nullptr, message.c_str(), "Effect", 0);
            } else {
                MessageBoxA(nullptr, "セーブ名を入力してください", "Error", 0);
            }
        }

        if (ImGui::Button("シェイク開始")) {
            StartShake();
        }
    }
    ImGui::End();
#endif
}