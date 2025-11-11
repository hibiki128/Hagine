#define NOMINMAX
#include "BaseObject.h"
#include "Scene/SceneManager.h"
#include "ShowFolder/ShowFolder.h"

void BaseObject::Init(const std::string objectName) {
    transform_ = std::make_unique<WorldTransform>();
    obj3d_ = std::make_unique<Object3d>();
    obj3d_->Initialize();
    objectName_ = objectName;
    /// ワールドトランスフォームの初期化
    transform_->Initialize();
    // ライティングのセット
    isLighting_ = true;
    isCollider = false;
    isAlive_ = true;
}

void BaseObject::Update() {
    if (obj3d_->GetHaveAnimation()) {
        obj3d_->AnimationUpdate(isLoop_);
    }
    SetBlendMode(blendMode_);
}

void BaseObject::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    // オフセットを適用する場合は、一時的にローカル位置を変更
    Vector3 originalPosition = transform_->translation_;

    if (offSet.x != 0.0f || offSet.y != 0.0f || offSet.z != 0.0f) {
        transform_->translation_ = originalPosition + offSet;
        // オフセット適用時は行列を更新
        transform_->UpdateMatrix();
    }

    // スケルトンの描画が必要な場合
    if (skeletonDraw_) {
        obj3d_->DrawSkeleton(*transform_, viewProjection);
    }
    if (!isWireframe_) {
        // オブジェクトの描画
        obj3d_->Draw(*transform_, viewProjection, reflect_, isLighting_, isModelDraw_);
    } else {
        obj3d_->DrawWireframe(*transform_, viewProjection, isRainbow_);
    }

    // オフセットを適用した場合は元の位置に戻す
    if (offSet.x != 0.0f || offSet.y != 0.0f || offSet.z != 0.0f) {
        transform_->translation_ = originalPosition;
        // 元の位置に戻した後も行列を更新
        transform_->UpdateMatrix();
    }
}

void BaseObject::UpdateWorldTransformHierarchy() {
    // まず自分のトランスフォームを更新
    if (transform_) {
        transform_->UpdateMatrix();
    }
    // 子を再帰的に更新
    for (auto it = children_.begin(); it != children_.end();) {
        BaseObject *child = *it;
        child->UpdateWorldTransformHierarchy();
        if (child->parent_ != this) {
            it = children_.erase(it);
        } else {
            ++it;
        }
    }
}

void BaseObject::UpdateHierarchy() {
    // 自分自身の処理
    Update();

    // 子リストをイテレート
    for (auto it = children_.begin(); it != children_.end();) {
        auto child = *it;
        // 再帰的に UpdateHierarchy
        child->UpdateHierarchy();

        // 子が「DetachParent()」した場合、parent_ == nullptr になる
        if (child->GetParent() != this) {
            // リストから削除
            it = children_.erase(it);
        } else {
            ++it;
        }
    }
}

void BaseObject::SetParent(BaseObject *parent) {
    if (parent_ == parent || parent == nullptr) {
        return; // 同じ親を持ってる場合何もしない
    }
    if (parent_) {
        DetachParent(); // もし現在の親がいるなら一旦デタッチ
    }

    assert(parent != nullptr && "SetParent to nullptr is not allowed.");

    parent_ = parent;
    // 親の子リストに追加
    parent_->children_.push_back(this);

    if (transform_) {
        transform_->parent_ = parent->GetWorldTransform();
    }
    parentName_ = parent_->GetName();
}

void BaseObject::AddChild(BaseObject *child) {
    assert(child != nullptr && "AddChild is nullptr");
    child->SetParent(this);
}

void BaseObject::DetachParent() {
    if (parent_) {
        parent_->children_.remove(this);
        parent_ = nullptr;
        if (transform_) {
            transform_->parent_ = nullptr;
        }
    }
}

void BaseObject::DetachChild(BaseObject *child) {
    if (!child) {
        return;
    }
    if (child->parent_ != this) {
        return;
    }
    child->parent_ = nullptr;
    if (child->transform_) {
        child->transform_->parent_ = nullptr;
    }
    children_.remove(child);
}

