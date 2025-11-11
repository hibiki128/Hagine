#define NOMINMAX
#include "ViewProjection.h"
#include "Data/DataHandler.h"
#include "Frame.h"
#include "cmath"
#include "myMath.h"
#include <type/Vector2.h>

void ViewProjection::Initialize(std::string jsonFile) {

    matView_ = MakeIdentity4x4();
    matProjection_ = MakeIdentity4x4();
    matWorld_ = MakeIdentity4x4();

    dxCommon_ = DirectXCommon::GetInstance();

    CreateConstBuffer();
    Map();
    UpdateMatrix();

    Load(jsonFile);
}

void ViewProjection::CreateConstBuffer() {
    const UINT bufferSize = sizeof(ConstBufferDataViewProjection);
    constBuffer_ = dxCommon_->CreateBufferResource(bufferSize);
}

void ViewProjection::Map() {
    HRESULT hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&constMap));
    if (FAILED(hr)) {
        // エラーハンドリング
    }
}

void ViewProjection::UpdateMatrix() {
    // イージング処理
    if (isEasing_) {
        easingTime_ += Frame::DeltaTime(); // 60FPS想定でフレーム時間を加算

        if (easingTime_ >= easingDuration_) {
            // イージング完了
            translation_ = targetTranslation_;
            eulerRotation_ = targetEulerRotation_;
            quateRotation_ = targetQuaternionRotation_;
            isEasing_ = false;
        } else {
            // イージング中の補間
            translation_ = ApplyEasing(currentEasingType_, startTranslation_, targetTranslation_, easingTime_, easingDuration_);
            eulerRotation_ = ApplyEasing(currentEasingType_, startEulerRotation_, targetEulerRotation_, easingTime_, easingDuration_);
            quateRotation_ = ApplyEasing(currentEasingType_, startQuaternionRotation_, targetQuaternionRotation_, easingTime_, easingDuration_);
        }
    }

    UpdateViewMatrix();
    UpdateProjectionMatrix();
    TransferMatrix();
}

void ViewProjection::TransferMatrix() {
    if (constMap) {
        constMap->view = matView_;
        constMap->projection = matProjection_;
        constMap->cameraPos = translation_;
    }
}

void ViewProjection::UpdateViewMatrix() {
    Matrix4x4 worldMatrix;

    if (isUseQuaternion_) {
        // クォータニオンから回転行列を作成
        worldMatrix = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, quateRotation_, translation_);
    } else {
        // オイラー角から回転行列を作成
        worldMatrix = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, eulerRotation_, translation_);
    }

    // matWorld_を更新
    matWorld_ = worldMatrix;

    // ビュー行列はワールド行列の逆行列
    matView_ = Inverse(worldMatrix);
}

void ViewProjection::UpdateProjectionMatrix() {
    matProjection_ = MakePerspectiveFovMatrix(fovAngleY, aspectRatio, nearZ, farZ);
}

void ViewProjection::EaseCameraMove(EasingType easeType, const std::string &jsonName, float duration) {
    if (isEasing_) {
        return; // 既にイージング中なら早期リターン
    }

    // 現在の値を開始値として保存
    startTranslation_ = translation_;
    startEulerRotation_ = eulerRotation_;
    startQuaternionRotation_ = quateRotation_;

    // JSONから目標値を読み込み
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("Camera", jsonName);
    targetTranslation_ = data->Load<Vector3>("translation", translation_);
    targetEulerRotation_ = data->Load<Vector3>("eulerRotation", eulerRotation_);
    targetQuaternionRotation_ = data->Load("quateRotation", quateRotation_);

    // イージング設定
    currentEasingType_ = easeType;
    easingDuration_ = duration;
    easingTime_ = 0.0f;
    isEasing_ = true;
}

