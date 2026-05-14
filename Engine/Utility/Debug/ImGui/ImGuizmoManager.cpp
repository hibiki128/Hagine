#define NOMINMAX
#ifdef _DEBUG
#include "ImGuizmoManager.h"
#include "Input.h"
#include <Line/DrawLine3D.h>
#include <Object/Base/BaseObjectManager.h>
#include <Transform/WorldTransform.h>
#include "WinApp.h"

// =======================================================================
// GizmoTarget メンバ関数実装
// =======================================================================

// 各型に対応したワールド行列を返す
// FreeTransform の場合は translate/rotate/scale ポインタから行列を構築する
Matrix4x4 GizmoTarget::GetWorldMatrix() const {
    switch (type) {
    case Type::BaseObject:
        if (baseObject && baseObject->GetWorldTransform()) {
            return baseObject->GetWorldTransform()->matWorld_;
        }
        break;

    case Type::WorldTransform:
        if (worldTransform) {
            return worldTransform->matWorld_;
        }
        break;

    case Type::FreeTransform:
        if (translate) {
            Vector3 s = scale ? *scale : Vector3{1.0f, 1.0f, 1.0f};
            Vector3 r = rotate ? *rotate : Vector3{0.0f, 0.0f, 0.0f};
            return MakeAffineMatrix(s, r, *translate);
        }
        break;
    }
    return MakeIdentity4x4();
}

// ワールド座標（位置成分）を返す
Vector3 GizmoTarget::GetWorldPosition() const {
    switch (type) {
    case Type::BaseObject:
        if (baseObject)
            return baseObject->GetWorldPosition();
        break;
    case Type::WorldTransform:
        if (worldTransform)
            return worldTransform->GetWorldPosition();
        break;
    case Type::FreeTransform:
        if (translate)
            return *translate;
        break;
    }
    return {0.0f, 0.0f, 0.0f};
}

// ギズモ操作によって生じた平行移動デルタを各型に適用する
void GizmoTarget::ApplyTranslationDelta(const Vector3 &delta) {
    switch (type) {
    case Type::BaseObject:
        if (baseObject) {
            baseObject->GetLocalPosition() = baseObject->GetLocalPosition() + delta;
            WorldTransform *wt = baseObject->GetWorldTransform();
            if (wt) {
                wt->translation_ = baseObject->GetLocalPosition();
                wt->UpdateMatrix();
                baseObject->UpdateWorldTransformHierarchy();
            }
        }
        break;

    case Type::WorldTransform:
        if (worldTransform) {
            worldTransform->translation_ = worldTransform->translation_ + delta;
            worldTransform->UpdateMatrix();
        }
        break;

    case Type::FreeTransform:
        if (translate) {
            *translate = *translate + delta;
        }
        break;
    }
}

// ImGui で変換詳細を表示する
// imguiCallback が設定されている場合はそちらを優先する
void GizmoTarget::ShowImGui() {
    switch (type) {
    case Type::BaseObject:
        if (baseObject) {
            baseObject->ImGui();
        }
        break;

    case Type::WorldTransform:
        if (worldTransform) {
            if (imguiCallback) {
                imguiCallback();
            } else {
                ImGui::DragFloat3("Translation", &worldTransform->translation_.x, 0.1f);
                ImGui::DragFloat3("Scale", &worldTransform->scale_.x, 0.01f);
                Vector3 euler = worldTransform->GetRotationEuler();
                if (ImGui::DragFloat3("Rotation (rad)", &euler.x, 0.01f)) {
                    worldTransform->SetRotationEuler(euler);
                }
                if (ImGui::Button("UpdateMatrix")) {
                    worldTransform->UpdateMatrix();
                }
            }
        }
        break;

    case Type::FreeTransform:
        if (imguiCallback) {
            imguiCallback();
        } else {
            if (translate) {
                if (isScreenSpace) {
                    // スプライトはピクセル座標のため XY のみ編集
                    ImGui::DragFloat2("Position (px)", &translate->x, 1.0f);
                } else {
                    ImGui::DragFloat3("Translation", &translate->x, 0.1f);
                }
            }
            if (rotate)
                ImGui::DragFloat3("Rotation (rad)", &rotate->x, 0.01f);
            if (scale)
                ImGui::DragFloat3("Scale", &scale->x, 0.01f);
        }
        break;
    }
}

// =======================================================================
// ImGuizmoManager メンバ関数実装
// =======================================================================

void ImGuizmoManager::Finalize() {
    transformMap.clear();
    selectedNames.clear();
    copiedObjects.clear();
}