BaseObject *BaseObject::GetParent() {
    return parent_;
}

std::list<BaseObject *> *BaseObject::GetChildren() {
    return &children_;
}

BaseObject *BaseObject::GetChildByName(const std::string &name) {
    for (auto &child : children_) {
        if (child->objectName_ == name) {
            return child;
        }
    }
    return nullptr;
}

void BaseObject::CreateModel(const std::string modelname) {
    modelPath_ = modelname;
    isPrimitive_ = false;

    obj3d_->CreateModel(modelname);

    // テクスチャパスを3Dモデル用にリサイズ
    texturePaths_.resize(obj3d_->GetMaterialCount());

    // デフォルトのテクスチャパスを設定
    auto allTexturePaths = obj3d_->GetAllTextruePath();
    for (int i = 0; i < texturePaths_.size() && i < allTexturePaths.size(); i++) {
        texturePaths_[i] = allTexturePaths[i];
    }

    // JSONファイルが存在する場合は読み込み（modelPath_は上書きされない）
    if (isScene_) {
        LoadFromJson();
    } else {
        LoadFromJson("ObjectDatas", objectName_);
    }

    // JSONから読み込んだカラー設定を適用
    if (ObjectDatas_) {
        for (int i = 0; i < int(obj3d_->GetMaterialCount()); i++) {
            SetColor(ObjectDatas_->Load<Vector4>("color_" + std::to_string(i), GetColor(i)), i);
        }
    }

    // テクスチャを設定
    for (int i = 0; i < texturePaths_.size(); i++) {
        obj3d_->SetTexture(texturePaths_[i], i);
    }

    AnimaLoadFromJson();
}

void BaseObject::CreatePrimitiveModel(const PrimitiveType &type) {
    modelPath_ = ""; // プリミティブの場合は空文字列
    isPrimitive_ = true;
    type_ = type;

    // プリミティブ用にテクスチャパスを設定（1枚のみ）
    texturePaths_.resize(1);
    texturePaths_[0] = "debug/uvChecker.png"; // デフォルト値

    // JSONファイルが存在する場合は読み込み
    if (isScene_) {
        LoadFromJson();
    } else {
        LoadFromJson("ObjectDatas", objectName_);
    }

    // プリミティブモデルを作成
    obj3d_->CreatePrimitiveModel(type_, texturePaths_[0]);

    SetColor(ObjectDatas_->Load<Vector4>("color_" + std::to_string(0), {1.0f, 1.0f, 1.0f, 1.0f}), 0);

    AnimaLoadFromJson();
}

void BaseObject::AddCollider() {
    colliders_.push_back(&Collider::AddCollider(objectName_));
    isCollider = true;
}

void BaseObject::SaveParentChildRelationship() {
    if (!ObjectDatas_) {
        return;
    }

    // 親の名前を保存
    std::string parentName = parent_ ? parent_->GetName() : "";
    ObjectDatas_->Save<std::string>("parentName", parentName);

    // 子の名前リストを保存
    std::vector<std::string> childrenNames;
    for (const auto &child : children_) {
        if (child) {
            childrenNames.push_back(child->GetName());
        }
    }
    ObjectDatas_->Save<std::vector<std::string>>("childrenNames", childrenNames);
}

void BaseObject::LoadParentChildRelationship() {
    if (!ObjectDatas_) {
        return;
    }

    // 親の名前を読み込み（実際の親付けはBaseObjectManagerで行う）
    std::string parentName = ObjectDatas_->Load<std::string>("parentName", "");

    // 子の名前リストを読み込み（実際の子付けはBaseObjectManagerで行う）
    std::vector<std::string> childrenNames = ObjectDatas_->Load<std::vector<std::string>>("childrenNames", std::vector<std::string>());
}

std::string BaseObject::GetParentName() const {
    return parent_ ? parentName_ : "";
}

