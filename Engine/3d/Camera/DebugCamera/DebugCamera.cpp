#include "DebugCamera.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "Mymath.h"
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG
#include "algorithm"

void DebugCamera::Initialize(ViewProjection *viewProjection) {
    viewProjection_ = viewProjection;
    translation_ = viewProjection->translation_;
    isUseQuaternion_ = viewProjection->isUseQuaternion_;
    eulerRotation_ = viewProjection->eulerRotation_;
    quateRotation_ = viewProjection->quateRotation_;
    matRot_ = MakeIdentity4x4();
    isActive_ = false;
    lockCamera_ = false;
    mouseSensitivity = 0.003f;
    moveZspeed = 0.005f;
    mouse = {0.0f, 0.0f};
}

void DebugCamera::Update() {
    if (isActive_) {
        if (!lockCamera_) {
            CameraMove(eulerRotation_, translation_, mouse); // rotation_をeulerRotation_に変更
        }

        Matrix4x4 cameraMatrix;

        if (isUseQuaternion_) {
            // クォータニオン使用時
            cameraMatrix = MakeAffineMatrix(
                {1.0f, 1.0f, 1.0f},
                quateRotation_,
                translation_);
        } else {
            // オイラー角使用時
            cameraMatrix = MakeAffineMatrix(
                {1.0f, 1.0f, 1.0f},
                eulerRotation_,
                translation_);
        }

        // ビュー・プロジェクション行列の設定
        viewProjection_->matWorld_ = cameraMatrix;
        viewProjection_->matView_ = Inverse(cameraMatrix);
        viewProjection_->translation_ = translation_;
        viewProjection_->eulerRotation_ = eulerRotation_;
        viewProjection_->quateRotation_ = quateRotation_;
        viewProjection_->isUseQuaternion_ = isUseQuaternion_;
        viewProjection_->matProjection_ = MakePerspectiveFovMatrix(
            45.0f * std::numbers::pi_v<float> / 180.0f,
            float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
            0.1f, 1000.0f);
    } else {
        viewProjection_->UpdateMatrix();
    }
}

