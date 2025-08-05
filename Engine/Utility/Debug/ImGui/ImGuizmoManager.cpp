#define NOMINMAX
#ifdef _DEBUG
#include "ImGuizmoManager.h"
#include "Input.h"
#include <Line/DrawLine3D.h>
#include <Object/Base/BaseObjectManager.h>
#include <Transform/WorldTransform.h>

ImGuizmoManager *ImGuizmoManager::instance = nullptr;

ImGuizmoManager *ImGuizmoManager::GetInstance() {
    if (instance == nullptr) {
        instance = new ImGuizmoManager();
    }
    return instance;
}

void ImGuizmoManager::Finalize() {
    transformMap.clear();
    selectedName.clear();
    delete instance;
    instance = nullptr;
}

void ImGuizmoManager::BeginFrame() {
    ImGuizmo::BeginFrame();
}

void ImGuizmoManager::SetViewProjection(ViewProjection *vp) {
    viewProjection = vp;
}

void ImGuizmoManager::AddTarget(const std::string &name, BaseObject *transform) {
    transformMap[name] = transform;

    // 初期選択が未設定の場合、最初に追加されたものを自動選択
    if (selectedName.empty()) {
        selectedName = name;
    }
}

BaseObject *ImGuizmoManager::GetSelectedTarget() {
    auto it = transformMap.find(selectedName);
    return (it != transformMap.end()) ? it->second : nullptr;
}