std::vector<std::string> BaseObject::GetChildrenNames() const {
    std::vector<std::string> names;
    for (const auto &child : children_) {
        if (child) {
            names.push_back(child->GetName());
        }
    }
    return names;
}

Vector3 BaseObject::GetWorldPosition() {
    return transform_->GetWorldPosition();
}

// ワールド行列からクォータニオンを取得
Quaternion BaseObject::GetWorldRotation() {
    return transform_->GetWorldRotation();
}

// ワールドスケールを取得（回転を考慮）
Vector3 BaseObject::GetWorldScale() {
    return transform_->GetWorldScale();
}

void BaseObject::SaveToJson() {
    // JSONデータを扱うハンドラを作成
    modelPath_ = obj3d_->GetModelFilePath();
    ObjectDatas_ = std::make_unique<DataHandler>("ObjectDatas", objectName_);
    ObjectDatas_->Save<std::string>("modelName", modelPath_);
    ObjectDatas_->Save<std::string>("objectName", objectName_);
    ObjectDatas_->Save<Vector3>("translation", transform_->translation_);
    ObjectDatas_->Save<Quaternion>("rotation", transform_->quateRotation_);
    ObjectDatas_->Save<Vector3>("scale", transform_->scale_);
    ObjectDatas_->Save<bool>("Lighting", isLighting_);
    ObjectDatas_->Save<PrimitiveType>("PrimitiveType", type_);
    ObjectDatas_->Save<bool>("skeletonDraw", skeletonDraw_);
    ObjectDatas_->Save<bool>("isModelDraw", isModelDraw_);
    if (parent_) {
        ObjectDatas_->Save<std::string>("parentName", parent_->GetName());
    }
    for (int i = 0; i < int(obj3d_->GetMaterialCount()); i++) {
        texturePaths_.push_back(obj3d_->GetTextureFilePath(i));
        ObjectDatas_->Save<std::string>("textureName_" + std::to_string(i), texturePaths_[i]);
        ObjectDatas_->Save("color_" + std::to_string(i), GetColor(i));
    }

    ObjectDatas_->Save<bool>("isLighting", isLighting_);

    ObjectDatas_->Save<int>("blendMode", static_cast<int>(blendMode_));

    SaveParentChildRelationship();
}

void BaseObject::SceneSaveToJson() {
    // JSONデータを扱うハンドラを作成
    ObjectDatas_ = std::make_unique<DataHandler>(foldarPath_, objectName_);
    modelPath_ = obj3d_->GetModelFilePath();
    ObjectDatas_->Save<std::string>("modelName", modelPath_);
    ObjectDatas_->Save<std::string>("objectName", objectName_);
    ObjectDatas_->Save<Vector3>("translation", transform_->translation_);
    ObjectDatas_->Save<Quaternion>("rotation", transform_->quateRotation_);
    ObjectDatas_->Save<Vector3>("scale", transform_->scale_);
    ObjectDatas_->Save<bool>("Lighting", isLighting_);
    ObjectDatas_->Save<PrimitiveType>("PrimitiveType", type_);
    ObjectDatas_->Save<bool>("skeletonDraw", skeletonDraw_);
    ObjectDatas_->Save<bool>("isModelDraw", isModelDraw_);
    if (parent_) {
        ObjectDatas_->Save<std::string>("parentName", parent_->GetName());
    }

    for (int i = 0; i < int(obj3d_->GetMaterialCount()); i++) {
        texturePaths_.push_back(obj3d_->GetTextureFilePath(i));
        ObjectDatas_->Save<std::string>("textureName_" + std::to_string(i), texturePaths_[i]);
        ObjectDatas_->Save("color_" + std::to_string(i), GetColor(i));
    }

    ObjectDatas_->Save<bool>("isLighting", isLighting_);

    ObjectDatas_->Save<int>("blendMode", static_cast<int>(blendMode_));

    SaveParentChildRelationship();
}

