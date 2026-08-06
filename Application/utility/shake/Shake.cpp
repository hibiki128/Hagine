#include "Shake.h"
#include <camera/CameraManager.h>
#include "utility/debug/imgui/ImGuiNotification.h"
#include <filesystem>
#include <MyMath.h>
#include <random>

using namespace Hagine;
void Shake::Initialize(std::string jsonName)
{
    // 設定ファイルが指定されていれば読み込む
    if (!jsonName.empty())
    {
        LoadSettings(jsonName);
    }
}

void Shake::Update()
{
    // 揺らす対象は「今描画に使われているカメラ」。切り替わっても追従できるよう毎フレーム引く。
    Camera *pCamera = CameraManager::GetInstance()->GetActive();
    if (!pCamera)
    {
        return;
    }

    // 揺れ中でなければ、前フレームのずれを消して処理しない
    if (!isShaking_)
    {
        return;
    }

    // 指定間隔で揺れの値を作り直す（間隔外は直前の値を維持する）
    if (currentFrame_ % shakeInterval_ == 0 && currentFrame_ < shakeDuration_)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distX(shakeMin_.x, shakeMax_.x);
        std::uniform_real_distribution<float> distY(shakeMin_.y, shakeMax_.y);
        std::uniform_real_distribution<float> distRot(rotationShakeMin_, rotationShakeMax_);

        currentOffset_ = {distX(gen), distY(gen), 0.0f};
        currentRotationOffset_ = distRot(gen);
    }

    // カメラへ「今フレームのずれ」として渡す（カメラの位置・向きは動かさない）
    pCamera->SetExternalOffset(currentOffset_, currentRotationOffset_);

    // フレーム経過処理
    currentFrame_++;
    if (currentFrame_ >= shakeDuration_)
    {
        isShaking_ = false;
        currentOffset_ = {0.0f, 0.0f, 0.0f};
        currentRotationOffset_ = 0.0f;
        pCamera->SetExternalOffset(currentOffset_, currentRotationOffset_); // ずれを元に戻す
    }
}

void Shake::StartShake()
{
    isShaking_ = true;
    currentFrame_ = 0;
}

void Shake::LoadSettings(std::string jsonName)
{
    dataHandler_ = std::make_unique<DataHandler>("Shake", jsonName);

    // JSONから各パラメータをロード
    shakeMin_ = dataHandler_->Load<Vector2>("shakeMin", {0, 0});
    shakeMax_ = dataHandler_->Load<Vector2>("shakeMax", {0, 0});
    rotationShakeMin_ = dataHandler_->Load<float>("rotationShakeMin", 0);
    rotationShakeMax_ = dataHandler_->Load<float>("rotationShakeMax", 0);
    shakeInterval_ = dataHandler_->Load<int>("shakeInterval", 0);
    shakeDuration_ = dataHandler_->Load<int>("shakeDuration", 0);

    // 設定ファイルが無い・壊れている場合の既定値は0で、そのままだと
    // Update() の剰余算がゼロ除算になるため最小値を保証する
    if (shakeInterval_ < 1)
    {
        shakeInterval_ = 1;
    }
    ImGuiNotification::Post("シェイク設定を読み込みました: " + jsonName, {0.2f, 0.8f, 0.8f, 1.0f});
}

void Shake::SaveSettings(std::string jsonName)
{
    dataHandler_ = std::make_unique<DataHandler>("Shake", jsonName);

    // 現在のパラメータをJSONに保存
    dataHandler_->Save("shakeMin", shakeMin_);
    dataHandler_->Save("shakeMax", shakeMax_);
    dataHandler_->Save("rotationShakeMin", rotationShakeMin_);
    dataHandler_->Save("rotationShakeMax", rotationShakeMax_);
    dataHandler_->Save("shakeInterval", shakeInterval_);
    dataHandler_->Save("shakeDuration", shakeDuration_);
    ImGuiNotification::Post("シェイク設定を保存しました: " + jsonName, {0.2f, 0.8f, 0.2f, 1.0f});
}

void Shake::DrawImGui()
{
#ifdef _DEBUG
    if (ImGui::Begin("シェイク設定"))
    {
        ImGui::DragFloat2("揺れの最小値", &shakeMin_.x, 0.01f);
        ImGui::DragFloat2("揺れの最大値", &shakeMax_.x, 0.01f);
        ImGui::DragFloat("回転の最小値", &rotationShakeMin_, 0.01f);
        ImGui::DragFloat("回転の最大値", &rotationShakeMax_, 0.01f);
        ImGui::DragInt("揺れの間隔 (フレーム)", &shakeInterval_, 1, 1, 10);
        ImGui::DragInt("揺れの持続時間 (フレーム)", &shakeDuration_, 1, 1, 300);

        static char saveNameBuffer[256] = "";
        ImGui::InputText("セーブ名", saveNameBuffer, sizeof(saveNameBuffer));

        if (ImGui::Button("セーブ"))
        {
            std::string saveName = saveNameBuffer;
            if (!saveName.empty())
            {
                SaveSettings(saveName);
            }
            else
            {
                ImGuiNotification::Post("セーブ名を入力してください", {1.0f, 0.2f, 0.2f, 1.0f});
            }
        }

        if (ImGui::Button("シェイク開始"))
        {
            StartShake();
        }
    }
    ImGui::End();
#endif
}