void ImGuizmoManager::BeginFrame() {
    ImGuizmo::BeginFrame();
}

void ImGuizmoManager::SetViewProjection(ViewProjection *vp) {
    viewProjection = vp;
}

// ---- AddTarget オーバーロード群 ----------------------------------------

// BaseObject を登録する
void ImGuizmoManager::AddTarget(const std::string &name, BaseObject *object) {
    GizmoTarget target;
    target.type = GizmoTarget::Type::BaseObject;
    target.name = name;
    target.baseObject = object;
    transformMap[name] = target;

    UpdateFilteredNames();

    if (selectedNames.empty()) {
        selectedNames.insert(name);
    }
}

// WorldTransform のみを持つオブジェクトを登録する
void ImGuizmoManager::AddTarget(const std::string &name, WorldTransform *worldTransform,
                                std::function<void()> imguiCallback) {
    GizmoTarget target;
    target.type = GizmoTarget::Type::WorldTransform;
    target.name = name;
    target.worldTransform = worldTransform;
    target.imguiCallback = imguiCallback;
    transformMap[name] = target;

    UpdateFilteredNames();

    if (selectedNames.empty()) {
        selectedNames.insert(name);
    }
}

// Vector3 ポインタを直接指定して登録する（Sprite・ParticleEmitter など）
void ImGuizmoManager::AddTarget(const std::string &name,
                                Vector3 *translate,
                                Vector3 *rotate,
                                Vector3 *scale,
                                std::function<void()> imguiCallback) {
    GizmoTarget target;
    target.type = GizmoTarget::Type::FreeTransform;
    target.name = name;
    target.translate = translate;
    target.rotate = rotate;
    target.scale = scale;
    target.imguiCallback = imguiCallback;
    transformMap[name] = target;

    UpdateFilteredNames();

    if (selectedNames.empty()) {
        selectedNames.insert(name);
    }
}

// ---- 選択オブジェクト取得（BaseObject 互換用）---------------------------

// 選択中の最初のエントリが BaseObject である場合に返す
BaseObject *ImGuizmoManager::GetSelectedTarget() {
    if (selectedNames.empty())
        return nullptr;

    auto it = transformMap.find(*selectedNames.begin());
    if (it != transformMap.end() && it->second.type == GizmoTarget::Type::BaseObject) {
        return it->second.baseObject;
    }
    return nullptr;
}

// 選択中のエントリのうち BaseObject のもののみを返す
std::vector<BaseObject *> ImGuizmoManager::GetSelectedTargets() {
    std::vector<BaseObject *> selected;
    for (const std::string &name : selectedNames) {
        auto it = transformMap.find(name);
        if (it != transformMap.end() && it->second.type == GizmoTarget::Type::BaseObject) {
            selected.push_back(it->second.baseObject);
        }
    }
    return selected;
}

// ---- imgui ------------------------------------------------------------