void BaseObject::LoadFromJson() {
    // JSONデータを扱うハンドラを作成
    ObjectDatas_ = std::make_unique<DataHandler>(foldarPath_, objectName_);

    // 基本トランスフォームを読み込み
    transform_->translation_ = ObjectDatas_->Load<Vector3>("translation", {0.0f, 0.0f, 0.0f});
    transform_->quateRotation_ = ObjectDatas_->Load<Quaternion>("rotation", Quaternion::IdentityQuaternion());
    transform_->scale_ = ObjectDatas_->Load<Vector3>("scale", {1.0f, 1.0f, 1.0f});
    isLighting_ = ObjectDatas_->Load<bool>("Lighting", true);
    type_ = ObjectDatas_->Load<PrimitiveType>("PrimitiveType", PrimitiveType::kCount);
    skeletonDraw_ = ObjectDatas_->Load<bool>("skeletonDraw", false);
    isModelDraw_ = ObjectDatas_->Load<bool>("isModelDraw", true);
    parentName_ = ObjectDatas_->Load<std::string>("parentName", "");

    // モデルパスをJSONから読み込み（既に設定されている場合は上書きしない）
    std::string loadedModelPath = ObjectDatas_->Load<std::string>("modelName", "");
    if (!loadedModelPath.empty()) {
        modelPath_ = loadedModelPath;
    }

    // 現在のmodelPath_の状態でプリミティブかどうかを判断
    if (modelPath_.empty()) {
        // プリミティブの場合
        isPrimitive_ = true;
        if (texturePaths_.empty()) {
            texturePaths_.resize(1);
            texturePaths_[0] = ObjectDatas_->Load<std::string>("textureName_0", "debug/uvChecker.png");
        } else {
            texturePaths_[0] = ObjectDatas_->Load<std::string>("textureName_0", texturePaths_[0]);
        }
    } else {
        // 3Dモデルの場合
        isPrimitive_ = false;
        // obj3d_が既に作成されている場合のみテクスチャパスを読み込み
        if (obj3d_ && obj3d_->GetMaterialCount() > 0) {
            texturePaths_.resize(obj3d_->GetMaterialCount());
            for (int i = 0; i < texturePaths_.size(); i++) {
                texturePaths_[i] = ObjectDatas_->Load<std::string>("textureName_" + std::to_string(i), "debug/uvChecker.png");
            }
        }
    }

    isLighting_ = ObjectDatas_->Load<bool>("isLighting", true);
    blendMode_ = static_cast<BlendMode>(ObjectDatas_->Load<int>("blendMode", int(BlendMode::kNormal)));

    LoadParentChildRelationship();
}

void BaseObject::LoadFromJson(std::string folderPath, std::string jsonName) {
    // JSONデータを扱うハンドラを作成
    ObjectDatas_ = std::make_unique<DataHandler>(folderPath, jsonName);

    // 基本トランスフォームを読み込み
    transform_->translation_ = ObjectDatas_->Load<Vector3>("translation", {0.0f, 0.0f, 0.0f});
    transform_->quateRotation_ = ObjectDatas_->Load<Quaternion>("rotation", Quaternion::IdentityQuaternion());
    transform_->scale_ = ObjectDatas_->Load<Vector3>("scale", {1.0f, 1.0f, 1.0f});
    isLighting_ = ObjectDatas_->Load<bool>("Lighting", true);
    type_ = ObjectDatas_->Load<PrimitiveType>("PrimitiveType", type_);
    skeletonDraw_ = ObjectDatas_->Load<bool>("skeletonDraw", false);
    isModelDraw_ = ObjectDatas_->Load<bool>("isModelDraw", true);
    parentName_ = ObjectDatas_->Load<std::string>("parentName", "");

    // モデルパスをJSONから読み込み（既に設定されている場合は上書きしない）
    std::string loadedModelPath = ObjectDatas_->Load<std::string>("modelName", "");
    if (!loadedModelPath.empty()) {
        modelPath_ = loadedModelPath;
    }

    // 現在のmodelPath_の状態でプリミティブかどうかを判断
    if (modelPath_.empty()) {
        // プリミティブの場合
        isPrimitive_ = true;
        if (texturePaths_.empty()) {
            texturePaths_.resize(1);
            texturePaths_[0] = ObjectDatas_->Load<std::string>("textureName_0", "debug/uvChecker.png");
        } else {
            texturePaths_[0] = ObjectDatas_->Load<std::string>("textureName_0", texturePaths_[0]);
        }
    } else {
        // 3Dモデルの場合
        isPrimitive_ = false;
        // obj3d_が既に作成されている場合のみテクスチャパスを読み込み
        if (obj3d_ && obj3d_->GetMaterialCount() > 0) {
            texturePaths_.resize(obj3d_->GetMaterialCount());
            for (int i = 0; i < texturePaths_.size(); i++) {
                texturePaths_[i] = ObjectDatas_->Load<std::string>("textureName_" + std::to_string(i), texturePaths_[i]);
            }
        }
    }

    isLighting_ = ObjectDatas_->Load<bool>("isLighting", true);
    blendMode_ = static_cast<BlendMode>(ObjectDatas_->Load<int>("blendMode", int(BlendMode::kNormal)));

    LoadParentChildRelationship();
}