void ImGuizmoManager::imgui() {
    if (!viewProjection) {
        return;
    }
    BaseObject *selectedObject = GetSelectedTarget();

    // 操作モード選択
    if (ImGui::RadioButton("移動", currentOperation == ImGuizmo::TRANSLATE)) {
        currentOperation = ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("回転", currentOperation == ImGuizmo::ROTATE)) {
        currentOperation = ImGuizmo::ROTATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("スケール", currentOperation == ImGuizmo::SCALE)) {
        currentOperation = ImGuizmo::SCALE;
    }

    // 座標系選択
    if (ImGui::RadioButton("ローカル", currentMode == ImGuizmo::LOCAL)) {
        currentMode = ImGuizmo::LOCAL;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("ワールド", currentMode == ImGuizmo::WORLD)) {
        currentMode = ImGuizmo::WORLD;
    }

    ImGui::Separator();

    // オブジェクト選択コンボボックス
    if (ImGui::BeginCombo("選択オブジェクト", selectedName.empty() ? "なし" : selectedName.c_str())) {
        // "なし"オプションを追加
        bool isNoneSelected = selectedName.empty();
        if (ImGui::Selectable("なし", isNoneSelected)) {
            selectedName.clear();
        }
        if (isNoneSelected) {
            ImGui::SetItemDefaultFocus();
        }

        for (const auto &pair : transformMap) {
            bool isSelected = (selectedName == pair.first);
            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                selectedName = pair.first;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Spacing();

    // === オブジェクト詳細セクション ===
    if (selectedObject) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
        ImGui::Text("オブジェクト詳細");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ShowSelectedObjectImGui();

        ImGui::Spacing();
        ImGui::Spacing();

        // === 削除ボタン ===
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

        if (ImGui::Button("オブジェクトを削除", ImVec2(-1, 0))) {
            DeleteSelectedObject();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("選択中のオブジェクトを削除します");
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::Separator();
}

void ImGuizmoManager::Update(const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    if (!viewProjection) {
        return;
    }

    // ギズモ描画範囲の設定 (UIウィンドウが開いてなくても常に設定)
    ImGuizmo::SetRect(scenePosition.x, scenePosition.y, sceneSize.x, sceneSize.y);
    ImGuizmo::SetDrawlist();

    // マウスクリック判定による選択（ギズモ操作中でない場合のみ）
    if (!ImGuizmo::IsUsing()) {
        HandleMouseSelection(scenePosition, sceneSize);
    }

    // 選択中オブジェクトに対してギズモ操作
    BaseObject *selectedObject = GetSelectedTarget();
    if (selectedObject) {
        WorldTransform *transform = selectedObject->GetWorldTransform();
        if (transform) {
            DisplayGizmo(transform); // ImGuizmo::Manipulateを内部で呼ぶ
        }
    }
}

void ImGuizmoManager::ShowSelectedObjectImGui() {
    BaseObject *selectedObject = GetSelectedTarget();
    if (selectedObject && !selectedName.empty()) {
        selectedObject->ImGui();
    }
}

void ImGuizmoManager::DeleteSelectedObject() {
    if (selectedName.empty()) {
        return; // 何も選択されていない場合は何もしない
    }

    // BaseObjectManagerから削除
    BaseObjectManager::GetInstance()->RemoveObject(selectedName);

    // ImGuizmoManagerの管理からも削除
    auto it = transformMap.find(selectedName);
    if (it != transformMap.end()) {
        transformMap.erase(it);
    }

    // 選択をクリア
    selectedName.clear();

    // 他にオブジェクトがある場合は最初のものを選択
    if (!transformMap.empty()) {
        selectedName = transformMap.begin()->first;
    }
}

void ImGuizmoManager::HandleMouseSelection(const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isInScene = (mousePos.x >= scenePosition.x && mousePos.x <= scenePosition.x + sceneSize.x &&
                      mousePos.y >= scenePosition.y && mousePos.y <= scenePosition.y + sceneSize.y);

    if (!isInScene || !Input::IsTriggerMouse(0) || ImGuizmo::IsOver() || !viewProjection) {
        return;
    }

    // より大きな判定範囲でフォールバック判定も追加
    float minDistSq = std::numeric_limits<float>::max();
    std::string pickedName;
    bool foundRayHit = false;

    // まずレイキャスティングで試す
    Ray mouseRay = CreateMouseRay(mousePos, scenePosition, sceneSize);

    for (const auto &pair : transformMap) {
        BaseObject *obj = pair.second;
        if (!obj)
            continue;

        float distance;
        if (RayIntersectObject(mouseRay, obj, distance)) {
            if (distance < minDistSq) {
                minDistSq = distance;
                pickedName = pair.first;
                foundRayHit = true;
            }
        }
    }

    // レイキャスティングで見つからなかった場合、スクリーン座標での判定も試す
    if (!foundRayHit) {
        minDistSq = std::numeric_limits<float>::max();

        for (const auto &pair : transformMap) {
            BaseObject *obj = pair.second;
            if (!obj)
                continue;

            Vector3 worldPos = obj->GetWorldPosition();
            Vector3 screenPos;

            // ワールド座標をスクリーン座標に変換
            if (WorldToScreen(worldPos, screenPos, scenePosition, sceneSize)) {
                float dx = mousePos.x - screenPos.x;
                float dy = mousePos.y - screenPos.y;
                float distSq = dx * dx + dy * dy;

                Vector3 objScale = obj->GetWorldScale();
                float maxScale = std::max({objScale.x, objScale.y, objScale.z});
                float screenRadius = std::max(maxScale * 50.0f, 30.0f); // より大きな判定範囲

                if (distSq < screenRadius * screenRadius && distSq < minDistSq) {
                    minDistSq = distSq;
                    pickedName = pair.first;
                }
            }
        }
    }

    // 結果に応じて選択を更新
    if (!pickedName.empty()) {
        selectedName = pickedName;
    } else {
        selectedName.clear();
    }
}

void ImGuizmoManager::DisplayGizmo(WorldTransform *transform) {
    if (!transform || !viewProjection)
        return;

    BaseObject *selectedObject = GetSelectedTarget();
    if (!selectedObject)
        return;

    Matrix4x4 worldMatrix;
    Matrix4x4 parentMatrix = MakeIdentity4x4();

    // 現在の操作モードに応じて適切な行列を使用
    if (currentMode == ImGuizmo::LOCAL) {
        // ローカルモード：親の変換を考慮したローカル変換行列を使用
        if (selectedObject->GetParent()) {
            // 親のワールド行列を取得
            BaseObject *parent = selectedObject->GetParent();
            WorldTransform *parentTransform = parent->GetWorldTransform();
            if (parentTransform) {
                parentMatrix = parentTransform->matWorld_;
            }
        }

        // ローカル変換行列を作成
        Matrix4x4 scaleMatrix = MakeScaleMatrix(selectedObject->GetLocalScale());
        Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(selectedObject->GetLocalRotation());
        Matrix4x4 translateMatrix = MakeTranslateMatrix(selectedObject->GetLocalPosition());
        Matrix4x4 localMatrix = scaleMatrix * rotateMatrix * translateMatrix;

        worldMatrix = localMatrix * parentMatrix;
    } else {
        // ワールドモード：ワールド変換行列をそのまま使用
        worldMatrix = transform->matWorld_;
    }

    // ImGuizmoで使用するため、行列を配列形式に変換
    float matrixArray[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            matrixArray[i * 4 + j] = worldMatrix.m[i][j];
        }
    }

    // ビューとプロジェクション行列も配列形式に変換
    float viewArray[16], projArray[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            viewArray[i * 4 + j] = viewProjection->matView_.m[i][j];
            projArray[i * 4 + j] = viewProjection->matProjection_.m[i][j];
        }
    }

    // Gizmoを表示・操作
    if (ImGuizmo::Manipulate(viewArray, projArray, currentOperation, currentMode, matrixArray)) {
        // 操作された行列から変換情報を抽出
        Matrix4x4 newMatrix;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                newMatrix.m[i][j] = matrixArray[i * 4 + j];
            }
        }

        if (currentMode == ImGuizmo::LOCAL && selectedObject->GetParent()) {
            // ローカルモード：親の逆変換を適用してローカル座標に変換
            Matrix4x4 invParentMatrix = Inverse(parentMatrix);
            Matrix4x4 localMatrix = newMatrix * invParentMatrix;

            Vector3 position, scale;
            Quaternion rotation;
            DecomposeMatrix(localMatrix, position, rotation, scale);

            // ローカル座標として設定
            selectedObject->GetLocalPosition() = position;
            selectedObject->GetLocalRotation() = rotation.Normalize();
            selectedObject->GetLocalScale() = scale;
        } else {
            // ワールドモードまたは親がない場合
            Vector3 position, scale;
            Quaternion rotation;
            DecomposeMatrix(newMatrix, position, rotation, scale);

            if (selectedObject->GetParent()) {
                // 親がある場合：ワールド座標をローカル座標に変換
                Matrix4x4 invParentMatrix = Inverse(parentMatrix);
                Vector4 localPos4 = Transformation(Vector4(position.x, position.y, position.z, 1.0f), invParentMatrix);
                Vector3 localPos = Vector3(localPos4.x, localPos4.y, localPos4.z);

                // 回転もローカルに変換
                Quaternion parentRotation = selectedObject->GetParent()->GetWorldRotation();
                Quaternion invParentRotation = parentRotation.Conjugate();
                Quaternion localRotation = rotation * invParentRotation;

                selectedObject->GetLocalPosition() = localPos;
                selectedObject->GetLocalRotation() = localRotation.Normalize();
                selectedObject->GetLocalScale() = scale; // スケールはそのまま
            } else {
                // 親がない場合：そのまま設定
                selectedObject->GetLocalPosition() = position;
                selectedObject->GetLocalRotation() = rotation.Normalize();
                selectedObject->GetLocalScale() = scale;
            }
        }

        // ワールドトランスフォームを更新
        transform->translation_ = selectedObject->GetLocalPosition();
        transform->quateRotation_ = selectedObject->GetLocalRotation();
        transform->scale_ = selectedObject->GetLocalScale();
        transform->UpdateMatrix();

        // 階層全体を更新
        selectedObject->UpdateWorldTransformHierarchy();
    }
}