void ViewProjection::ShowDebugInfo() {
#ifdef USE_IMGUI
    if (ImGui::Begin("カメラ設定 デバッグ")) {

        // 基本情報セクション
        if (ImGui::CollapsingHeader("基本設定", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Separator();

            // 回転モード
            ImGui::Text("回転モード: %s", isUseQuaternion_ ? "クォータニオン" : "オイラー角");
            ImGui::Checkbox("クォータニオンを使用", &isUseQuaternion_);

            ImGui::Spacing();

            // 位置調整
            ImGui::Text("カメラ位置");
            ImGui::DragFloat3("##translation", &translation_.x, 0.1f, -1000.0f, 1000.0f, "%.2f");

            ImGui::Spacing();

            // 回転調整
            if (isUseQuaternion_) {
                ImGui::Text("クォータニオン回転");
                ImGui::DragFloat4("##quaternion", &quateRotation_.x, 0.01f, -1.0f, 1.0f, "%.3f");

                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "オイラー角 (参考): %.1f°, %.1f°, %.1f°",
                                   eulerRotation_.x * 180.0f / std::numbers::pi_v<float>,
                                   eulerRotation_.y * 180.0f / std::numbers::pi_v<float>,
                                   eulerRotation_.z * 180.0f / std::numbers::pi_v<float>);
            } else {
                ImGui::Text("オイラー角回転 (度)");
                Vector3 eulerDegrees = {
                    eulerRotation_.x * 180.0f / std::numbers::pi_v<float>,
                    eulerRotation_.y * 180.0f / std::numbers::pi_v<float>,
                    eulerRotation_.z * 180.0f / std::numbers::pi_v<float>};
                if (ImGui::DragFloat3("##euler", &eulerDegrees.x, 1.0f, -360.0f, 360.0f, "%.1f°")) {
                    eulerRotation_ = {
                        eulerDegrees.x * std::numbers::pi_v<float> / 180.0f,
                        eulerDegrees.y * std::numbers::pi_v<float> / 180.0f,
                        eulerDegrees.z * std::numbers::pi_v<float> / 180.0f};
                }

                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "クォータニオン (参考): %.3f, %.3f, %.3f, %.3f",
                                   quateRotation_.x, quateRotation_.y, quateRotation_.z, quateRotation_.w);
            }
        }

        // カメラパラメータセクション
        if (ImGui::CollapsingHeader("カメラパラメータ")) {
            ImGui::Separator();

            float fovDegrees = fovAngleY * 180.0f / std::numbers::pi_v<float>;
            ImGui::Text("視野角");
            if (ImGui::SliderFloat("##fov", &fovDegrees, 10.0f, 170.0f, "%.1f°")) {
                fovAngleY = fovDegrees * std::numbers::pi_v<float> / 180.0f;
            }

            ImGui::Text("アスペクト比");
            ImGui::SliderFloat("##aspect", &aspectRatio, 0.1f, 5.0f, "%.3f");

            ImGui::Text("描画範囲");
            ImGui::DragFloat("Near##near", &nearZ, 0.01f, 0.001f, 10.0f, "%.3f");
            ImGui::DragFloat("Far##far", &farZ, 1.0f, 1.0f, 10000.0f, "%.1f");
        }

        // マトリックス表示セクション
        if (ImGui::CollapsingHeader("マトリックス情報")) {
            ImGui::Separator();

            // カラーテーマ
            ImVec4 headerColor = ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
            ImVec4 matrixColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);

            if (ImGui::TreeNode("ワールドマトリックス")) {
                ImGui::PushStyleColor(ImGuiCol_Text, matrixColor);
                for (int i = 0; i < 4; i++) {
                    ImGui::Text("[%d] %8.3f %8.3f %8.3f %8.3f", i,
                                matWorld_.m[i][0], matWorld_.m[i][1],
                                matWorld_.m[i][2], matWorld_.m[i][3]);
                }
                ImGui::PopStyleColor();
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("ビューマトリックス")) {
                ImGui::PushStyleColor(ImGuiCol_Text, matrixColor);
                for (int i = 0; i < 4; i++) {
                    ImGui::Text("[%d] %8.3f %8.3f %8.3f %8.3f", i,
                                matView_.m[i][0], matView_.m[i][1],
                                matView_.m[i][2], matView_.m[i][3]);
                }
                ImGui::PopStyleColor();
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("プロジェクションマトリックス")) {
                ImGui::PushStyleColor(ImGuiCol_Text, matrixColor);
                for (int i = 0; i < 4; i++) {
                    ImGui::Text("[%d] %8.3f %8.3f %8.3f %8.3f", i,
                                matProjection_.m[i][0], matProjection_.m[i][1],
                                matProjection_.m[i][2], matProjection_.m[i][3]);
                }
                ImGui::PopStyleColor();
                ImGui::TreePop();
            }
        }

        // 計算された情報
        if (ImGui::CollapsingHeader("計算情報", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Separator();

            Vector3 cameraWorldPos = {matWorld_.m[3][0], matWorld_.m[3][1], matWorld_.m[3][2]};
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "ワールド座標でのカメラ位置");
            ImGui::Text("   X: %.3f  Y: %.3f  Z: %.3f",
                        cameraWorldPos.x, cameraWorldPos.y, cameraWorldPos.z);

            float distanceFromOrigin = sqrt(cameraWorldPos.x * cameraWorldPos.x +
                                            cameraWorldPos.y * cameraWorldPos.y +
                                            cameraWorldPos.z * cameraWorldPos.z);
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "原点からの距離: %.3f", distanceFromOrigin);
        }
    }

    // 計算された情報セクションの後に追加
    if (ImGui::CollapsingHeader("データ保存・読み込み")) {
        ImGui::Separator();

        static char saveFileName[256] = "MyCamera";
        ImGui::InputText("ファイル名", saveFileName, sizeof(saveFileName));

        if (ImGui::Button("保存")) {
            Save(std::string(saveFileName));
        }

        ImGui::SameLine();

        if (ImGui::Button("読み込み")) {
            // Load関数も必要なら追加
            Load(std::string(saveFileName));
        }
    }

    if (ImGui::CollapsingHeader("カメライージング")) {
        ImGui::Separator();

        // イージングの状態表示
        if (isEasing_) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "イージング中...");
            ImGui::ProgressBar(easingTime_ / easingDuration_, ImVec2(0.0f, 0.0f));
            ImGui::Text("経過時間: %.2f / %.2f 秒", easingTime_, easingDuration_);
        } else {
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "待機中");
        }

        ImGui::Spacing();

        // イージングタイプ選択
        static int selectedEasingType = 0;
        const char *easingTypeNames[] = {
            "Linear",
            "InSine", "OutSine", "InOutSine",
            "InQuad", "OutQuad", "InOutQuad",
            "InCubic", "OutCubic", "InOutCubic",
            "InQuart", "OutQuart", "InOutQuart",
            "InQuint", "OutQuint", "InOutQuint",
            "InCirc", "OutCirc", "InOutCirc",
            "InExpo", "OutExpo", "InOutExpo",
            "InBack", "OutBack", "InOutBack",
            "InElastic", "OutElastic", "InOutElastic",
            "InBounce", "OutBounce", "InOutBounce"};

        ImGui::Text("イージングタイプ");
        ImGui::Combo("##easingType", &selectedEasingType, easingTypeNames,
                     sizeof(easingTypeNames) / sizeof(easingTypeNames[0]));

        // 継続時間設定
        static float duration = 2.0f;
        ImGui::Text("継続時間 (秒)");
        ImGui::SliderFloat("##duration", &duration, 0.1f, 10.0f, "%.1f秒");

        // JSONファイル名入力
        static char jsonFileName[256] = "CameraTarget";
        ImGui::Text("目標値JSONファイル名");
        ImGui::InputText("##targetJson", jsonFileName, sizeof(jsonFileName));

        ImGui::Spacing();

        // イージング実行ボタン
        if (ImGui::Button("イージング開始", ImVec2(150, 30))) {
            if (!isEasing_ && strlen(jsonFileName) > 0) {
                EasingType easeType = static_cast<EasingType>(selectedEasingType);
                EaseCameraMove(easeType, std::string(jsonFileName), duration);
            }
        }

        ImGui::SameLine();

        // 緊急停止ボタン
        if (ImGui::Button("停止", ImVec2(80, 30))) {
            if (isEasing_) {
                isEasing_ = false;
                easingTime_ = 0.0f;
            }
        }

        ImGui::Spacing();

        // 現在値と目標値の表示（イージング中のみ）
        if (isEasing_) {
            if (ImGui::TreeNode("イージング詳細情報")) {
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "開始値");
                ImGui::Text("位置: %.2f, %.2f, %.2f",
                            startTranslation_.x, startTranslation_.y, startTranslation_.z);
                ImGui::Text("回転: %.2f, %.2f, %.2f",
                            startEulerRotation_.x, startEulerRotation_.y, startEulerRotation_.z);

                ImGui::Spacing();

                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.7f, 1.0f), "目標値");
                ImGui::Text("位置: %.2f, %.2f, %.2f",
                            targetTranslation_.x, targetTranslation_.y, targetTranslation_.z);
                ImGui::Text("回転: %.2f, %.2f, %.2f",
                            targetEulerRotation_.x, targetEulerRotation_.y, targetEulerRotation_.z);

                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "現在値");
                ImGui::Text("位置: %.2f, %.2f, %.2f",
                            translation_.x, translation_.y, translation_.z);
                ImGui::Text("回転: %.2f, %.2f, %.2f",
                            eulerRotation_.x, eulerRotation_.y, eulerRotation_.z);

                ImGui::TreePop();
            }
        }

        ImGui::Spacing();

        // クイック設定ボタン群
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "クイック設定");

        if (ImGui::Button("現在位置を保存", ImVec2(120, 25))) {
            Save("CurrentCamera");
        }

        ImGui::SameLine();

        if (ImGui::Button("原点に戻る", ImVec2(80, 25))) {
            if (!isEasing_) {
                // 一時的にJSONファイルに原点位置を保存
                std::unique_ptr<DataHandler> tempData = std::make_unique<DataHandler>("Camera", "TempOrigin");
                tempData->Save("translation", Vector3{0.0f, 0.0f, -10.0f});
                tempData->Save("eulerRotation", Vector3{0.0f, 0.0f, 0.0f});
                tempData->Save("quateRotation", Quaternion::IdentityQuaternion());

                EasingType easeType = static_cast<EasingType>(selectedEasingType);
                EaseCameraMove(easeType, "TempOrigin", duration);
            }
        }

        // 使い方の説明
        if (ImGui::TreeNode("使い方")) {
            ImGui::TextWrapped("1. 目標位置をJSONファイルに保存しておく");
            ImGui::TextWrapped("2. イージングタイプと継続時間を選択");
            ImGui::TextWrapped("3. JSONファイル名を入力して「イージング開始」をクリック");
            ImGui::TextWrapped("4. 必要に応じて「停止」ボタンで中断可能");
            ImGui::TreePop();
        }
    }

    ImGui::End();