void ImGuizmoManager::imgui() {
    if (!viewProjection)
        return;

    ImGui::Checkbox("デバッグ表示する", &isDrawDebug_);

    // 操作モード選択
    if (ImGui::RadioButton("移動", currentOperation == ImGuizmo::TRANSLATE))
        currentOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("回転", currentOperation == ImGuizmo::ROTATE))
        currentOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("スケール", currentOperation == ImGuizmo::SCALE))
        currentOperation = ImGuizmo::SCALE;

    // 座標系選択
    if (ImGui::RadioButton("ローカル", currentMode == ImGuizmo::LOCAL))
        currentMode = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("ワールド", currentMode == ImGuizmo::WORLD))
        currentMode = ImGuizmo::WORLD;

    ImGui::Separator();

    // 検索ボックス
    ImGui::Text("オブジェクト検索:");
    bool searchChanged = ImGui::InputText("##ObjectSearch", searchBuffer_, sizeof(searchBuffer_));
    if (searchChanged)
        UpdateFilteredNames();
    if (filteredNames_.empty())
        UpdateFilteredNames();

    std::string currentDisplayName = selectedNames.empty() ? "なし"
                                                           : (selectedNames.size() == 1 ? *selectedNames.begin()
                                                                                        : "複数選択 (" + std::to_string(selectedNames.size()) + "個)");

    if (ImGui::BeginCombo("選択オブジェクト", currentDisplayName.c_str())) {
        bool isNoneSelected = selectedNames.empty();
        if (ImGui::Selectable("なし", isNoneSelected))
            selectedNames.clear();
        if (isNoneSelected)
            ImGui::SetItemDefaultFocus();

        for (const std::string &name : filteredNames_) {
            auto it = transformMap.find(name);
            if (it != transformMap.end()) {
                bool isSelected = (selectedNames.find(name) != selectedNames.end());
                if (ImGui::Selectable(name.c_str(), isSelected)) {
                    selectedNames.clear();
                    selectedNames.insert(name);
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (strlen(searchBuffer_) > 0) {
        ImGui::Text("検索結果: %zu個", filteredNames_.size());
    }

    ImGui::Spacing();
    ImGui::Text("選択中のオブジェクト数: %zu", selectedNames.size());
    if (!selectedNames.empty()) {
        ImGui::Text("選択中:");
        for (const std::string &name : selectedNames) {
            ImGui::BulletText("%s", name.c_str());
        }
    }

    ImGui::Separator();

    if (ImGui::Button("全選択")) {
        selectedNames.clear();
        for (const auto &pair : transformMap)
            selectedNames.insert(pair.first);
    }
    ImGui::SameLine();
    if (ImGui::Button("選択解除"))
        selectedNames.clear();

    ImGui::Spacing();

    if (!selectedNames.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
        ImGui::Text("オブジェクト詳細 (%s)", selectedNames.begin()->c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        ShowSelectedObjectImGui();

        ImGui::Spacing();
        ImGui::Spacing();

        // BaseObject のみコピー・ペーストが可能
        auto it = transformMap.find(*selectedNames.begin());
        if (it != transformMap.end() && it->second.type == GizmoTarget::Type::BaseObject) {
            if (ImGui::Button("コピー", ImVec2(-1, 30)))
                CopySelectedObjects();
            if (!copiedObjects.empty()) {
                if (ImGui::Button("ペースト", ImVec2(-1, 30)))
                    PasteObjects();
            }
            ImGui::Spacing();
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("選択オブジェクトを削除", ImVec2(-1, 0)))
            DeleteSelectedObjects();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("選択中の全オブジェクトを削除します");
        ImGui::PopStyleColor(3);
    }

    ImGui::Separator();
    if (isDrawDebug_)
        DrawDebugRaycast();
}

// ---- Update -----------------------------------------------------------

void ImGuizmoManager::Update(const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    if (!viewProjection)
        return;

    ImGuizmo::SetRect(scenePosition.x, scenePosition.y, sceneSize.x, sceneSize.y);
    ImGuizmo::SetDrawlist();

    if (!ImGuizmo::IsUsing()) {
        HandleMouseSelection(scenePosition, sceneSize);
    }

    DrawSelectedObjectHighlight();

    if (!selectedNames.empty()) {
        // スプライト用正射影 VP を使うためシーン情報を渡す
        DisplayGizmo(scenePosition, sceneSize);
    }
}

// ---- ShowSelectedObjectImGui ------------------------------------------

// 選択中エントリの ShowImGui を呼び出す
void ImGuizmoManager::ShowSelectedObjectImGui() {
    if (selectedNames.empty())
        return;

    std::string firstName = *selectedNames.begin();
    auto it = transformMap.find(firstName);
    if (it != transformMap.end()) {
        it->second.ShowImGui();
    }

    if (selectedNames.size() > 1) {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
        ImGui::Text("※ %zu個のオブジェクトが選択されています", selectedNames.size());
        ImGui::Text("表示しているのは '%s' の設定です", firstName.c_str());
        ImGui::PopStyleColor();
    }
}

// ---- HandleMouseSelection ---------------------------------------------

// マウスクリック時のレイキャストによる選択判定
// BaseObject/WorldTransform/FreeTransform すべての型に対応するため
// 行列版の RayIntersectAABBByMatrix を使用する
void ImGuizmoManager::HandleMouseSelection(const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isInScene = (mousePos.x >= scenePosition.x && mousePos.x <= scenePosition.x + sceneSize.x &&
                      mousePos.y >= scenePosition.y && mousePos.y <= scenePosition.y + sceneSize.y);

    if (ImGuizmo::IsUsing() || !isInScene || !Input::IsTriggerMouse(0) || !viewProjection)
        return;

    bool isCtrlPressed = Input::GetInstance()->PushKey(DIK_LCONTROL);
    std::string pickedName;
    bool foundHit = false;

    // マウス位置をシーンウィンドウ相対座標に変換し、さらにスプライト座標系にスケール
    // シーンウィンドウ(sceneSize)は実際の解像度(kClientWidth/Height)と異なるサイズで表示されている
    Vector2 mouseScreenPos = Input::GetMousePos();
    float relX = mouseScreenPos.x - scenePosition.x;
    float relY = mouseScreenPos.y - scenePosition.y;
    float spriteSpaceX = (relX / sceneSize.x) * static_cast<float>(WinApp::kClientWidth);
    float spriteSpaceY = (relY / sceneSize.y) * static_cast<float>(WinApp::kClientHeight);

    // ---- パス1: スクリーン空間（Sprite）ターゲットを優先して2Dヒットテスト ----
    float minDist2D = std::numeric_limits<float>::max();
    for (const auto &pair : transformMap) {
        const GizmoTarget &target = pair.second;
        if (!target.selectable || !target.isScreenSpace || !target.translate)
            continue;
        if (isMultiSelecting && selectedNames.find(pair.first) != selectedNames.end())
            continue;

        float dx = spriteSpaceX - target.translate->x;
        float dy = spriteSpaceY - target.translate->y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= target.screenHitRadius && dist < minDist2D) {
            minDist2D = dist;
            pickedName = pair.first;
            foundHit = true;
        }
    }

    // ---- パス2: 2Dヒットがなければ3DレイキャストでBaseObject/WorldTransformを判定 ----
    if (!foundHit) {
        Ray currentRay = Input::GetInstance()->GetCurrentRay();
        float minDist3D = std::numeric_limits<float>::max();

        AABB defaultAABB;
        defaultAABB.min = {-1.3f, -1.3f, -1.3f};
        defaultAABB.max = {1.3f, 1.3f, 1.3f};
        Sphere defaultSphere;
        defaultSphere.center = {0.0f, 0.0f, 0.0f};
        defaultSphere.radius = 1.3f;

        for (const auto &pair : transformMap) {
            const GizmoTarget &target = pair.second;
            if (!target.selectable || target.isScreenSpace)
                continue;
            if (target.type == GizmoTarget::Type::BaseObject) {
                if (!target.baseObject || !target.baseObject->IsGizmoSelectable())
                    continue;
            }
            if (isMultiSelecting && selectedNames.find(pair.first) != selectedNames.end())
                continue;

            Matrix4x4 worldMatrix = target.GetWorldMatrix();
            RayHitInfo currentHit;
            bool hit = Input::RayIntersectAABBByMatrix(currentRay, worldMatrix, currentHit, defaultAABB);
            if (!hit) {
                hit = Input::RayIntersectSphereByMatrix(currentRay, worldMatrix, currentHit, defaultSphere);
            }

            if (hit && currentHit.distance < minDist3D) {
                minDist3D = currentHit.distance;
                pickedName = pair.first;
                foundHit = true;
            }
        }
    }

    // 選択状態を更新
    if (foundHit && !pickedName.empty()) {
        if (isCtrlPressed) {
            if (selectedNames.find(pickedName) != selectedNames.end()) {
                selectedNames.erase(pickedName);
            } else {
                selectedNames.insert(pickedName);
            }
            isMultiSelecting = true;
        } else {
            selectedNames.clear();
            selectedNames.insert(pickedName);
            isMultiSelecting = false;
        }
    } else {
        if (!isCtrlPressed) {
            selectedNames.clear();
            isMultiSelecting = false;
        }
    }

    if (!isCtrlPressed && isMultiSelecting) {
        isMultiSelecting = false;
    }
}

// ---- DisplayGizmo -----------------------------------------------------

// 選択中の全エントリの重心位置にギズモを表示し、操作量を各エントリに反映する
void ImGuizmoManager::DisplayGizmo(const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    if (!viewProjection || selectedNames.empty())
        return;

    std::vector<GizmoTarget *> selectedTargets;
    for (const std::string &name : selectedNames) {
        auto it = transformMap.find(name);
        if (it != transformMap.end()) {
            selectedTargets.push_back(&it->second);
        }
    }
    if (selectedTargets.empty())
        return;

    // 選択中にスクリーン空間（Sprite）が含まれるか確認
    // ※ 3Dオブジェクトとスプライトを同時選択した場合は動作が未定義
    bool anyScreenSpace = std::any_of(selectedTargets.begin(), selectedTargets.end(),
                                      [](const GizmoTarget *t) { return t->isScreenSpace; });

    // 選択中ターゲットの重心位置を求め、ギズモ表示用仮想行列を生成
    Vector3 centerPos = {0.0f, 0.0f, 0.0f};
    for (GizmoTarget *target : selectedTargets) {
        centerPos = centerPos + target->GetWorldPosition();
    }
    centerPos = centerPos / static_cast<float>(selectedTargets.size());

    Matrix4x4 centerMatrix = MakeIdentity4x4();
    centerMatrix.m[3][0] = centerPos.x;
    centerMatrix.m[3][1] = centerPos.y;
    centerMatrix.m[3][2] = centerPos.z;

    float matrixArray[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            matrixArray[i * 4 + j] = centerMatrix.m[i][j];

    float viewArray[16], projArray[16];

    if (anyScreenSpace) {
        // スプライト用：単位ビュー行列 + スプライトと同じ正射影行列
        // これにより ImGuizmo がピクセル座標系でギズモを正しい位置に描画する
        Matrix4x4 identView = MakeIdentity4x4();
        Matrix4x4 orthoProj = MakeOrthographicMatrix(
            0.0f, 0.0f,
            static_cast<float>(WinApp::kClientWidth),
            static_cast<float>(WinApp::kClientHeight),
            0.0f, 100.0f);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                viewArray[i * 4 + j] = identView.m[i][j];
                projArray[i * 4 + j] = orthoProj.m[i][j];
            }
        }
    } else {
        // 3Dオブジェクト用：カメラの View/Projection をそのまま使用
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                viewArray[i * 4 + j] = viewProjection->matView_.m[i][j];
                projArray[i * 4 + j] = viewProjection->matProjection_.m[i][j];
            }
        }
    }

    if (ImGuizmo::Manipulate(viewArray, projArray, currentOperation, currentMode, matrixArray)) {
        Matrix4x4 newMatrix;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                newMatrix.m[i][j] = matrixArray[i * 4 + j];

        // 操作によって生じたデルタを全選択ターゲットに適用する
        // スクリーン空間の場合、deltaPos はピクセル単位になるので直接加算できる
        Vector3 deltaPos = {
            newMatrix.m[3][0] - centerMatrix.m[3][0],
            newMatrix.m[3][1] - centerMatrix.m[3][1],
            newMatrix.m[3][2] - centerMatrix.m[3][2]};

        for (GizmoTarget *target : selectedTargets) {
            target->ApplyTranslationDelta(deltaPos);
        }
    }
}

