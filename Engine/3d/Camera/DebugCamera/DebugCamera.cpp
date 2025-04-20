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
    if (ImGui::BeginTabBar("デバッグカメラ")) {
        if (ImGui::BeginTabItem("デバッグカメラ")) {
            ImGui::Checkbox("カメラ使用", &isActive_);
            if (isActive_) {
                ImGui::DragFloat3("位置", &translation_.x, 0.01f);
                Vector3 rotate = GetEulerAnglesFromMatrix(matRot_);
                ImGui::DragFloat3("回転", &rotate.x, 0.01f);
                ImGui::DragFloat("カメラスピード", &moveZspeed, 0.001f);
                ImGui::DragFloat("感度", &mouseSensitivity, 0.001f);
                if (ImGui::Button("位置リセット")) {
                    translation_ = {0.0f, 0.0f, -50.0f};
                }
                if (ImGui::Button("スピードリセット")) {
                    mouseSensitivity = 0.003f;
                    moveZspeed = 0.005f;
                }
                if (ImGui::Button("回転リセット")) {
                    matRot_ = MakeIdentity4x4();
                }
                ImGui::Checkbox("カメラ固定", &lockCamera_);

                // --- 排他制御付きチェックボックス ---
                bool useKeyTemp = useKey_;
                bool useMouseTemp = useMouse_;

                if (ImGui::Checkbox("キーボード使用", &useKeyTemp)) {
                    useKey_ = useKeyTemp;
                    useMouse_ = !useKeyTemp; // キーボードがtrueならマウスはfalse
                }

                if (ImGui::Checkbox("マウス使用", &useMouseTemp)) {
                    useMouse_ = useMouseTemp;
                    useKey_ = !useMouseTemp; // マウスがtrueならキーボードはfalse
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
#endif // _DEBUG
}