void BaseObject::AnimaSaveToJson() {
    if (!AnimaDatas_) {
        return;
    }
    AnimaDatas_->Save<bool>("Loop", isLoop_);
}

void BaseObject::AnimaLoadFromJson() {
    AnimaDatas_ = std::make_unique<DataHandler>("Animation", objectName_);
    isLoop_ = AnimaDatas_->Load<bool>("Loop", false);
}

void BaseObject::DebugCollider() {
    for (auto &collider : colliders_) {
        collider->OffsetImgui();
    }
}

Vector3 BaseObject::GetCenterPosition() {
    return GetWorldPosition();
}

Quaternion BaseObject::GetCenterRotation() {
    return GetWorldRotation();
}

void BaseObject::ImGui() {
#ifdef _DEBUG
    if (ImGui::BeginTabBar(objectName_.c_str())) {
        if (ImGui::BeginTabItem(objectName_.c_str())) {
            DebugObject();

            // コライダーの設定
            if (isCollider) {
                DebugCollider();
            }
            if (ImGui::Button("コライダー追加")) {
                AddCollider();
            }

            // モデル描画チェックボックス
            bool modelDrawChanged = ImGui::Checkbox("モデル描画", &isModelDraw_);
            if (modelDrawChanged && isModelDraw_) {
                // モデル描画がオンになったときはワイヤーフレームをオフにする
                isWireframe_ = false;
            }

            // ワイヤーフレームチェックボックス
            bool wireframeChanged = ImGui::Checkbox("ワイヤーフレーム", &isWireframe_);
            if (wireframeChanged && isWireframe_) {
                // ワイヤーフレームがオンになったときはモデル描画をオフにする
                isModelDraw_ = false;
            }
            if (isWireframe_) {
                ImGui::Checkbox("???", &isRainbow_);
            } else {
                isRainbow_ = false;
            }

            // セーブボタン
            if (ImGui::Button("セーブ")) {
                SaveToJson();
                AnimaSaveToJson();
                for (auto &collider : colliders_) {
                    collider->SaveToJson();
                }
                std::string message = std::format("ObjectData saved.");
                MessageBoxA(nullptr, message.c_str(), "Object", 0);
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

#endif // _DEBUG
}

void BaseObject::DebugObject() {
#ifdef _DEBUG

    // 全体のスタイル設定
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

    // === トランスフォーム設定 ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.6f, 0.8f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.7f, 0.9f, 0.4f));

    if (ImGui::CollapsingHeader("トランスフォーム", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10);

        // === ローカル座標 ===
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("ローカル座標:");
        ImGui::PopStyleColor();
        ImGui::Separator();

        // 位置設定
        ImGui::AlignTextToFramePadding();
        ImGui::Text("位置:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);
        ImGui::DragFloat3("##Position", &transform_->translation_.x, 0.1f, -1000.0f, 1000.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("リセット##ResetPos")) {
            transform_->translation_ = {0.0f, 0.0f, 0.0f};
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("位置をリセット");

        // 回転設定
        ImGui::AlignTextToFramePadding();
        ImGui::Text("回転:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);
        static Vector3 deltaRotation = {0.0f, 0.0f, 0.0f};
        if (ImGui::DragFloat3("##Rotation", &deltaRotation.x, 0.1f, -10.0f, 10.0f, "%.1f°")) {
            Quaternion currentRotation = transform_->GetRotationQuaternion();
            Quaternion deltaQuatX = Quaternion::FromAxisAngle(Vector3(1, 0, 0), deltaRotation.x * std::numbers::pi_v<float> / 180.0f);
            Quaternion deltaQuatY = Quaternion::FromAxisAngle(Vector3(0, 1, 0), deltaRotation.y * std::numbers::pi_v<float> / 180.0f);
            Quaternion deltaQuatZ = Quaternion::FromAxisAngle(Vector3(0, 0, 1), deltaRotation.z * std::numbers::pi_v<float> / 180.0f);
            Quaternion deltaQuat = deltaQuatY * deltaQuatX * deltaQuatZ;
            Quaternion newRotation = currentRotation * deltaQuat;
            transform_->SetRotationQuaternion(newRotation.Normalize());
            transform_->UpdateMatrix();
            deltaRotation = {0.0f, 0.0f, 0.0f};
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("リセット##ResetRot")) {
            transform_->SetRotationQuaternion(Quaternion::IdentityQuaternion());
            transform_->UpdateMatrix();
            deltaRotation = {0.0f, 0.0f, 0.0f};
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("回転をリセット");

        // 現在の回転を表示（参考用）
        ImGui::Text("現在の回転:");
        Vector3 currentEuler = transform_->GetRotationEuler();
        ImGui::Text("X: %.1f°, Y: %.1f°, Z: %.1f°",
                    currentEuler.x * 180.0f / std::numbers::pi_v<float>,
                    currentEuler.y * 180.0f / std::numbers::pi_v<float>,
                    currentEuler.z * 180.0f / std::numbers::pi_v<float>);

        // スケール設定
        ImGui::AlignTextToFramePadding();
        ImGui::Text("大きさ:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);
        ImGui::DragFloat3("##Scale", &transform_->scale_.x, 0.01f, 0.01f, 10.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("リセット##ResetScale")) {
            transform_->scale_ = {1.0f, 1.0f, 1.0f};
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("大きさをリセット");

        ImGui::Spacing();

        // === ワールド座標 ===
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 1.0f, 0.8f, 1.0f));
        ImGui::Text("ワールド座標:");
        ImGui::PopStyleColor();
        ImGui::Separator();

        // ワールド座標の取得
        Vector3 worldPos = GetWorldPosition();
        Quaternion worldRot = GetWorldRotation();
        Vector3 worldScale = GetWorldScale();

        // ワールド位置（読み取り専用）
        ImGui::AlignTextToFramePadding();
        ImGui::Text("位置:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        float worldPosArray[3] = {worldPos.x, worldPos.y, worldPos.z};
        ImGui::InputFloat3("##WorldPosition", worldPosArray, "%.2f", ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();

        // ワールド回転（読み取り専用、度数で表示）
        ImGui::AlignTextToFramePadding();
        ImGui::Text("回転:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

        // クォータニオンからワールド回転を取得
        float worldRotDegrees[3] = {
            radiansToDegrees(worldRot.x),
            radiansToDegrees(worldRot.y),
            radiansToDegrees(worldRot.z)};

        ImGui::InputFloat3("##WorldRotation", worldRotDegrees, "%.1f°", ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();

        // ワールドスケール（読み取り専用）
        ImGui::AlignTextToFramePadding();
        ImGui::Text("大きさ:");
        ImGui::SameLine(80);
        ImGui::PushItemWidth(200);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        float worldScaleArray[3] = {worldScale.x, worldScale.y, worldScale.z};
        ImGui::InputFloat3("##WorldScale", worldScaleArray, "%.2f", ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();

        ImGui::Unindent(10);
        ImGui::Spacing();
    }

    // === マテリアル設定 ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.4f, 0.2f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.5f, 0.3f, 0.4f));

    if (ImGui::CollapsingHeader("マテリアル設定")) {
        ImGui::Indent(10);
        // ライティング設定
        ImGui::Text("ライティング:");
        ImGui::SameLine(120);

        if (isLighting_) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 1.0f, 0.7f, 1.0f));
            ImGui::Text("有効");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.7f, 1.0f));
            ImGui::Text("無効");
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        if (ImGui::Button(isLighting_ ? "無効化" : "有効化")) {
            isLighting_ = !isLighting_;
        }

        ImGui::Unindent(10);
        ImGui::Spacing();
    }

    // === モデル設定 ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.2f, 0.8f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.7f, 0.3f, 0.9f, 0.4f));

    if (ImGui::CollapsingHeader("モデル設定")) {
        ImGui::Indent(10);

        static int selectedMaterialIndex = 0;
        size_t materialCount = obj3d_->GetMaterialCount();

        if (obj3d_->GetHaveAnimation() && materialCount > 1) {
            --materialCount;
        }

        // マテリアル選択
        if (materialCount > 1) {
            ImGui::Text("マテリアル:");
            ImGui::SameLine(120);

            std::vector<std::string> comboItems;
            for (int i = 0; i < static_cast<int>(materialCount); ++i) {
                comboItems.push_back("Material " + std::to_string(i + 1));
            }

            std::vector<const char *> comboItemsCStr;
            for (const auto &item : comboItems) {
                comboItemsCStr.push_back(item.c_str());
            }

            ImGui::PushItemWidth(150);
            if (ImGui::Combo("##MaterialIndex", &selectedMaterialIndex, comboItemsCStr.data(), static_cast<int>(comboItemsCStr.size()))) {
                // 選択変更時の処理
            }
            ImGui::PopItemWidth();

            selectedMaterialIndex = std::clamp(selectedMaterialIndex, 0, static_cast<int>(materialCount) - 1);
        } else {
            selectedMaterialIndex = 0;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::Text("マテリアル: Single Material");
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.7f, 0.3f));
        if (ImGui::TreeNode("マテリアル色設定")) {
            Vector4 currentColor = GetColor(selectedMaterialIndex);
            float color[4] = {currentColor.x, currentColor.y, currentColor.z, currentColor.w};

            if (ImGui::ColorEdit4("色", color)) {
                SetColor(Vector4(color[0], color[1], color[2], color[3]), selectedMaterialIndex);
            }

            ImGui::Spacing();
            if (ImGui::Button("リセット", ImVec2(80, 0))) {
                SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f), selectedMaterialIndex);
            }

            ImGui::TreePop();
        }
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // テクスチャ設定
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.7f, 0.3f, 0.3f));
        if (ImGui::TreeNode("テクスチャ設定")) {
            ShowTextureFile(texturePath_);
            ImGui::Spacing();

            if (ImGui::Button("適用", ImVec2(80, 0))) {
                SetTexture(texturePath_, selectedMaterialIndex);
                texturePaths_[selectedMaterialIndex] = texturePath_;
            }

            ImGui::SameLine();
            if (ImGui::Button("クリア", ImVec2(80, 0))) {
                texturePath_.clear();
            }

            ImGui::TreePop();
        }
        ImGui::PopStyleColor();

        // ブレンドモード設定
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.7f, 0.3f, 0.7f, 0.3f));
        if (ImGui::TreeNode("ブレンドモード")) {
            ShowBlendModeCombo(blendMode_);
            ImGui::TreePop();
        }
        ImGui::PopStyleColor();

        ImGui::Unindent(10);
        ImGui::Spacing();
    }

    // === アニメーション設定 ===
    if (obj3d_->GetHaveAnimation()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.6f, 0.2f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.7f, 0.3f, 0.4f));

        if (ImGui::CollapsingHeader("アニメーション設定")) {
            ImGui::Indent(10);

            // 制御オプション
            ImGui::Text("ループ:");
            ImGui::SameLine(80);
            ImGui::Checkbox("##Loop", &isLoop_);

            ImGui::Text("スケルトン:");
            ImGui::SameLine(100); // 幅を80から100に拡張
            ImGui::Checkbox("##Skeleton", &skeletonDraw_);

            ImGui::Spacing();

            // 制御ボタン
            if (ImGui::Button("再生", ImVec2(80, 0))) {
                obj3d_->PlayAnimation();
            }

            ImGui::Spacing();

            // アニメーションセット選択
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.8f, 0.2f, 0.3f));
            if (ImGui::TreeNode("アニメーションセット")) {
                ShowFileSelector();
                ImGui::TreePop();
            }
            ImGui::PopStyleColor();

            ImGui::Unindent(10);
        }

        ImGui::PopStyleColor(2);
    }

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(2);

    if (ImGui::CollapsingHeader("ギズモ設定")) {
        ImGui::Checkbox("ギズモで選択可能", &isGizmoSelectable_);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("オフにするとマウスクリックやギズモ操作の対象外になります");
        }
    }