// ---- DecomposeMatrix --------------------------------------------------

// 行列からスケール・回転（クォータニオン）・位置を分解して返す
void ImGuizmoManager::DecomposeMatrix(const Matrix4x4 &matrix, Vector3 &position, Quaternion &rotation, Vector3 &scale) {
    position = {matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]};

    Vector3 col0 = {matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]};
    Vector3 col1 = {matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]};
    Vector3 col2 = {matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]};

    scale.x = col0.Length();
    scale.y = col1.Length();
    scale.z = col2.Length();

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

    rotation = Quaternion::FromMatrix(rotMatrix);
}

// ---- WorldToScreen ----------------------------------------------------

// ワールド座標をシーンウィンドウのスクリーン座標に変換する
bool ImGuizmoManager::WorldToScreen(const Vector3 &worldPos, Vector3 &screenPos, const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    Vector4 clipPos;
    {
        Vector3 v = worldPos;
        float x = v.x * viewProjection->matView_.m[0][0] + v.y * viewProjection->matView_.m[1][0] + v.z * viewProjection->matView_.m[2][0] + viewProjection->matView_.m[3][0];
        float y = v.x * viewProjection->matView_.m[0][1] + v.y * viewProjection->matView_.m[1][1] + v.z * viewProjection->matView_.m[2][1] + viewProjection->matView_.m[3][1];
        float z = v.x * viewProjection->matView_.m[0][2] + v.y * viewProjection->matView_.m[1][2] + v.z * viewProjection->matView_.m[2][2] + viewProjection->matView_.m[3][2];
        float w = v.x * viewProjection->matView_.m[0][3] + v.y * viewProjection->matView_.m[1][3] + v.z * viewProjection->matView_.m[2][3] + viewProjection->matView_.m[3][3];

        clipPos.x = x * viewProjection->matProjection_.m[0][0] + y * viewProjection->matProjection_.m[1][0] + z * viewProjection->matProjection_.m[2][0] + w * viewProjection->matProjection_.m[3][0];
        clipPos.y = x * viewProjection->matProjection_.m[0][1] + y * viewProjection->matProjection_.m[1][1] + z * viewProjection->matProjection_.m[2][1] + w * viewProjection->matProjection_.m[3][1];
        clipPos.z = x * viewProjection->matProjection_.m[0][2] + y * viewProjection->matProjection_.m[1][2] + z * viewProjection->matProjection_.m[2][2] + w * viewProjection->matProjection_.m[3][2];
        clipPos.w = x * viewProjection->matProjection_.m[0][3] + y * viewProjection->matProjection_.m[1][3] + z * viewProjection->matProjection_.m[2][3] + w * viewProjection->matProjection_.m[3][3];
    }

    if (clipPos.w <= 0.0f)
        return false;

    float ndcX = clipPos.x / clipPos.w;
    float ndcY = clipPos.y / clipPos.w;

    screenPos.x = scenePosition.x + (ndcX * 0.5f + 0.5f) * sceneSize.x;
    screenPos.y = scenePosition.y + (0.5f - ndcY * 0.5f) * sceneSize.y;
    screenPos.z = clipPos.z / clipPos.w;

    return true;
}