#endif // USE_IMGUI
}

void ViewProjection::Save(std::string jsonFile) {
    if (jsonFile.empty()) {
        return;
    }
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("Camera", jsonFile);
    data->Save("matView", matView_);
    data->Save("matProjection", matProjection_);
    data->Save("matWorld", matWorld_);
    data->Save("translation", translation_);
    data->Save("eulerRotation", eulerRotation_);
    data->Save("isUseQuaternion", isUseQuaternion_);
    data->Save("fov", fovAngleY);
    data->Save("nearZ", nearZ);
    data->Save("farZ", farZ);
    data->Save("aspectRatio", aspectRatio);
    data->Save("quateRotation", quateRotation_);
}

void ViewProjection::Load(std::string jsonFile) {
    if (jsonFile.empty()) {
        return;
    }
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("Camera", jsonFile);
    matView_ = data->Load("matView", MakeIdentity4x4());
    matProjection_ = data->Load("matProjection", MakeIdentity4x4());
    matWorld_ = data->Load("matWorld", MakeIdentity4x4());
    translation_ = data->Load<Vector3>("translation", {0.0f, 0.0f, -10.0f});
    eulerRotation_ = data->Load<Vector3>("eulerRotation", {0.0f, 0.0f, 0.0f});
    quateRotation_ = data->Load("quateRotation", Quaternion::IdentityQuaternion());
    isUseQuaternion_ = data->Load("isUseQuaternion", true);
    fovAngleY = data->Load("fov", fovAngleY);
    nearZ = data->Load("nearZ", nearZ);
    farZ = data->Load("farZ", farZ);
    aspectRatio = data->Load("aspectRatio", aspectRatio);
}
