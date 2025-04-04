#include "DebugCamera.h"
#include"DirectXCommon.h"
#include"Input.h"
#include"Mymath.h"
#ifdef _DEBUG
#include"imgui.h"
#endif // _DEBUG



void DebugCamera::Initialize(ViewProjection* viewProjection)
{
	viewProjection_ = viewProjection;
	translation_ = { 0.0f,0.0f,-50.0f };
	rotation_.y = 0.0f;
	matRot_ = MakeIdentity4x4();
}

void DebugCamera::Update()
{
	if (isActive_) {
		rotation_ = { 0.0f,0.0f,0.0f };
		Vector3 offset = translation_;
		if (useMouse) {
			CameraMove(rotation_, translation_, mouse);
		}
		/*追加分の回転行列を生成*/
		Matrix4x4 matRotDelta = MakeIdentity4x4();
		matRotDelta *= MakeRotateXMatrix(rotation_.x);
		matRotDelta *= MakeRotateYMatrix(rotation_.y);
		matRot_ = matRotDelta * matRot_;
		offset = TransformNormal(offset, matRot_);
		Matrix4x4 scaleMatrix = MakeScaleMatrix(Vector3(1.0f, 1.0f, 1.0f));
		rotateXYZMatrix = matRot_;
		Matrix4x4 translateMatrix = MakeTranslateMatrix(offset);
		Matrix4x4 cameraMatrix = (scaleMatrix * rotateXYZMatrix) * translateMatrix;
		viewProjection_->matWorld_ = cameraMatrix;
		viewProjection_->matView_ = Inverse(cameraMatrix);
		viewProjection_->matProjection_ = MakePerspectiveFovMatrix(45.0f * std::numbers::pi_v<float> / 180.0f,
			float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
			0.1f, 1000.0f);;
	}
}

void DebugCamera::imgui()
{
#ifdef _DEBUG
	if (ImGui::BeginTabBar("debugCamera")) {
		if (ImGui::BeginTabItem("debugCamera")) {
			ImGui::Checkbox("CameraActive", &isActive_);
			if (isActive_) {
				ImGui::DragFloat3("translation", &translation_.x, 0.01f);
				Vector3 rotate = GetEulerAnglesFromMatrix(matRot_);
				ImGui::DragFloat3("rotation", &rotate.x, 0.01f);
				ImGui::DragFloat("ZSpeed", &moveZspeed, 0.001f);
				ImGui::DragFloat("mouseSensitivity", &mouseSensitivity, 0.001f);
				if (ImGui::Button("Camera Reset")) {
					translation_ = { 0.0f,0.0f,-50.0f };
					matRot_ = MakeIdentity4x4();
				}
				if (ImGui::Button("Speed Reset")) {
					mouseSensitivity = 0.003f;
					moveZspeed = 0.005f;
				}
				ImGui::Checkbox("useMouse", &useMouse);
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
#endif // _DEBUG
}



void DebugCamera::CameraMove(Vector3& cameraRotate, Vector3& cameraTranslate, Vector2& clickPosition)
{
	// 各フラグ
	static bool isLeftClicked = false;
	static bool isWheelClicked = false;

	// 回転を考慮する
	Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(cameraRotate);
	Vector3 X = { 1.0f, 0.0f, 0.0f };
	Vector3 Y = { 0.0f, 1.0f, 0.0f };
	Vector3 Z = { 0.0f, 0.0f, -1.0f };

	Vector3 rotatedX = Transformation(X, rotationMatrix);
	Vector3 rotatedY = Transformation(Y, rotationMatrix);
	Vector3 rotatedZ = Transformation(Z, rotationMatrix);

	/// ========カメラ操作========
	// カメラの回転を更新する
	if (Input::GetInstance()->IsPressMouse(0) == 1) {
		if (!isLeftClicked) {
			// マウスがクリックされたときに現在のマウス位置を保存する
			clickPosition = Input::GetInstance()->GetMousePos();
			isLeftClicked = true;
		}
		else {
			// マウスがクリックされている間はカメラの回転を更新する
			Vector2 currentMousePos;
			currentMousePos = Input::GetInstance()->GetMousePos();

			float deltaX = static_cast<float>(currentMousePos.x - clickPosition.x);
			float deltaY = static_cast<float>(currentMousePos.y - clickPosition.y);

			cameraRotate.x += (deltaY * mouseSensitivity); // 5.0f;
			cameraRotate.y += (deltaX * mouseSensitivity); // 5.0f;


			// 現在のマウス位置を保存する
			clickPosition = currentMousePos;
		}
	}
	else {
		// マウスがクリックされていない場合はフラグをリセットする
		isLeftClicked = false;
	}

	// カメラの位置を更新する
	if (Input::GetInstance()->IsPressMouse(2) == 1) {
		if (!isWheelClicked) {
			// マウスがクリックされたときに現在のマウス位置を保存する
			clickPosition = Input::GetInstance()->GetMousePos();
			isWheelClicked = true;
		}
		else {
			// マウスがクリックされている間はカメラの位置を更新する
			Vector2 currentMousePos;
			currentMousePos = Input::GetInstance()->GetMousePos();

			float deltaX = static_cast<float>(currentMousePos.x - clickPosition.x);
			float deltaY = static_cast<float>(currentMousePos.y - clickPosition.y);

			cameraTranslate -= (rotatedX * deltaX * mouseSensitivity);
			cameraTranslate += (rotatedY * deltaY * mouseSensitivity);

			// 現在のマウス位置を保存する
			clickPosition = currentMousePos;
		}
	}
	else {
		// マウスがクリックされていない場合はフラグをリセットする
		isWheelClicked = false;
	}

	// マウスホイールの移動量を取得する
	int wheelDelta = -Input::GetInstance()->GetWheel();

	// マウスホイールの移動量に応じてカメラの移動を更新する
	cameraTranslate.z += rotatedZ.z * float(wheelDelta) * moveZspeed;
	/// =====================
}