// ---- GenerateUniqueName -----------------------------------------------

// 同名エントリが存在しないユニークな名前を生成する
std::string ImGuizmoManager::GenerateUniqueName(const std::string &baseName) {
    std::string newName;
    int counter = 1;

    std::string cleanBaseName = baseName;
    size_t underscorePos = baseName.find_last_of('_');
    if (underscorePos != std::string::npos) {
        std::string suffix = baseName.substr(underscorePos + 1);
        bool isNumber = true;
        for (char c : suffix) {
            if (!std::isdigit(c)) {
                isNumber = false;
                break;
            }
        }
        if (isNumber)
            cleanBaseName = baseName.substr(0, underscorePos);
    }

    do {
        newName = cleanBaseName + "_" + std::to_string(counter++);
    } while (transformMap.find(newName) != transformMap.end());

    return newName;
}

// ---- CopySelectedObjects / PasteObjects / DeleteSelectedObjects --------

// 選択中の BaseObject をコピーバッファに保存する（非 BaseObject はスキップ）
void ImGuizmoManager::CopySelectedObjects() {
    copiedObjects.clear();
    for (const std::string &name : selectedNames) {
        auto it = transformMap.find(name);
        if (it != transformMap.end() && it->second.type == GizmoTarget::Type::BaseObject) {
            copiedObjects.push_back(it->second.baseObject);
        }
    }
}