void DebugCamera::CameraMove(Vector3 &cameraRotate, Vector3 &cameraTranslate, Vector2 &clickPosition) {
    // 各方向ベクトル（現在の回転に基づいて計算）
    Matrix4x4 matRot;
    if (isUseQuaternion_) {
        matRot = QuaternionToMatrix4x4(quateRotation_);
    } else {
        matRot = MakeRotateXMatrix(eulerRotation_.x) * MakeRotateYMatrix(eulerRotation_.y);
    }

    Vector3 forward = TransformNormal({0.0f, 0.0f, -2.0f}, matRot);
    Vector3 right = TransformNormal({2.0f, 0.0f, 0.0f}, matRot);
    Vector3 up = {0.0f, 2.0f, 0.0f};

    // ---------- キーボードによるカメラ移動 ----------
    if (useKey_) {
        // ダッシュ倍率判定
        bool isDashing = Input::GetInstance()->PushKey(DIK_LCONTROL);
        float speed = moveZspeed * 10.0f * (isDashing ? 5.0f : 1.0f);

        // 移動ベクトル初期化
        Vector3 move = {0, 0, 0};
        if (Input::GetInstance()->PushKey(DIK_W))
            move -= forward;
        if (Input::GetInstance()->PushKey(DIK_S))
            move += forward;
        if (Input::GetInstance()->PushKey(DIK_D))
            move += right;
        if (Input::GetInstance()->PushKey(DIK_A))
            move -= right;
        if (Input::GetInstance()->PushKey(DIK_SPACE))
            move += up;
        if (Input::GetInstance()->PushKey(DIK_LSHIFT))
            move -= up;

        // 反映
        translation_ += move * speed;
    }

    // ---------- マウスによるカメラ移動 ----------
    if (useMouse_) {
        // ホイールクリックによるXY移動
        if (Input::GetInstance()->IsPressMouse(2)) {
            Vector2 currentMousePos = Input::GetInstance()->GetMousePos();
            float deltaX = static_cast<float>(currentMousePos.x - clickPosition.x);
            float deltaY = static_cast<float>(currentMousePos.y - clickPosition.y);

            // X方向（右）とY方向（上）にカメラを平行移動
            translation_ -= right * deltaX * mouseSensitivity;
            translation_ += up * deltaY * mouseSensitivity;

            // マウス位置更新
            clickPosition = currentMousePos;
        }

        // ホイール回転でカメラの前後移動（Z軸）
        int wheel = Input::GetInstance()->GetWheel();
        if (wheel != 0) {
            translation_ -= forward * static_cast<float>(wheel) * mouseSensitivity;
        }
    }

    // ---------- マウス右クリックによる視点回転 ----------
    if (Input::GetInstance()->IsPressMouse(1)) {
        Vector2 currentMousePos = Input::GetInstance()->GetMousePos();
        float deltaX = static_cast<float>(currentMousePos.x - clickPosition.x);
        float deltaY = static_cast<float>(currentMousePos.y - clickPosition.y);

        if (isUseQuaternion_) {
            // クォータニオンでの回転処理
            Quaternion yawRotation = Quaternion::FromAxisAngle({0, 1, 0}, deltaX * mouseSensitivity);
            Quaternion pitchRotation = Quaternion::FromAxisAngle({1, 0, 0}, deltaY * mouseSensitivity);

            quateRotation_ = yawRotation * pitchRotation * quateRotation_;
            quateRotation_ = quateRotation_.Normalize();

            // 参考用にオイラー角も更新
            eulerRotation_ = quateRotation_.ToEulerAngles();
        } else {
            // オイラー角での回転処理
            cameraRotate.y += deltaX * mouseSensitivity;
            cameraRotate.x += deltaY * mouseSensitivity;

            // 上下反転制限
            const float pi_2 = std::numbers::pi_v<float> / 2.0f - 0.01f;
            cameraRotate.x = std::clamp(cameraRotate.x, -pi_2, pi_2);

            // 参考用にクォータニオンも更新
            quateRotation_ = Quaternion::FromEulerAngles(eulerRotation_);
        }

        clickPosition = currentMousePos;
    } else if (!Input::GetInstance()->IsPressMouse(2)) {
        clickPosition = Input::GetInstance()->GetMousePos();
    }
}

