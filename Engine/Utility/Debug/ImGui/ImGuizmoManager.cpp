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
    selectedNames.clear();
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

    // フィルタリストを更新
    UpdateFilteredNames();

    // 初期選択が未設定の場合、最初に追加されたものを自動選択
    if (selectedNames.empty()) {
        selectedNames.insert(name);
    }
}

BaseObject *ImGuizmoManager::GetSelectedTarget() {
    if (selectedNames.empty())
        return nullptr;

    auto it = transformMap.find(*selectedNames.begin());
    return (it != transformMap.end()) ? it->second : nullptr;
}

std::vector<BaseObject *> ImGuizmoManager::GetSelectedTargets() {
    std::vector<BaseObject *> selected;
    for (const std::string &name : selectedNames) {
        auto it = transformMap.find(name);
        if (it != transformMap.end()) {
            selected.push_back(it->second);
        }
    }
    return selected;
}

void ImGuizmoManager::imgui() {
    if (!viewProjection) {
        return;
    }

    ImGui::Checkbox("デバッグ表示する", &isDrawDebug_);

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

    // 検索ボックス
    ImGui::Text("オブジェクト検索:");
    bool searchChanged = ImGui::InputText("##ObjectSearch", searchBuffer_, sizeof(searchBuffer_));

    // 検索結果の更新とソート
    if (searchChanged) {
        UpdateFilteredNames();
    }

    // 初回実行時にフィルタリスト作成
    if (filteredNames_.empty()) {
        UpdateFilteredNames();
    }

    std::string currentDisplayName = selectedNames.empty() ? "なし" : (selectedNames.size() == 1 ? *selectedNames.begin() : "複数選択 (" + std::to_string(selectedNames.size()) + "個)");

    if (ImGui::BeginCombo("選択オブジェクト", currentDisplayName.c_str())) {
        // "なし"オプションを追加
        bool isNoneSelected = selectedNames.empty();
        if (ImGui::Selectable("なし", isNoneSelected)) {
            selectedNames.clear();
        }
        if (isNoneSelected) {
            ImGui::SetItemDefaultFocus();
        }

        // フィルタされたリストを表示
        for (const std::string &name : filteredNames_) {
            auto it = transformMap.find(name);
            if (it != transformMap.end()) {
                bool isSelected = (selectedNames.find(name) != selectedNames.end());
                if (ImGui::Selectable(name.c_str(), isSelected)) {
                    // 単一選択モード（従来の動作）
                    selectedNames.clear();
                    selectedNames.insert(name);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndCombo();
    }

    // 検索がアクティブな場合の表示
    if (strlen(searchBuffer_) > 0) {
        ImGui::Text("検索結果: %zu個", filteredNames_.size());
    }

    ImGui::Spacing();

    // 選択状態表示
    ImGui::Text("選択中のオブジェクト数: %zu", selectedNames.size());
    if (!selectedNames.empty()) {
        ImGui::Text("選択中:");
        for (const std::string &name : selectedNames) {
            ImGui::BulletText("%s", name.c_str());
        }
    }

    ImGui::Separator();

    // 複数選択コントロール
    if (ImGui::Button("全選択")) {
        selectedNames.clear();
        for (const auto &pair : transformMap) {
            selectedNames.insert(pair.first);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("選択解除")) {
        selectedNames.clear();
    }

    ImGui::Spacing();

    // 選択中のオブジェクト詳細（最初の1つのみ表示）
    if (!selectedNames.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
        ImGui::Text("オブジェクト詳細 (%s)", selectedNames.begin()->c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        ShowSelectedObjectImGui();

        ImGui::Spacing();
        ImGui::Spacing();

        // コピー・ペーストボタン
        if (ImGui::Button("コピー", ImVec2(-1, 30))) {
            CopySelectedObjects();
        }

        if (!copiedObjects.empty()) {
            if (ImGui::Button("ペースト", ImVec2(-1, 30))) {
                PasteObjects();
            }
        }

        ImGui::Spacing();

        // 削除ボタン
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

        if (ImGui::Button("選択オブジェクトを削除", ImVec2(-1, 0))) {
            DeleteSelectedObjects();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("選択中の全オブジェクトを削除します");
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::Separator();
    if (isDrawDebug_) {
        DrawDebugRaycast();
    }
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

    // 選択されたオブジェクトのハイライト表示
    DrawSelectedObjectHighlight();

    // 選択中オブジェクトに対してギズモ操作（複数選択対応）
    if (!selectedNames.empty()) {
        // 最初の選択オブジェクトのTransformを取得してギズモ表示用に使用
        BaseObject *primaryObject = GetSelectedTarget();
        if (primaryObject) {
            WorldTransform *transform = primaryObject->GetWorldTransform();
            if (transform) {
                DisplayGizmo(transform); // 複数選択に対応したギズモ表示
            }
        }
    }
}

void ImGuizmoManager::ShowSelectedObjectImGui() {
    if (selectedNames.empty()) {
        return;
    }

    // 複数選択の場合は最初のオブジェクトの詳細を表示
    std::string firstName = *selectedNames.begin();
    auto it = transformMap.find(firstName);
    if (it != transformMap.end() && it->second) {
        it->second->ImGui();
    }

    // 複数選択時は追加情報を表示
    if (selectedNames.size() > 1) {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
        ImGui::Text("※ %zu個のオブジェクトが選択されています", selectedNames.size());
        ImGui::Text("表示しているのは '%s' の設定です", firstName.c_str());
        ImGui::PopStyleColor();
    }
}

// void ImGuizmoManager::DeleteSelectedObject() {
//     if (selectedName.empty()) {
//         return; // 何も選択されていない場合は何もしない
//     }
//
//     // BaseObjectManagerから削除
//     BaseObjectManager::GetInstance()->RemoveObject(selectedName);
//
//     // ImGuizmoManagerの管理からも削除
//     auto it = transformMap.find(selectedName);
//     if (it != transformMap.end()) {
//         transformMap.erase(it);
//     }
//
//     // 選択をクリア
//     selectedName.clear();
//
//     // 他にオブジェクトがある場合は最初のものを選択
//     if (!transformMap.empty()) {
//         selectedName = transformMap.begin()->first;
//     }
// }

void ImGuizmoManager::HandleMouseSelection(const ImVec2 &scenePosition, const ImVec2 &sceneSize) {
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isInScene = (mousePos.x >= scenePosition.x && mousePos.x <= scenePosition.x + sceneSize.x &&
                      mousePos.y >= scenePosition.y && mousePos.y <= scenePosition.y + sceneSize.y);

    // ギズモ操作中、シーン外、マウスクリックなし、ViewProjectionなしの場合はスキップ
    if (ImGuizmo::IsUsing() || !isInScene || !Input::IsTriggerMouse(0) || !viewProjection) {
        return;
    }

    // Ctrlキーの状態確認
    bool isCtrlPressed = Input::GetInstance()->PushKey(DIK_LCONTROL);

    // レイキャストによる選択判定
    Ray currentRay = Input::GetInstance()->GetCurrentRay();
    float minDistance = std::numeric_limits<float>::max();
    std::string pickedName;
    bool foundHit = false;

    // 各オブジェクトに対してレイキャスト判定
    for (const auto &pair : transformMap) {
        BaseObject *obj = pair.second;
        if (!obj)
            continue;

        // ギズモ選択可能フラグをチェック
        if (!obj->IsGizmoSelectable()) {
            continue; // 選択不可のオブジェクトはスキップ
        }

        // 複数選択時：既に選択済みのオブジェクトは判定対象から除外
        if (isMultiSelecting && selectedNames.find(pair.first) != selectedNames.end()) {
            continue;
        }

        AABB aabb;
        aabb.min = {-1.3f, -1.3f, -1.3f};
        aabb.max = {1.3f, 1.3f, 1.3f};

        bool hit = Input::RayIntersectAABB(currentRay, obj, hitInfo, aabb);

        if (!hit) {
            Sphere sphere;
            sphere.center = {0.0f, 0.0f, 0.0f};
            sphere.radius = 1.3f;
            hit = Input::RayIntersectSphere(currentRay, obj, hitInfo, sphere);
        }

        if (hit && hitInfo.distance < minDistance) {
            minDistance = hitInfo.distance;
            pickedName = pair.first;
            foundHit = true;
        }
    }

    // 結果に応じて選択を更新
    if (foundHit && !pickedName.empty()) {
        if (isCtrlPressed) {
            // Ctrlが押されている場合は複数選択モード
            if (selectedNames.find(pickedName) != selectedNames.end()) {
                // 既に選択されている場合は選択解除
                selectedNames.erase(pickedName);
            } else {
                // 新しく選択に追加
                selectedNames.insert(pickedName);
            }
            isMultiSelecting = true;
        } else {
            // Ctrlが押されていない場合
            if (!isMultiSelecting) {
                // 通常の単一選択モード
                selectedNames.clear();
                selectedNames.insert(pickedName);
            } else {
                // 複数選択中でCtrlが押されていない場合：新しいオブジェクトに切り替え
                selectedNames.clear();
                selectedNames.insert(pickedName);
                isMultiSelecting = false;
            }
        }
    } else {
        // 何もヒットしなかった場合
        if (!isCtrlPressed) {
            // Ctrlが押されていない場合は選択解除
            selectedNames.clear();
            isMultiSelecting = false;
        }
        // Ctrlが押されている場合は現在の選択状態を維持
    }

    // Ctrlキーが離されたら複数選択モードを終了（選択状態は維持）
    if (!isCtrlPressed && isMultiSelecting) {
        isMultiSelecting = false;
    }
}
void ImGuizmoManager::DisplayGizmo(WorldTransform *transform) {
    if (!transform || !viewProjection)
        return;

    std::vector<BaseObject *> selectedObjects = GetSelectedTargets();
    if (selectedObjects.empty())
        return;

    // 複数選択の場合は重心を計算してギズモを表示
    // Ctrlキーの状態に関係なく、選択されている全オブジェクトを対象とする
    Vector3 centerPos = Vector3(0, 0, 0);
    for (BaseObject *obj : selectedObjects) {
        centerPos = centerPos + obj->GetWorldPosition();
    }
    centerPos = centerPos / static_cast<float>(selectedObjects.size());

    // 重心位置でのギズモ表示用の仮想行列を作成
    Matrix4x4 centerMatrix = MakeIdentity4x4();
    centerMatrix.m[3][0] = centerPos.x;
    centerMatrix.m[3][1] = centerPos.y;
    centerMatrix.m[3][2] = centerPos.z;

    float matrixArray[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            matrixArray[i * 4 + j] = centerMatrix.m[i][j];
        }
    }

    float viewArray[16], projArray[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            viewArray[i * 4 + j] = viewProjection->matView_.m[i][j];
            projArray[i * 4 + j] = viewProjection->matProjection_.m[i][j];
        }
    }

    // ギズモ操作（Ctrlキーの状態に関係なく動作）
    if (ImGuizmo::Manipulate(viewArray, projArray, currentOperation, currentMode, matrixArray)) {
        Matrix4x4 newMatrix;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                newMatrix.m[i][j] = matrixArray[i * 4 + j];
            }
        }

        // 移動量を計算
        Vector3 deltaPos = Vector3(
            newMatrix.m[3][0] - centerMatrix.m[3][0],
            newMatrix.m[3][1] - centerMatrix.m[3][1],
            newMatrix.m[3][2] - centerMatrix.m[3][2]);

        // 選択されている全オブジェクトに変更を適用
        for (BaseObject *obj : selectedObjects) {
            Vector3 newPos = obj->GetLocalPosition() + deltaPos;
            obj->GetLocalPosition() = newPos;

            WorldTransform *objTransform = obj->GetWorldTransform();
            if (objTransform) {
                objTransform->translation_ = obj->GetLocalPosition();
                objTransform->UpdateMatrix();
                obj->UpdateWorldTransformHierarchy();
            }
        }
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

// void ImGuizmoManager::CopySelectedObject() {
//     copiedObject = GetSelectedTarget();
// }
//
// void ImGuizmoManager::PasteObject() {
//     if (!copiedObject) {
//         return; // コピーされたオブジェクトがない場合は何もしない
//     }
//
//     // 新しいオブジェクトを作成
//     std::unique_ptr<BaseObject> newObject = std::make_unique<BaseObject>();
//
//     newObject->SetPrimitive(copiedObject->IsPrimitive());
//     // コピー元の基本データをコピー
//     newObject->Init(copiedObject->GetName());
//
//     // モデルとテクスチャをコピー
//     if (!copiedObject->GetModelPath().empty()) {
//         newObject->CreateModel(copiedObject->GetModelPath());
//     } else if (copiedObject->GetPrimitiveType() != PrimitiveType::kCount) {
//         newObject->CreatePrimitiveModel(copiedObject->GetPrimitiveType());
//     }
//
//     if (!copiedObject->GetTexturePath().empty()) {
//         newObject->SetTexture(copiedObject->GetTexturePath());
//     }
//     // 変換データをコピー
//     newObject->GetLocalPosition() = copiedObject->GetLocalPosition();
//     newObject->GetLocalRotation() = copiedObject->GetLocalRotation();
//     newObject->GetLocalScale() = copiedObject->GetLocalScale();
//
//     // 位置を少しずらす（重複を避けるため）
//     newObject->GetLocalPosition().x += 1.0f;
//
//     // その他の設定をコピー
//     newObject->GetLighting() = copiedObject->GetLighting();
//     newObject->GetLoop() = copiedObject->GetLoop();
//
//     // 一意な名前を生成
//     std::string baseName = copiedObject->GetName();
//     std::string uniqueName = GenerateUniqueName(baseName);
//     newObject->GetName() = uniqueName;
//     newObject->SetColor(copiedObject->GetColor());
//
//     // BaseObjectManagerに追加
//     BaseObjectManager::GetInstance()->AddObject(std::move(newObject));
//
//     // 新しいオブジェクトを選択
//     selectedName = uniqueName;
//
//     copiedObject = nullptr; // コピー状態をクリア
// }

std::string ImGuizmoManager::GenerateUniqueName(const std::string &baseName) {
    std::string newName;
    int counter = 1;

    // ベース名がすでに数字で終わっている場合の処理
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
        if (isNumber) {
            cleanBaseName = baseName.substr(0, underscorePos);
        }
    }

    // ユニークな名前を見つけるまでループ
    do {
        newName = cleanBaseName + "_" + std::to_string(counter);
        counter++;
    } while (transformMap.find(newName) != transformMap.end());

    return newName;
}

void ImGuizmoManager::DrawDebugRaycast() {
    if (!showDebugRaycast)
        return;

    Ray currentRay = Input::GetInstance()->GetCurrentRay();
    Vector3 rayEnd = currentRay.origin + (currentRay.direction * currentRay.length);
    DrawLine3D::GetInstance()->SetPoints(currentRay.origin, rayEnd, {1.0f, 0.0f, 0.0f, 1.0f});

    // 各オブジェクトの判定形状を描画
    for (const auto &pair : transformMap) {
        BaseObject *obj = pair.second;
        if (!obj)
            continue;

        Matrix4x4 worldMatrix = obj->GetWorldTransform()->matWorld_;

        // 選択状態に応じた色分け
        bool isSelected = selectedNames.find(pair.first) != selectedNames.end();
        Vector4 aabbColor = isSelected ? Vector4{1.0f, 1.0f, 0.0f, 1.0f} : // 選択中：黄
                                Vector4{0.0f, 0.0f, 1.0f, 1.0f};           // 通常：青

        Vector4 sphereColor = isSelected ? Vector4{1.0f, 0.5f, 0.0f, 1.0f} : // 選択中：オレンジ
                                  Vector4{1.0f, 0.0f, 1.0f, 1.0f};           // 通常：マゼンタ

        DrawAABBWireframe(worldMatrix, aabbColor);
        DrawSphereWireframe(worldMatrix, sphereColor);
        TestAndDrawRayHit(currentRay, obj, pair.first);
    }
}

void ImGuizmoManager::DrawAABBWireframe(const Matrix4x4 &worldMatrix, const Vector4 &color) {
    AABB aabb;
    aabb.min = {-1.3f, -1.3f, -1.3f};
    aabb.max = {1.3f, 1.3f, 1.3f};

    // AABBの8つの頂点を計算
    Vector3 vertices[8] = {
        // 下面の4頂点
        {aabb.min.x, aabb.min.y, aabb.min.z}, // 0: 左下奥
        {aabb.max.x, aabb.min.y, aabb.min.z}, // 1: 右下奥
        {aabb.max.x, aabb.min.y, aabb.max.z}, // 2: 右下手前
        {aabb.min.x, aabb.min.y, aabb.max.z}, // 3: 左下手前
        // 上面の4頂点
        {aabb.min.x, aabb.max.y, aabb.min.z}, // 4: 左上奥
        {aabb.max.x, aabb.max.y, aabb.min.z}, // 5: 右上奥
        {aabb.max.x, aabb.max.y, aabb.max.z}, // 6: 右上手前
        {aabb.min.x, aabb.max.y, aabb.max.z}  // 7: 左上手前
    };

    // 各頂点をワールド座標に変換
    for (int i = 0; i < 8; i++) {
        vertices[i] = Transformation(vertices[i], worldMatrix);
    }

    // ワイヤーフレームの線を描画（12本の辺）
    // 下面の4辺
    DrawLine3D::GetInstance()->SetPoints(vertices[0], vertices[1], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[1], vertices[2], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[2], vertices[3], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[3], vertices[0], color);

    // 上面の4辺
    DrawLine3D::GetInstance()->SetPoints(vertices[4], vertices[5], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[5], vertices[6], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[6], vertices[7], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[7], vertices[4], color);

    // 縦の4辺
    DrawLine3D::GetInstance()->SetPoints(vertices[0], vertices[4], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[1], vertices[5], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[2], vertices[6], color);
    DrawLine3D::GetInstance()->SetPoints(vertices[3], vertices[7], color);
}

void ImGuizmoManager::DrawSphereWireframe(const Matrix4x4 &worldMatrix, const Vector4 &color) {
    // 既定のSphere
    Sphere sphere{};

    // ワールド座標での中心と半径を計算
    Vector3 worldCenter = Transformation(sphere.center, worldMatrix);

    // スケールを考慮した半径
    Vector3 scale = {
        sqrt(worldMatrix.m[0][0] * worldMatrix.m[0][0] + worldMatrix.m[1][0] * worldMatrix.m[1][0] + worldMatrix.m[2][0] * worldMatrix.m[2][0]),
        sqrt(worldMatrix.m[0][1] * worldMatrix.m[0][1] + worldMatrix.m[1][1] * worldMatrix.m[1][1] + worldMatrix.m[2][1] * worldMatrix.m[2][1]),
        sqrt(worldMatrix.m[0][2] * worldMatrix.m[0][2] + worldMatrix.m[1][2] * worldMatrix.m[1][2] + worldMatrix.m[2][2] * worldMatrix.m[2][2])};
    float worldRadius = sphere.radius * std::max({scale.x, scale.y, scale.z});

    // DrawSphere関数を使用（既存の関数）
    DrawLine3D::GetInstance()->DrawSphere(worldCenter, color, worldRadius, 16);
}

void ImGuizmoManager::TestAndDrawRayHit(const Ray &ray, BaseObject *targetObject, const std::string &objectName) {
    RayHitInfo aabbHit, sphereHit;

    AABB aabb;
    aabb.min = {-1.3f, -1.3f, -1.3f};
    aabb.max = {1.3f, 1.3f, 1.3f};
    Sphere sphere;
    sphere.center = {0.0f, 0.0f, 0.0f};
    sphere.radius = 1.3f;

    // AABB判定テスト
    bool aabbResult = Input::RayIntersectAABB(ray, targetObject, aabbHit, aabb);

    // Sphere判定テスト
    bool sphereResult = Input::RayIntersectSphere(ray, targetObject, sphereHit, sphere);

    // ヒット点を描画
    if (aabbResult) {
        // AABBのヒット点を小さな緑の球で表示
        DrawLine3D::GetInstance()->DrawSphere(aabbHit.hitPoint, {0.0f, 1.0f, 0.0f, 1.0f}, 0.05f, 8);

        // ヒット点から法線方向に線を描画
        Vector3 normalEnd = aabbHit.hitPoint + (aabbHit.hitNormal * 0.3f);
        DrawLine3D::GetInstance()->SetPoints(aabbHit.hitPoint, normalEnd, {0.0f, 1.0f, 0.0f, 1.0f});
    }

    if (sphereResult) {
        // Sphereのヒット点を小さなマゼンタの球で表示
        DrawLine3D::GetInstance()->DrawSphere(sphereHit.hitPoint, {1.0f, 0.0f, 1.0f, 1.0f}, 0.05f, 8);

        // ヒット点から法線方向に線を描画
        Vector3 normalEnd = sphereHit.hitPoint + (sphereHit.hitNormal * 0.3f);
        DrawLine3D::GetInstance()->SetPoints(sphereHit.hitPoint, normalEnd, {1.0f, 0.0f, 1.0f, 1.0f});
    }
}

void ImGuizmoManager::CopySelectedObjects() {
    copiedObjects.clear();
    for (const std::string &name : selectedNames) {
        auto it = transformMap.find(name);
        if (it != transformMap.end()) {
            copiedObjects.push_back(it->second);
        }
    }
}

void ImGuizmoManager::PasteObjects() {
    if (copiedObjects.empty())
        return;

    selectedNames.clear(); // 新しくペーストされるオブジェクトを選択状態に

    for (BaseObject *copiedObj : copiedObjects) {
        // 新しいオブジェクトを作成
        std::unique_ptr<BaseObject> newObject = std::make_unique<BaseObject>();
        newObject->SetPrimitive(copiedObj->IsPrimitive());
        newObject->Init(copiedObj->GetName());

        // モデルとテクスチャをコピー
        if (!copiedObj->GetModelPath().empty()) {
            newObject->CreateModel(copiedObj->GetModelPath());
        } else if (copiedObj->GetPrimitiveType() != PrimitiveType::kCount) {
            newObject->CreatePrimitiveModel(copiedObj->GetPrimitiveType());
        }

        if (!copiedObj->GetTexturePath().empty()) {
            newObject->SetTexture(copiedObj->GetTexturePath());
        }

        // 変換データをコピー
        newObject->GetLocalPosition() = copiedObj->GetLocalPosition();
        newObject->GetLocalRotation() = copiedObj->GetLocalRotation();
        newObject->GetLocalScale() = copiedObj->GetLocalScale();

        // 位置を少しずらす
        newObject->GetLocalPosition().x += 1.0f;

        // その他の設定をコピー
        newObject->GetLighting() = copiedObj->GetLighting();
        newObject->GetLoop() = copiedObj->GetLoop();
        newObject->SetColor(copiedObj->GetColor());

        // 一意な名前を生成
        std::string uniqueName = GenerateUniqueName(copiedObj->GetName());
        newObject->GetName() = uniqueName;

        // BaseObjectManagerに追加
        BaseObjectManager::GetInstance()->AddObject(std::move(newObject));

        // **重要：ImGuizmoManagerにも追加**
        // BaseObjectManagerから追加されたオブジェクトを取得してtransformMapに登録
        BaseObject *addedObject = BaseObjectManager::GetInstance()->GetObjectByName(uniqueName);
        if (addedObject) {
            AddTarget(uniqueName, addedObject);
        }

        // 新しいオブジェクトを選択状態に追加
        selectedNames.insert(uniqueName);
    }

    // コピー状態をクリア
    copiedObjects.clear();
}

void ImGuizmoManager::DeleteSelectedObjects() {
    if (selectedNames.empty())
        return;

    // 選択されたオブジェクトを全て削除
    for (const std::string &name : selectedNames) {
        BaseObjectManager::GetInstance()->RemoveObject(name);
        transformMap.erase(name);
    }

    // フィルタリストを更新
    UpdateFilteredNames();

    // 選択をクリア
    selectedNames.clear();

    // 他にオブジェクトがある場合は最初のものを選択
    if (!transformMap.empty()) {
        selectedNames.insert(transformMap.begin()->first);
    }
}

void ImGuizmoManager::DrawSelectedObjectHighlight() {
    if (selectedNames.empty() || !viewProjection)
        return;

    for (const std::string &selectedName : selectedNames) {
        auto it = transformMap.find(selectedName);
        if (it == transformMap.end() || !it->second)
            continue;

        BaseObject *obj = it->second;
        Matrix4x4 worldMatrix = obj->GetWorldTransform()->matWorld_;

        // 選択ハイライト用の色（明るいオレンジ）
        Vector4 highlightColor = {1.0f, 0.6f, 0.0f, 1.0f};

        // 選択されたオブジェクトの上に選択マーカーを描画
        DrawSelectionMarker(obj->GetWorldPosition());
    }
}

void ImGuizmoManager::DrawSelectionMarker(const Vector3 &worldPosition) {
    // オブジェクトの上に浮かぶ選択マーカー（逆ピラミッド型）
    Vector3 markerPos = worldPosition + Vector3(0.0f, 2.0f, 0.0f); // オブジェクトの上方
    Vector4 markerColor = {1.0f, 1.0f, 0.0f, 1.0f};                // 明るい黄色
    float markerSize = 0.5f;

    // 逆ピラミッド型のマーカーを描画（とがった頂点が下向き）
    Vector3 apex = markerPos - Vector3(0.0f, markerSize, 0.0f); // 下向きの頂点
    Vector3 topLeft = markerPos + Vector3(-markerSize, markerSize, -markerSize);
    Vector3 topRight = markerPos + Vector3(markerSize, markerSize, -markerSize);
    Vector3 topFront = markerPos + Vector3(-markerSize, markerSize, markerSize);
    Vector3 topBack = markerPos + Vector3(markerSize, markerSize, markerSize);

    // 頂点から各コーナーへの線
    DrawLine3D::GetInstance()->SetPoints(apex, topLeft, markerColor);
    DrawLine3D::GetInstance()->SetPoints(apex, topRight, markerColor);
    DrawLine3D::GetInstance()->SetPoints(apex, topFront, markerColor);
    DrawLine3D::GetInstance()->SetPoints(apex, topBack, markerColor);

    // 上部の四角形の辺
    DrawLine3D::GetInstance()->SetPoints(topLeft, topRight, markerColor);
    DrawLine3D::GetInstance()->SetPoints(topRight, topBack, markerColor);
    DrawLine3D::GetInstance()->SetPoints(topBack, topFront, markerColor);
    DrawLine3D::GetInstance()->SetPoints(topFront, topLeft, markerColor);
}

void ImGuizmoManager::UpdateFilteredNames() {
    filteredNames_.clear();

    // transformMapから全ての名前を取得
    std::vector<std::string> allNames;
    for (const auto &pair : transformMap) {
        allNames.push_back(pair.first);
    }

    // 昇順でソート
    std::sort(allNames.begin(), allNames.end());

    // 検索フィルタを適用
    std::string searchStr = searchBuffer_;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    for (const std::string &name : allNames) {
        if (strlen(searchBuffer_) == 0) {
            // 検索文字列が空の場合は全て表示
            filteredNames_.push_back(name);
        } else {
            // 名前を小文字にして部分一致検索
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            if (lowerName.find(searchStr) != std::string::npos) {
                filteredNames_.push_back(name);
            }
        }
    }
}

#endif // _DEBUG