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
    translation_ = {0.0f, 0.0f, -50.0f};
    rotation_ = {0.0f, 0.0f, 0.0f};
    matRot_ = MakeIdentity4x4();
    isActive_ = false;
    lockCamera_ = true;
    mouseSensitivity = 0.003f;
    moveZspeed = 0.005f;
    mouse = {0.0f, 0.0f};
}

void DebugCamera::Update() {
    if (isActive_) {
        if (lockCamera_) {
            CameraMove(rotation_, translation_, mouse);
        }

        // 回転行列を更新せず、直接MakeAffineMatrixで渡す
        rotateXYZMatrix = MakeRotateXMatrix(rotation_.x) *
                          MakeRotateYMatrix(rotation_.y) *
                          MakeRotateZMatrix(rotation_.z); // デバッグ用に保持

        // カメラ行列の作成（回転はオイラー角ベースで）
        Matrix4x4 cameraMatrix = MakeAffineMatrix(
            {1.0f, 1.0f, 1.0f},
            rotation_,
            translation_);

        // ビュー・プロジェクション行列の設定
        viewProjection_->matWorld_ = cameraMatrix;
        viewProjection_->matView_ = Inverse(cameraMatrix);
        viewProjection_->matProjection_ = MakePerspectiveFovMatrix(
            45.0f * std::numbers::pi_v<float> / 180.0f,
            float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
            0.1f, 1000.0f);
    }
}

void DebugCamera::CameraMove(Vector3 &cameraRotate, Vector3 &cameraTranslate, Vector2 &clickPosition) {
    // 各方向ベクトル（回転行列適用後）
    Matrix4x4 matRot = MakeRotateXMatrix(rotation_.x) * MakeRotateYMatrix(rotation_.y);
    Vector3 forward = TransformNormal({0.0f, 0.0f, -2.0f}, matRot);
    Vector3 right = TransformNormal({2.0f, 0.0f, 0.0f}, matRot);
    Vector3 up = {0.0f, 2.0f, 0.0f}; // ワールド上方向は固定

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
        int wheel = Input::GetInstance()->GetWheel(); // 正:奥へ, 負:手前へ
        if (wheel != 0) {
            translation_ -= forward * static_cast<float>(wheel) * mouseSensitivity;
        }
    }

    // ---------- マウス右クリックによる視点回転 ----------
    if (Input::GetInstance()->IsPressMouse(1)) {
        Vector2 currentMousePos = Input::GetInstance()->GetMousePos();

        float deltaX = static_cast<float>(currentMousePos.x - clickPosition.x);
        float deltaY = static_cast<float>(currentMousePos.y - clickPosition.y);

        // 視点回転を加算（Yaw：Y軸、Pitch：X軸）
        cameraRotate.y += deltaX * mouseSensitivity;
        cameraRotate.x += deltaY * mouseSensitivity;

        // 上下反転制限
        const float pi_2 = std::numbers::pi_v<float> / 2.0f - 0.01f;
        cameraRotate.x = std::clamp(cameraRotate.x, -pi_2, pi_2);

        // マウス位置更新
        clickPosition = currentMousePos;
    } else if (!Input::GetInstance()->IsPressMouse(2)) {
        // 右クリックでもホイールクリックでもないときに初期化
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

            Vector3 rotate = GetEulerAnglesFromMatrix(matRot_);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.8f, 0.3f, 0.2f));
            if (ImGui::DragFloat3("回転", &rotate.x, 0.01f, -360.0f, 360.0f, "%.2f度")) {
                // 回転が変更された場合の処理をここに追加
            }
            ImGui::PopStyleColor();

            // リセットボタン
            if (ImGui::Button("位置リセット", ImVec2(-1, 0))) {
                translation_ = {0.0f, 0.0f, -50.0f};
            }
            if (ImGui::Button("回転リセット", ImVec2(-1, 0))) {
                matRot_ = MakeIdentity4x4();
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