void ImGuizmoManager::DecomposeMatrix(const Matrix4x4 &matrix, Vector3 &position, Quaternion &rotation, Vector3 &scale) {
    // 位置の抽出
    position = {matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]};

    // スケールの抽出
    Vector3 col0 = {matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]};
    Vector3 col1 = {matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]};
    Vector3 col2 = {matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]};

    scale.x = col0.Length();
    scale.y = col1.Length();
    scale.z = col2.Length();

    // 回転行列の抽出（スケールを除去）
    Matrix4x4 rotMatrix = matrix;
    if (scale.x != 0.0f) {
        rotMatrix.m[0][0] /= scale.x;
        rotMatrix.m[0][1] /= scale.x;
        rotMatrix.m[0][2] /= scale.x;
    }
    if (scale.y != 0.0f) {
        rotMatrix.m[1][0] /= scale.y;
        rotMatrix.m[1][1] /= scale.y;
        rotMatrix.m[1][2] /= scale.y;
    }
    if (scale.z != 0.0f) {
        rotMatrix.m[2][0] /= scale.z;
        rotMatrix.m[2][1] /= scale.z;
        rotMatrix.m[2][2] /= scale.z;
    }

    // 回転行列からクォータニオンを抽出
    rotation = Quaternion::FromMatrix(rotMatrix);
}

Ray ImGuizmoManager::CreateMouseRay(const ImVec2 &mousePos, const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    // マウス位置を正規化デバイス座標系に変換
    float ndcX = ((mousePos.x - scenePosition.x) / sceneSize.x) * 2.0f - 1.0f;
    float ndcY = 1.0f - ((mousePos.y - scenePosition.y) / sceneSize.y) * 2.0f;

    // ビュー行列とプロジェクション行列の逆行列を計算
    Matrix4x4 invView = Inverse(viewProjection->matView_);
    Matrix4x4 invProj = Inverse(viewProjection->matProjection_);

    // NDC座標からビュー空間の座標に変換
    Vector4 nearPoint = {ndcX, ndcY, -1.0f, 1.0f}; // Near plane
    Vector4 farPoint = {ndcX, ndcY, 1.0f, 1.0f};   // Far plane

    // プロジェクション逆変換
    nearPoint = Transformation(nearPoint, invProj);
    farPoint = Transformation(farPoint, invProj);

    // 同次座標系から3D座標に変換
    if (nearPoint.w != 0.0f) {
        nearPoint.x /= nearPoint.w;
        nearPoint.y /= nearPoint.w;
        nearPoint.z /= nearPoint.w;
    }
    if (farPoint.w != 0.0f) {
        farPoint.x /= farPoint.w;
        farPoint.y /= farPoint.w;
        farPoint.z /= farPoint.w;
    }

    // ビュー空間からワールド空間に変換
    Vector4 worldNear = Transformation(nearPoint, invView);
    Vector4 worldFar = Transformation(farPoint, invView);

    Ray ray;
    ray.origin = {worldNear.x, worldNear.y, worldNear.z};
    Vector3 farPos = {worldFar.x, worldFar.y, worldFar.z};
    ray.direction = (farPos - ray.origin).Normalize();

    return ray;
}