// コピー済み BaseObject を複製して BaseObjectManager に追加する
void ImGuizmoManager::PasteObjects() {
    if (copiedObjects.empty())
        return;

    selectedNames.clear();

    for (BaseObject *copiedObj : copiedObjects) {
        std::unique_ptr<BaseObject> newObject = std::make_unique<BaseObject>();
        newObject->SetPrimitive(copiedObj->IsPrimitive());
        newObject->Init(copiedObj->GetName());

        if (!copiedObj->GetModelPath().empty()) {
            newObject->CreateModel(copiedObj->GetModelPath());
        } else if (copiedObj->GetPrimitiveType() != PrimitiveType::kCount) {
            newObject->CreatePrimitiveModel(copiedObj->GetPrimitiveType());
        }

        if (!copiedObj->GetTexturePath().empty()) {
            newObject->SetTexture(copiedObj->GetTexturePath());
        }

        newObject->GetLocalPosition() = copiedObj->GetLocalPosition();
        newObject->GetLocalRotation() = copiedObj->GetLocalRotation();
        newObject->GetLocalScale() = copiedObj->GetLocalScale();
        newObject->GetLocalPosition().x += 1.0f;
        newObject->GetLighting() = copiedObj->GetLighting();
        newObject->GetLoop() = copiedObj->GetLoop();
        newObject->SetColor(copiedObj->GetColor());

        std::string uniqueName = GenerateUniqueName(copiedObj->GetName());
        newObject->GetName() = uniqueName;

        BaseObjectManager::GetInstance()->AddObject(std::move(newObject));

        BaseObject *addedObject = BaseObjectManager::GetInstance()->GetObjectByName(uniqueName);
        if (addedObject) {
            AddTarget(uniqueName, addedObject);
        }

        selectedNames.insert(uniqueName);
    }

    copiedObjects.clear();
}

// 選択中の全エントリを削除する
// BaseObject の場合は BaseObjectManager からも削除する
void ImGuizmoManager::DeleteSelectedObjects() {
    if (selectedNames.empty())
        return;

    for (const std::string &name : selectedNames) {
        auto it = transformMap.find(name);
        if (it != transformMap.end() && it->second.type == GizmoTarget::Type::BaseObject) {
            BaseObjectManager::GetInstance()->RemoveObject(name);
        }
        transformMap.erase(name);
    }

    UpdateFilteredNames();
    selectedNames.clear();

    if (!transformMap.empty()) {
        selectedNames.insert(transformMap.begin()->first);
    }
}