#endif // _DEBUG
}

void BaseObject::ShowFileSelector() {
#ifdef _DEBUG

    static int selectedIndex = -1;                              // 選択中のインデックス（-1は未選択）
    static std::vector<std::string> gltfFiles = GetGltfFiles(); // GLTFファイルのリスト

    // ファイルリストをCスタイル文字列の配列に変換
    std::vector<const char *> fileNames;
    for (const auto &filePath : gltfFiles) {
        fileNames.push_back(filePath.c_str());
    }

    ImGui::Text("GLTFファイル選択");
    ImGui::Separator();

    // Comboボックスでファイル選択
    if (ImGui::Combo("GLTF Files", &selectedIndex, fileNames.data(), static_cast<int>(fileNames.size()))) {
        // ファイル選択時の動作（選択されたファイル名を表示）
        if (selectedIndex >= 0) {
            ImGui::Text("Selected File:");
            ImGui::TextWrapped("%s", gltfFiles[selectedIndex].c_str());
        }
    }

    // ボタンでアニメーションをセット
    if (selectedIndex >= 0 && ImGui::Button("Set Animation")) {
        obj3d_->SetAnimation(gltfFiles[selectedIndex]); // 選択されたファイルをSetAnimationに渡す
    }
#endif // _DEBUG
}