bool ImGuizmoManager::RayIntersectObject(const Ray &ray, BaseObject *obj, float &distance) {
    Vector3 objPos = obj->GetWorldPosition();
    Vector3 objScale = obj->GetWorldScale();

    // より大きな判定半径
    float maxScale = std::max({objScale.x, objScale.y, objScale.z});
    float radius = maxScale * 1.2f;
    radius = std::max(radius, 1.0f);

    return RayIntersectSphere(ray, objPos, radius, distance);
}

bool ImGuizmoManager::RayIntersectSphere(const Ray &ray, const Vector3 &center, float radius, float &distance) {
    Vector3 oc = ray.origin - center;
    float a = ray.direction.Dot(ray.direction);
    float b = 2.0f * oc.Dot(ray.direction);
    float c = oc.Dot(oc) - radius * radius;

    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) {
        return false; // 交差なし
    }

    float t1 = (-b - std::sqrt(discriminant)) / (2.0f * a);
    float t2 = (-b + std::sqrt(discriminant)) / (2.0f * a);

    // 最も近い正の交点を選択
    if (t1 > 0) {
        distance = t1;
        return true;
    } else if (t2 > 0) {
        distance = t2;
        return true;
    }

    return false;
}

bool ImGuizmoManager::WorldToScreen(const Vector3 &worldPos, Vector3 &screenPos, const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    // ビュー射影変換
    Vector4 clipPos;
    {
        Vector3 v = worldPos;
        float x = v.x * viewProjection->matView_.m[0][0] + v.y * viewProjection->matView_.m[1][0] + v.z * viewProjection->matView_.m[2][0] + viewProjection->matView_.m[3][0];
        float y = v.x * viewProjection->matView_.m[0][1] + v.y * viewProjection->matView_.m[1][1] + v.z * viewProjection->matView_.m[2][1] + viewProjection->matView_.m[3][1];
        float z = v.x * viewProjection->matView_.m[0][2] + v.y * viewProjection->matView_.m[1][2] + v.z * viewProjection->matView_.m[2][2] + viewProjection->matView_.m[3][2];
        float w = v.x * viewProjection->matView_.m[0][3] + v.y * viewProjection->matView_.m[1][3] + v.z * viewProjection->matView_.m[2][3] + viewProjection->matView_.m[3][3];

        // 射影変換
        clipPos.x = x * viewProjection->matProjection_.m[0][0] + y * viewProjection->matProjection_.m[1][0] + z * viewProjection->matProjection_.m[2][0] + w * viewProjection->matProjection_.m[3][0];
        clipPos.y = x * viewProjection->matProjection_.m[0][1] + y * viewProjection->matProjection_.m[1][1] + z * viewProjection->matProjection_.m[2][1] + w * viewProjection->matProjection_.m[3][1];
        clipPos.z = x * viewProjection->matProjection_.m[0][2] + y * viewProjection->matProjection_.m[1][2] + z * viewProjection->matProjection_.m[2][2] + w * viewProjection->matProjection_.m[3][2];
        clipPos.w = x * viewProjection->matProjection_.m[0][3] + y * viewProjection->matProjection_.m[1][3] + z * viewProjection->matProjection_.m[2][3] + w * viewProjection->matProjection_.m[3][3];
    }

    if (clipPos.w <= 0.0f) {
        return false; // カメラの後ろにある
    }

    // NDC座標に変換
    float ndcX = clipPos.x / clipPos.w;
    float ndcY = clipPos.y / clipPos.w;

    // スクリーン座標に変換
    screenPos.x = scenePosition.x + (ndcX * 0.5f + 0.5f) * sceneSize.x;
    screenPos.y = scenePosition.y + (0.5f - ndcY * 0.5f) * sceneSize.y;
    screenPos.z = clipPos.z / clipPos.w;

    return true;
}

#endif // _DEBUG