// ---- DrawSelectedObjectHighlight / DrawSelectionMarker ----------------

// 選択中の全エントリにハイライトマーカーを描画する
void ImGuizmoManager::DrawSelectedObjectHighlight() {
    if (selectedNames.empty() || !viewProjection)
        return;

    for (const std::string &selectedName : selectedNames) {
        auto it = transformMap.find(selectedName);
        if (it == transformMap.end())
            continue;

        // スクリーン空間ターゲットはピクセル座標を3D世界座標として扱えないためスキップ
        if (it->second.isScreenSpace)
            continue;

        DrawSelectionMarker(it->second.GetWorldPosition());
    }
}

// オブジェクトの上方に逆ピラミッド型の選択マーカーを描画する
void ImGuizmoManager::DrawSelectionMarker(const Vector3 &worldPosition) {
    Vector3 markerPos = worldPosition + Vector3(0.0f, 2.0f, 0.0f);
    Vector4 markerColor = {1.0f, 1.0f, 0.0f, 1.0f};
    float markerSize = 0.5f;

    Vector3 apex = markerPos - Vector3(0.0f, markerSize, 0.0f);
    Vector3 topLeft = markerPos + Vector3(-markerSize, markerSize, -markerSize);
    Vector3 topRight = markerPos + Vector3(markerSize, markerSize, -markerSize);
    Vector3 topFront = markerPos + Vector3(-markerSize, markerSize, markerSize);
    Vector3 topBack = markerPos + Vector3(markerSize, markerSize, markerSize);

    DrawLine3D::GetInstance()->SetPoints(apex, topLeft, markerColor);
    DrawLine3D::GetInstance()->SetPoints(apex, topRight, markerColor);
    DrawLine3D::GetInstance()->SetPoints(apex, topFront, markerColor);
    DrawLine3D::GetInstance()->SetPoints(apex, topBack, markerColor);
    DrawLine3D::GetInstance()->SetPoints(topLeft, topRight, markerColor);
    DrawLine3D::GetInstance()->SetPoints(topRight, topBack, markerColor);
    DrawLine3D::GetInstance()->SetPoints(topBack, topFront, markerColor);
    DrawLine3D::GetInstance()->SetPoints(topFront, topLeft, markerColor);
}

// ---- UpdateFilteredNames ----------------------------------------------

// 検索バッファに基づいてフィルタ済みの名前リストを更新する
void ImGuizmoManager::UpdateFilteredNames() {
    filteredNames_.clear();

    std::vector<std::string> allNames;
    for (const auto &pair : transformMap)
        allNames.push_back(pair.first);
    std::sort(allNames.begin(), allNames.end());

    std::string searchStr = searchBuffer_;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    for (const std::string &name : allNames) {
        if (strlen(searchBuffer_) == 0) {
            filteredNames_.push_back(name);
        } else {
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(searchStr) != std::string::npos) {
                filteredNames_.push_back(name);
            }
        }
    }
}

// ---- DrawDebugRaycast / DrawAABBWireframe / DrawSphereWireframe -------

// 全エントリのAABB・スフィアワイヤーフレームとレイを描画する
void ImGuizmoManager::DrawDebugRaycast() {
    if (!showDebugRaycast)
        return;

    Ray currentRay = Input::GetInstance()->GetCurrentRay();
    Vector3 rayEnd = currentRay.origin + (currentRay.direction * currentRay.length);
    DrawLine3D::GetInstance()->SetPoints(currentRay.origin, rayEnd, {1.0f, 0.0f, 0.0f, 1.0f});

    for (const auto &pair : transformMap) {
        const GizmoTarget &target = pair.second;

        // スクリーン空間ターゲットは3Dデバッグ描画対象外
        if (target.isScreenSpace)
            continue;

        Matrix4x4 worldMatrix = target.GetWorldMatrix();
        bool isSelected = selectedNames.find(pair.first) != selectedNames.end();
        Vector4 aabbColor = isSelected ? Vector4{1.0f, 1.0f, 0.0f, 1.0f} : Vector4{0.0f, 0.0f, 1.0f, 1.0f};
        Vector4 sphereColor = isSelected ? Vector4{1.0f, 0.5f, 0.0f, 1.0f} : Vector4{1.0f, 0.0f, 1.0f, 1.0f};

        DrawAABBWireframe(worldMatrix, aabbColor);
        DrawSphereWireframe(worldMatrix, sphereColor);
        TestAndDrawRayHit(currentRay, target);
    }
}