void DebugCamera::imgui() {
#ifdef _DEBUG
    // カメラ有効化チェックボックス（メインコントロール）
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.3f, 0.8f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::Checkbox("カメラ使用", &isActive_);
    ImGui::PopStyleColor(2);

    if (!isActive_) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        ImGui::TextDisabled("カメラは無効です");
        ImGui::PopStyleVar();
    }

    // カメラがアクティブな場合のみ設定を表示
    if (isActive_) {
        ImGui::Separator();

        // === 位置・回転設定 ===
        if (ImGui::CollapsingHeader("位置・回転設定", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.3f, 0.3f, 0.2f));
            ImGui::DragFloat3("位置", &translation_.x, 0.01f, -1000.0f, 1000.0f, "%.2f");
            ImGui::PopStyleColor();

            // 回転モード切り替え
            ImGui::Text("回転モード: %s", isUseQuaternion_ ? "クォータニオン" : "オイラー角");
            ImGui::Checkbox("クォータニオンを使用", &isUseQuaternion_);

            ImGui::Spacing();

            // 回転調整
            if (isUseQuaternion_) {
                ImGui::Text("クォータニオン回転");
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.8f, 0.3f, 0.2f));
                ImGui::DragFloat4("##quaternion", &quateRotation_.x, 0.01f, -1.0f, 1.0f, "%.3f");
                ImGui::PopStyleColor();

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
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.8f, 0.3f, 0.2f));
                if (ImGui::DragFloat3("##euler", &eulerDegrees.x, 1.0f, -360.0f, 360.0f, "%.1f°")) {
                    eulerRotation_ = {
                        eulerDegrees.x * std::numbers::pi_v<float> / 180.0f,
                        eulerDegrees.y * std::numbers::pi_v<float> / 180.0f,
                        eulerDegrees.z * std::numbers::pi_v<float> / 180.0f};
                }
                ImGui::PopStyleColor();

                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "クォータニオン (参考): %.3f, %.3f, %.3f, %.3f",
                                   quateRotation_.x, quateRotation_.y, quateRotation_.z, quateRotation_.w);
            }

            // リセットボタン
            if (ImGui::Button("位置リセット", ImVec2(-1, 0))) {
                translation_ = {0.0f, 0.0f, -50.0f};
            }
            if (ImGui::Button("回転リセット", ImVec2(-1, 0))) {
                eulerRotation_ = {0.0f, 0.0f, 0.0f};
                quateRotation_ = Quaternion::IdentityQuaternion();
            }
        }

        // === 移動設定 ===
        if (ImGui::CollapsingHeader("移動設定", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.6f, 0.2f, 0.2f));
            ImGui::DragFloat("カメラスピード", &moveZspeed, 0.001f, 0.001f, 1.0f, "%.3f");
            ImGui::DragFloat("感度", &mouseSensitivity, 0.001f, 0.001f, 0.1f, "%.3f");
            ImGui::PopStyleColor();

            if (ImGui::Button("スピードリセット", ImVec2(-1, 0))) {
                mouseSensitivity = 0.003f;
                moveZspeed = 0.005f;
            }
        }

        // === 制御設定 ===
        if (ImGui::CollapsingHeader("制御設定", ImGuiTreeNodeFlags_DefaultOpen)) {
            // カメラロック
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.2f, 0.8f, 0.3f));
            ImGui::Checkbox("カメラ固定", &lockCamera_);
            ImGui::PopStyleColor();

            ImGui::Separator();
            ImGui::Text("入力方法:");
            ImGui::Indent();

            // 排他制御付きラジオボタン
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.2f, 0.8f, 0.9f, 1.0f));
            if (ImGui::RadioButton("キーボード制御", useKey_ && !useMouse_)) {
                useKey_ = true;
                useMouse_ = false;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("マウス制御", useMouse_ && !useKey_)) {
                useMouse_ = true;
                useKey_ = false;
            }
            ImGui::PopStyleColor();

            // 両方オフの場合の警告
            if (!useKey_ && !useMouse_) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
                ImGui::TextWrapped("警告: 入力方法が選択されていません");
                ImGui::PopStyleColor();
            }

            ImGui::Unindent();
        }

        // === ステータス情報 ===
        if (ImGui::CollapsingHeader("ステータス情報")) {
            ImGui::BeginTable("StatusTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
            ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("カメラ状態");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", isActive_ ? "有効" : "無効");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("位置");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("(%.2f, %.2f, %.2f)", translation_.x, translation_.y, translation_.z);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("入力方法");
            ImGui::TableSetColumnIndex(1);
            if (useKey_)
                ImGui::Text("キーボード");
            else if (useMouse_)
                ImGui::Text("マウス");
            else
                ImGui::Text("なし");

            ImGui::EndTable();
        }

        // === 操作説明 ===
        if (ImGui::CollapsingHeader("操作説明")) {
            if (useMouse_) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.9f, 1.0f));
                ImGui::Text("マウス制御:");
                ImGui::PopStyleColor();
                ImGui::Indent();
                ImGui::BulletText("マウスホイール: Z位置を移動");
                ImGui::BulletText("ホイールドラッグ: XY位置を移動");
                ImGui::BulletText("右クリックドラッグ: カメラ回転");
                ImGui::Unindent();
            } else if (useKey_) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
                ImGui::Text("キーボード制御:");
                ImGui::PopStyleColor();
                ImGui::Indent();
                ImGui::BulletText("WASD: XZ位置を移動");
                ImGui::BulletText("Space,Shift: XZ位置を移動");
                ImGui::BulletText("Ctrl + WASD: 高速移動");
                ImGui::BulletText("右クリックドラッグ: カメラ回転");
                ImGui::Unindent();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
                ImGui::Text("入力方法が選択されていません");
                ImGui::PopStyleColor();
            }
        }
    }
#endif // _DEBUG
}