void BaseObject::ShowBlendModeCombo(BlendMode &currentMode) {
#ifdef _DEBUG

    // コンボボックスに表示する項目（日本語）
    static const char *blendModeItems[] = {
        "なし",      // kNone
        "通常",      // kNormal
        "加算",      // kAdd
        "減算",      // kSubtract
        "乗算",      // kMultiply
        "スクリーン" // kScreen
    };

    // 現在の選択状態（enumをintにキャスト）
    int currentIndex = static_cast<int>(currentMode);

    // コンボボックス表示
    if (ImGui::Combo("ブレンドモード", &currentIndex, blendModeItems, IM_ARRAYSIZE(blendModeItems))) {
        // ユーザーが選択を変更したときに反映
        currentMode = static_cast<BlendMode>(currentIndex);
    }
#endif // _DEBUG
}

std::vector<std::string> BaseObject::GetGltfFiles() {
    std::vector<std::string> gltfFiles;
    std::filesystem::path baseDir = "resources/models/animation"; // ベースディレクトリ
    for (const auto &entry : std::filesystem::directory_iterator(baseDir)) {
        if (entry.path().extension() == ".gltf") {
            // フルパスではなく相対パスを取得し、区切り文字をスラッシュに変更
            std::string relativePath = std::filesystem::relative(entry.path(), baseDir.parent_path()).string();
            std::replace(relativePath.begin(), relativePath.end(), '\\', '/'); // バックスラッシュをスラッシュに置換
            gltfFiles.push_back(relativePath);
        }
    }
    return gltfFiles;
}