// ローカル空間のAABBをワールド変換してワイヤーフレームを描画する
void ImGuizmoManager::DrawAABBWireframe(const Matrix4x4 &worldMatrix, const Vector4 &color) {
    AABB aabb;
    aabb.min = {-1.3f, -1.3f, -1.3f};
    aabb.max = {1.3f, 1.3f, 1.3f};

    Vector3 vertices[8] = {
        {aabb.min.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.min.y, aabb.max.z},
        {aabb.min.x, aabb.min.y, aabb.max.z},
        {aabb.min.x, aabb.max.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.max.z},
        {aabb.min.x, aabb.max.y, aabb.max.z},
    };

    for (int i = 0; i < 8; i++) {
        vertices[i] = Transformation(vertices[i], worldMatrix);
    }

    // 下面
    DrawLine3D::GetInstance()->SetPoints(vertices[0], vertices[1], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[1], vertices[2], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[2], vertices[3], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[3], vertices[0], color);
    // 上面
    DrawLine3D::GetInstance()->SetPoints(vertices[4], vertices[5], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[5], vertices[6], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[6], vertices[7], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[7], vertices[4], color);
    // 縦
    DrawLine3D::GetInstance()->SetPoints(vertices[0], vertices[4], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[1], vertices[5], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[2], vertices[6], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[3], vertices[7], color);
}

// スフィアワイヤーフレームを描画する
void ImGuizmoManager::DrawSphereWireframe(const Matrix4x4 &worldMatrix, const Vector4 &color) {
    Sphere sphere{};

    Vector3 worldCenter = Transformation(sphere.center, worldMatrix);

    Vector3 scale = {
        sqrt(worldMatrix.m[0][0] * worldMatrix.m[0][0] + worldMatrix.m[1][0] * worldMatrix.m[1][0] + worldMatrix.m[2][0] * worldMatrix.m[2][0]),
        sqrt(worldMatrix.m[0][1] * worldMatrix.m[0][1] + worldMatrix.m[1][1] * worldMatrix.m[1][1] + worldMatrix.m[2][1] * worldMatrix.m[2][1]),
        sqrt(worldMatrix.m[0][2] * worldMatrix.m[0][2] + worldMatrix.m[1][2] * worldMatrix.m[1][2] + worldMatrix.m[2][2] * worldMatrix.m[2][2])};
    float worldRadius = sphere.radius * std::max({scale.x, scale.y, scale.z});

    DrawLine3D::GetInstance()->DrawSphere(worldCenter, color, worldRadius, 16);
}

// GizmoTarget のワールド行列を使ってAABB・スフィアのレイヒット点を描画する
void ImGuizmoManager::TestAndDrawRayHit(const Ray &ray, const GizmoTarget &target) {
    Matrix4x4 worldMatrix = target.GetWorldMatrix();

    AABB aabb;
    aabb.min = {-1.3f, -1.3f, -1.3f};
    aabb.max = {1.3f, 1.3f, 1.3f};
    Sphere sphere;
    sphere.center = {0.0f, 0.0f, 0.0f};
    sphere.radius = 1.3f;

    RayHitInfo aabbHit, sphereHit;
    bool aabbResult = Input::RayIntersectAABBByMatrix(ray, worldMatrix, aabbHit, aabb);
    bool sphereResult = Input::RayIntersectSphereByMatrix(ray, worldMatrix, sphereHit, sphere);

    if (aabbResult) {
        DrawLine3D::GetInstance()->DrawSphere(aabbHit.hitPoint, {0.0f, 1.0f, 0.0f, 1.0f}, 0.05f, 8);
        Vector3 normalEnd = aabbHit.hitPoint + (aabbHit.hitNormal * 0.3f);
        DrawLine3D::GetInstance()->SetPoints(aabbHit.hitPoint, normalEnd, {0.0f, 1.0f, 0.0f, 1.0f});
    }

    if (sphereResult) {
        DrawLine3D::GetInstance()->DrawSphere(sphereHit.hitPoint, {1.0f, 0.0f, 1.0f, 1.0f}, 0.05f, 8);
        Vector3 normalEnd = sphereHit.hitPoint + (sphereHit.hitNormal * 0.3f);
        DrawLine3D::GetInstance()->SetPoints(sphereHit.hitPoint, normalEnd, {1.0f, 0.0f, 1.0f, 1.0f});
    }
}

#endif // _DEBUG