#define NOMINMAX
#include "BaseObject.h"
#include "Collider/CollisionManager.h"
#include "Engine/Utility/Debug/ImGui/Debugui_improved.h"
#include "Engine/Utility/Debug/ImGui/ImGuiNotification.h"
#include "Scene/SceneManager.h"
#include "ShowFolder/ShowFolder.h"
#ifdef _DEBUG
#include <imgui_internal.h>
#include <implot.h>
#endif // DEBUG

BaseObject::~BaseObject() {
    colliders_.clear();
}

void BaseObject::Init(const std::string objectName) {
    transform_ = std::make_unique<WorldTransform>();
    obj3d_ = std::make_unique<Object3d>();
    obj3d_->Initialize();
    objectName_ = objectName;
    /// ワールドトランスフォームの初期化
    transform_->Initialize();
    // ライティングのセット
    isLighting_ = true;
    isAlive_ = true;
}

void BaseObject::Update() {
    if (obj3d_->GetHaveAnimation()) {
        // ループフラグはアニメーションごとに Object3d が内部管理する
        obj3d_->AnimationUpdate();
    }
    SetBlendMode(blendMode_);
}

void BaseObject::Draw(const ViewProjection &viewProjection) {
    // オフセットを適用する場合は、一時的にローカル位置を変更
    Vector3 originalPosition = transform_->translation_;

    if (offSet_.x != 0.0f || offSet_.y != 0.0f || offSet_.z != 0.0f) {
        transform_->translation_ = originalPosition + offSet_;
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
    if (offSet_.x != 0.0f || offSet_.y != 0.0f || offSet_.z != 0.0f) {
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

    // コライダー情報を保存
    SaveColliders();
    ObjectDatas_->Flush();
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

    // コライダー情報を保存
    SaveColliders();
    ObjectDatas_->Flush();
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

    // コライダー情報を読み込み
    LoadColliders();
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

    // コライダー情報を読み込み
    LoadColliders();
}

void BaseObject::SaveColliders() {
    if (!ObjectDatas_) {
        return;
    }

    // コライダー数を保存
    ObjectDatas_->Save<int>("colliderCount", static_cast<int>(colliders_.size()));

    // 各コライダーの情報を保存
    for (size_t i = 0; i < colliders_.size(); ++i) {
        auto *collider = colliders_[i].get();
        if (!collider)
            continue;

        std::string prefix = "collider_" + std::to_string(i) + "_";

        // 共通情報
        ObjectDatas_->Save<std::string>(prefix + "name", collider->GetName());
        ObjectDatas_->Save<int>(prefix + "type", static_cast<int>(collider->GetType()));
        ObjectDatas_->Save<std::string>(prefix + "tag", collider->GetTag());
        ObjectDatas_->Save<bool>(prefix + "isEnabled", collider->IsEnabled());
        ObjectDatas_->Save<bool>(prefix + "isVisible", collider->IsVisible());

        // 衝突マスクを保存
        const auto &mask = collider->GetCollisionMask();
        std::vector<std::string> maskList(mask.begin(), mask.end());
        ObjectDatas_->Save<std::vector<std::string>>(prefix + "collisionMask", maskList);

        // 型別の詳細情報を保存
        if (auto *sphere = dynamic_cast<SphereCollider *>(collider)) {
            ObjectDatas_->Save<float>(prefix + "radius", sphere->GetRadius());
            ObjectDatas_->Save<Vector3>(prefix + "offset", sphere->GetOffset());
        } else if (auto *aabb = dynamic_cast<AABBCollider *>(collider)) {
            ObjectDatas_->Save<Vector3>(prefix + "size", aabb->GetSize());
            ObjectDatas_->Save<Vector3>(prefix + "offset", aabb->GetOffset());
        } else if (auto *obb = dynamic_cast<OBBCollider *>(collider)) {
            ObjectDatas_->Save<Vector3>(prefix + "size", obb->GetSize());
            ObjectDatas_->Save<Vector3>(prefix + "rotationOffset", obb->GetRotationOffset());
            ObjectDatas_->Save<Vector3>(prefix + "scaleOffset", obb->GetPositionOffset());
        }
    }
}

void BaseObject::LoadColliders() {
    if (!ObjectDatas_) {
        return;
    }

    // 既存のコライダーをクリア（登録解除後、unique_ptr が自動解放）
    for (auto &collider : colliders_) {
        if (collider) {
            CollisionManager::GetInstance()->Unregister(collider.get());
        }
    }
    colliders_.clear();

    // コライダー数を読み込み
    int colliderCount = ObjectDatas_->Load<int>("colliderCount", 0);

    // 各コライダーを読み込んで作成
    for (int i = 0; i < colliderCount; ++i) {
        std::string prefix = "collider_" + std::to_string(i) + "_";

        // 型を読み込み
        ColliderType type = static_cast<ColliderType>(
            ObjectDatas_->Load<int>(prefix + "type", 0));

        ColliderBase *collider = nullptr;
        std::unique_ptr<ColliderBase> colliderOwner;

        // 型に応じてコライダーを作成
        switch (type) {
        case ColliderType::Sphere: {
            auto sphere = std::make_unique<SphereCollider>();
            sphere->SetRadius(ObjectDatas_->Load<float>(prefix + "radius", 1.0f));
            sphere->SetOffset(ObjectDatas_->Load<Vector3>(prefix + "offset", {0.0f, 0.0f, 0.0f}));
            collider = sphere.get();
            colliderOwner = std::move(sphere);
            break;
        }
        case ColliderType::AABB: {
            auto aabb = std::make_unique<AABBCollider>();
            aabb->SetSize(ObjectDatas_->Load<Vector3>(prefix + "size", {1.0f, 1.0f, 1.0f}));
            aabb->SetOffset(ObjectDatas_->Load<Vector3>(prefix + "offset", {0.0f, 0.0f, 0.0f}));
            collider = aabb.get();
            colliderOwner = std::move(aabb);
            break;
        }
        case ColliderType::OBB: {
            auto obb = std::make_unique<OBBCollider>();
            obb->SetSize(ObjectDatas_->Load<Vector3>(prefix + "size", {1.0f, 1.0f, 1.0f}));
            obb->SetRotationOffset(ObjectDatas_->Load<Vector3>(prefix + "rotationOffset", {0.0f, 0.0f, 0.0f}));
            obb->SetPositionOffSet(ObjectDatas_->Load<Vector3>(prefix + "scaleOffset", {0.0f, 0.0f, 0.0f}));
            collider = obb.get();
            colliderOwner = std::move(obb);
            break;
        }
        default:
            continue;
        }

        if (!collider)
            continue;

        // 共通情報を設定
        std::string name = ObjectDatas_->Load<std::string>(prefix + "name", objectName_ + "_Collider" + std::to_string(i));
        collider->SetName(name);
        collider->SetTag(ObjectDatas_->Load<std::string>(prefix + "tag", "None"));
        collider->SetEnabled(ObjectDatas_->Load<bool>(prefix + "isEnabled", true));
        collider->SetVisible(ObjectDatas_->Load<bool>(prefix + "isVisible", true));

        // 衝突マスクを読み込み
        auto maskList = ObjectDatas_->Load<std::vector<std::string>>(
            prefix + "collisionMask",
            std::vector<std::string>());
        for (const auto &maskTag : maskList) {
            collider->AddCollisionMask(maskTag);
        }

        // 位置と回転の取得関数を設定
        collider->SetPositionGetter([this]() { return this->GetWorldPosition(); });
        collider->SetRotationGetter([this]() { return this->GetWorldRotation(); });

        // リストに追加して登録
        colliders_.push_back(std::move(colliderOwner));
        CollisionManager::GetInstance()->Register(collider);
    }
}

void BaseObject::AnimaSaveToJson() {
   /* if (!AnimaDatas_) {
        return;
    }
    AnimaDatas_->Save<bool>("Loop");*/
}

void BaseObject::AnimaLoadFromJson() {
   /* AnimaDatas_ = std::make_unique<DataHandler>("Animation", objectName_);
    isLoop_ = AnimaDatas_->Load<bool>("Loop", false);*/
}

void BaseObject::DebugCollider() {
#ifdef _DEBUG
    if (colliders_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::TextUnformatted("  コライダーなし");
        ImGui::PopStyleColor();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    for (size_t i = 0; i < colliders_.size(); ++i) {
        auto *col = colliders_[i].get();
        if (!col)
            continue;

        ImGui::PushID((int)i);

        // ヘッダー色：衝突中は赤寄り、非衝突は通常
        bool colliding = col->IsCollidingInCurrentFrame();
        ImVec4 hdrColor = colliding
                              ? ImVec4{0.75f, 0.25f, 0.25f, 0.40f}
                              : ImVec4{0.25f, 0.40f, 0.65f, 0.35f};
        ImVec4 hdrHov = colliding
                            ? ImVec4{0.85f, 0.35f, 0.35f, 0.50f}
                            : ImVec4{0.35f, 0.50f, 0.75f, 0.45f};

        ImGui::PushStyleColor(ImGuiCol_Header, hdrColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hdrHov);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, hdrHov);

        bool open = ImGui::CollapsingHeader(col->GetName().c_str());
        ImGui::PopStyleColor(3);

        if (!open) {
            ImGui::PopID();
            continue;
        }

        ImGui::Indent(8.0f);

        // ---- 状態バッジ行 ----
        {
            bool ena = col->IsEnabled();
            bool vis = col->IsVisible();

            ImGui::PushStyleColor(ImGuiCol_CheckMark,
                                  ena ? DebugTheme::kAccentGreen : DebugTheme::kAccentRed);
            if (ImGui::Checkbox("有効##cena", &ena))
                col->SetEnabled(ena);
            ImGui::PopStyleColor();
            ImGui::SameLine(120.f);
            ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentBlue);
            if (ImGui::Checkbox("表示##cvis", &vis))
                col->SetVisible(vis);
            ImGui::PopStyleColor();

            ImGui::SameLine(240.f);
            StatusBadge(colliding ? "衝突中" : "待機",
                        colliding ? DebugTheme::kAccentRed : DebugTheme::kAccentGreen);
        }

        ImGui::Spacing();

        // ---- タグ設定 ----
        ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kBgBlue);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.55f, 1.0f, 0.20f));
        if (ImGui::TreeNodeEx("タグ設定##ctag", ImGuiTreeNodeFlags_SpanAvailWidth)) {
            col->ImGuiTagSettings();
            ImGui::TreePop();
        }
        ImGui::PopStyleColor(2);

        ImGui::Spacing();

        // ---- コライダー形状パラメータ ----
        SectionHeader("[ 形状パラメータ ]", DebugTheme::kAccentCyan);

        if (auto *sphere = dynamic_cast<SphereCollider *>(col)) {
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentCyan);
            ImGui::TextUnformatted("種別: 球体");
            ImGui::PopStyleColor();

            float r = sphere->GetRadius();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::TextUnformatted("半径");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgBlue);
            if (ImGui::DragFloat("##srad", &r, 0.1f, 0.1f, 100.f, "%.2f"))
                sphere->SetRadius(r);
            ImGui::PopStyleColor();

            Vector3 off = sphere->GetOffset();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::TextUnformatted("オフセット (X / Y / Z)");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgBlue);
            if (ImGui::DragFloat3("##soff", &off.x, 0.1f, -1000.f, 1000.f, "%.2f"))
                sphere->SetOffset(off);
            ImGui::PopStyleColor();
        } else if (auto *aabb = dynamic_cast<AABBCollider *>(col)) {
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentGreen);
            ImGui::TextUnformatted("種別: AABB");
            ImGui::PopStyleColor();

            Vector3 sz = aabb->GetSize();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::TextUnformatted("サイズ (X / Y / Z)");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgGreen);
            if (ImGui::DragFloat3("##asz", &sz.x, 0.1f, 0.1f, 100.f, "%.2f"))
                aabb->SetSize(sz);
            ImGui::PopStyleColor();

            Vector3 off = aabb->GetOffset();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::TextUnformatted("オフセット (X / Y / Z)");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgGreen);
            if (ImGui::DragFloat3("##aoff", &off.x, 0.1f, -1000.f, 1000.f, "%.2f"))
                aabb->SetOffset(off);
            ImGui::PopStyleColor();
        } else if (auto *obb = dynamic_cast<OBBCollider *>(col)) {
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentPurple);
            ImGui::TextUnformatted("種別: OBB");
            ImGui::PopStyleColor();

            Vector3 sz = obb->GetSize();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::TextUnformatted("サイズ (X / Y / Z)");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgPurple);
            if (ImGui::DragFloat3("##osz", &sz.x, 0.1f, 0.1f, 100.f, "%.2f"))
                obb->SetSize(sz);
            ImGui::PopStyleColor();

            Vector3 rotOff = obb->GetRotationOffset();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::TextUnformatted("回転オフセット (X / Y / Z)");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgPurple);
            if (ImGui::DragFloat3("##oroff", &rotOff.x, 0.1f, -1000.f, 1000.f, "%.2f"))
                obb->SetRotationOffset(rotOff);
            ImGui::PopStyleColor();

            Vector3 posOff = obb->GetPositionOffset();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::TextUnformatted("位置オフセット (X / Y / Z)");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgPurple);
            if (ImGui::DragFloat3("##opoff", &posOff.x, 0.1f, -1000.f, 1000.f, "%.2f"))
                obb->SetPositionOffSet(posOff);
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // ---- 保存 / 削除ボタン ----
        float bw = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        ImGui::PushStyleColor(ImGuiCol_Button, {0.20f, 0.45f, 0.20f, 0.8f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f, 0.55f, 0.25f, 0.9f});
        if (ImGui::Button("保存##colsv", ImVec2(bw, 0)))
            col->SaveToJson();
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, {0.55f, 0.15f, 0.15f, 0.8f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.75f, 0.20f, 0.20f, 0.9f});
        if (ImGui::Button("削除##coldel", ImVec2(bw, 0))) {
            CollisionManager::GetInstance()->Unregister(col);
            delete col;
            colliders_.erase(colliders_.begin() + i);
            ImGui::Unindent(8.0f);
            ImGui::PopID();
            break;
        }
        ImGui::PopStyleColor(2);

        ImGui::Unindent(8.0f);
        ImGui::Spacing();
        ImGui::PopID();
    }

    ImGui::PopStyleVar(2);
#endif
}

// ============================================================
//  BaseObject::ImGui  (メインタブ)
// ============================================================
void BaseObject::ImGui() {
#ifdef _DEBUG
    if (ImGui::BeginTabBar(objectName_.c_str())) {
        if (ImGui::BeginTabItem(objectName_.c_str())) {
            // トランスフォーム・マテリアル等
            DebugObject();

            ImGui::Spacing();

            // ---- 描画モード ----
            SectionHeader("[ 描画モード ]", DebugTheme::kAccentBlue);

            ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentBlue);
            if (ImGui::Checkbox("モデル描画##mdraw", &isModelDraw_) && isModelDraw_)
                isWireframe_ = false;
            ImGui::SameLine(160.f);
            if (ImGui::Checkbox("ワイヤーフレーム##wf", &isWireframe_) && isWireframe_)
                isModelDraw_ = false;
            ImGui::PopStyleColor();

            if (isWireframe_) {
                ImGui::SameLine(280.f);
                ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentYellow);
                ImGui::Checkbox("レインボー##rb", &isRainbow_);
                ImGui::PopStyleColor();
            } else {
                isRainbow_ = false;
            }

            ImGui::Spacing();

            // ---- セーブボタン ----
            ImGui::PushStyleColor(ImGuiCol_Button, {0.20f, 0.45f, 0.20f, 0.80f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f, 0.55f, 0.25f, 0.90f});
            if (ImGui::Button("全て保存##objsave", ImVec2(-1, 0))) {
                SaveToJson();
                AnimaSaveToJson();
                for (auto &c : colliders_)
                    c->SaveToJson();

                std::string msg = std::format("対象のオブジェクトをセーブしました！: {}", objectName_);
                ImGuiNotification::Post(msg);
            }
            ImGui::PopStyleColor(2);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
#endif // _DEBUG
}

SphereCollider *BaseObject::AddSphereCollider(const std::string &name) {
    auto collider = std::make_unique<SphereCollider>();

    std::string colliderName = name.empty() ? objectName_ + "_SphereCollider" : name;
    collider->SetName(colliderName);

    collider->SetPositionGetter([this]() { return this->GetWorldPosition(); });
    collider->SetRotationGetter([this]() { return this->GetWorldRotation(); });

    SphereCollider *raw = collider.get();
    colliders_.push_back(std::move(collider));
    CollisionManager::GetInstance()->Register(raw);

    return raw;
}

AABBCollider *BaseObject::AddAABBCollider(const std::string &name) {
    auto collider = std::make_unique<AABBCollider>();

    std::string colliderName = name.empty() ? objectName_ + "_AABBCollider" : name;
    collider->SetName(colliderName);

    collider->SetPositionGetter([this]() { return this->GetWorldPosition(); });
    collider->SetRotationGetter([this]() { return this->GetWorldRotation(); });

    AABBCollider *raw = collider.get();
    colliders_.push_back(std::move(collider));
    CollisionManager::GetInstance()->Register(raw);

    return raw;
}

OBBCollider *BaseObject::AddOBBCollider(const std::string &name) {
    auto collider = std::make_unique<OBBCollider>();

    std::string colliderName = name.empty() ? objectName_ + "_OBBCollider" : name;
    collider->SetName(colliderName);

    collider->SetPositionGetter([this]() { return this->GetWorldPosition(); });
    collider->SetRotationGetter([this]() { return this->GetWorldRotation(); });

    OBBCollider *raw = collider.get();
    colliders_.push_back(std::move(collider));
    CollisionManager::GetInstance()->Register(raw);

    return raw;
}

CylinderCollider *BaseObject::AddCylinderCollider(const std::string &name) {
    auto collider = std::make_unique<CylinderCollider>();

    std::string colliderName = name.empty() ? objectName_ + "_CylinderCollider" : name;
    collider->SetName(colliderName);

    collider->SetPositionGetter([this]() { return this->GetWorldPosition(); });
    collider->SetRotationGetter([this]() { return this->GetWorldRotation(); });

    CylinderCollider *raw = collider.get();
    colliders_.push_back(std::move(collider));
    CollisionManager::GetInstance()->Register(raw);

    return raw;
}

void BaseObject::DebugObject() {
#ifdef _DEBUG
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);

    ImGui::BeginChild("DebugObjectRoot", ImVec2(0, 0), false);

    // ====================================================
    // [1] Transform
    // ====================================================
    ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kHeaderBlue);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.25f, 0.60f, 1.0f, 0.35f});
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.30f, 0.65f, 1.0f, 0.45f});
    bool tfOpen = ImGui::CollapsingHeader("トランスフォーム##hdr", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor(3);

    if (tfOpen) {
        ImGui::Indent(6.0f);

        // ---- Local ----
        SectionHeader("[ ローカル ]", DebugTheme::kAccentBlue);

        // Table: Label | DragFloat3 | ResetBtn
        if (ImGui::BeginTable("LocalTF", 3,
                              ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX)) {
            ImGui::TableSetupColumn("Lbl", ImGuiTableColumnFlags_WidthFixed, 62.0f);
            ImGui::TableSetupColumn("Drg", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Btn", ImGuiTableColumnFlags_WidthFixed, 30.0f);

            // -- Position --
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentBlue);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("位置");
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgBlue);
            ImGui::DragFloat3("##lpos", &transform_->translation_.x,
                              0.1f, -1000.f, 1000.f, "%.2f");
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            if (SmallResetButton("[R]##rpos"))
                transform_->translation_ = {};

            // -- Rotation (delta) --
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentCyan);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("回転(差分)");
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            static Vector3 deltaRot{};
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.10f, 0.85f, 0.90f, 0.12f});
            if (ImGui::DragFloat3("##lrot", &deltaRot.x, 0.1f, -10.f, 10.f, "%.1fdeg")) {
                float r = std::numbers::pi_v<float> / 180.f;
                Quaternion cur = transform_->GetRotationQuaternion();
                Quaternion dx = Quaternion::FromAxisAngle({1, 0, 0}, deltaRot.x * r);
                Quaternion dy = Quaternion::FromAxisAngle({0, 1, 0}, deltaRot.y * r);
                Quaternion dz = Quaternion::FromAxisAngle({0, 0, 1}, deltaRot.z * r);
                transform_->SetRotationQuaternion((cur * (dy * dx * dz)).Normalize());
                transform_->UpdateMatrix();
                deltaRot = {};
            }
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            if (SmallResetButton("[R]##rrot")) {
                transform_->SetRotationQuaternion(Quaternion::IdentityQuaternion());
                transform_->UpdateMatrix();
                deltaRot = {};
            }

            // -- Scale --
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentGreen);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("スケール");
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgGreen);
            ImGui::DragFloat3("##lscl", &transform_->scale_.x,
                              0.01f, 0.01f, 10.f, "%.2f");
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();
            if (SmallResetButton("[R]##rscl"))
                transform_->scale_ = {1, 1, 1};

            ImGui::EndTable();
        }

        // 現在のオイラー角（参考）
        {
            Vector3 e = transform_->GetRotationEuler();
            float d = 180.f / std::numbers::pi_v<float>;
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::Text(" Current Rot  X:%.1f  Y:%.1f  Z:%.1f (deg)",
                        e.x * d, e.y * d, e.z * d);
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // ---- World (read-only) ----
        SectionHeader("[ ワールド (読み取り専用) ]", DebugTheme::kAccentGreen);

        Vector3 wPos = GetWorldPosition();
        Quaternion wRot = GetWorldRotation();
        Vector3 wScale = GetWorldScale();
        float toDeg = 180.f / std::numbers::pi_v<float>;

        // ReadOnlyRow を使って ## が画面に出ないようにする
        // InputFloat3 の ID は ## 始まりで非表示
        auto WorldVec3Row = [](const char *rowLabel,
                               const char *dragId,
                               float x, float y, float z) {
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("  %-10s", rowLabel);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            float v[3] = {x, y, z};
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.12f, 0.12f, 0.14f, 0.8f});
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextReadOnly);
            ImGui::InputFloat3(dragId, v, "%.2f", ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor(2);
        };

        WorldVec3Row("位置", "##wpos", wPos.x, wPos.y, wPos.z);
        WorldVec3Row("Rotation", "##wrot",
                     wRot.x * toDeg, wRot.y * toDeg, wRot.z * toDeg);
        WorldVec3Row("スケール", "##wscl", wScale.x, wScale.y, wScale.z);

        ImGui::Spacing();

        // ---- ImPlot: Scale history ----
        ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kBgGreen);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.20f, 0.85f, 0.50f, 0.20f});
        bool plotOpen = ImGui::CollapsingHeader("スケール履歴 (グラフ)##scgr");
        ImGui::PopStyleColor(2);

        if (plotOpen) {
            constexpr int kN = 120;
            static float hx[kN]{}, hy[kN]{}, hz[kN]{};
            static int head = 0, cnt = 0;
            hx[head] = transform_->scale_.x;
            hy[head] = transform_->scale_.y;
            hz[head] = transform_->scale_.z;
            head = (head + 1) % kN;
            if (cnt < kN)
                ++cnt;

            static float dx[kN], dy[kN], dz[kN];
            int s = (head - cnt + kN) % kN;
            for (int i = 0; i < cnt; ++i) {
                int id = (s + i) % kN;
                dx[i] = hx[id];
                dy[i] = hy[id];
                dz[i] = hz[id];
            }

            ImPlot::PushStyleColor(ImPlotCol_PlotBg, {0.08f, 0.08f, 0.10f, 1.0f});
            if (ImPlot::BeginPlot("##scplot", ImVec2(-1, 75),
                                  ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                                      ImPlotFlags_NoInputs | ImPlotFlags_NoFrame)) {
                ImPlot::SetupAxes(nullptr, nullptr,
                                  ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, kN, ImGuiCond_Always);
                ImPlot::PushStyleColor(ImPlotCol_Line, DebugTheme::kAccentRed);
                ImPlot::PlotLine("X", dx, cnt);
                ImPlot::PopStyleColor();
                ImPlot::PushStyleColor(ImPlotCol_Line, DebugTheme::kAccentGreen);
                ImPlot::PlotLine("Y", dy, cnt);
                ImPlot::PopStyleColor();
                ImPlot::PushStyleColor(ImPlotCol_Line, DebugTheme::kAccentBlue);
                ImPlot::PlotLine("Z", dz, cnt);
                ImPlot::PopStyleColor();
                ImPlot::EndPlot();
            }
            ImPlot::PopStyleColor();
            // 凡例
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentRed);
            ImGui::TextUnformatted(" X");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentGreen);
            ImGui::TextUnformatted(" Y");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentBlue);
            ImGui::TextUnformatted(" Z");
            ImGui::PopStyleColor();
        }

        ImGui::Unindent(6.0f);
        ImGui::Spacing();
    }

    // ====================================================
    // [2] Material
    // ====================================================
    ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kHeaderOrange);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {1.0f, 0.65f, 0.15f, 0.35f});
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, {1.0f, 0.70f, 0.20f, 0.45f});
    bool matOpen = ImGui::CollapsingHeader("マテリアル##hdr");
    ImGui::PopStyleColor(3);

    if (matOpen) {
        ImGui::Indent(6.0f);
        SectionHeader("[ ライティング ]", DebugTheme::kAccentOrange);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("状態:");
        ImGui::SameLine();
        StatusBadge(isLighting_ ? "オン" : "オフ",
                    isLighting_ ? DebugTheme::kAccentGreen : DebugTheme::kAccentRed);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,
                              isLighting_ ? ImVec4{0.6f, 0.2f, 0.2f, 0.7f} : ImVec4{0.2f, 0.5f, 0.2f, 0.7f});
        if (ImGui::SmallButton(isLighting_ ? "無効にする##lt" : "有効にする##lt"))
            isLighting_ = !isLighting_;
        ImGui::PopStyleColor();

        ImGui::Unindent(6.0f);
        ImGui::Spacing();
    }

    // ====================================================
    // [3] Model
    // ====================================================
    ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kHeaderPurple);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.75f, 0.35f, 1.0f, 0.35f});
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.80f, 0.40f, 1.0f, 0.45f});
    bool modelOpen = ImGui::CollapsingHeader("モデル##hdr");
    ImGui::PopStyleColor(3);

    if (modelOpen) {
        ImGui::Indent(6.0f);
        static int selMat = 0;
        size_t matCount = obj3d_->GetMaterialCount();
        if (obj3d_->GetHaveAnimation() && matCount > 1)
            --matCount;

        SectionHeader("[ マテリアルスロット ]", DebugTheme::kAccentPurple);

        if (matCount > 1) {
            std::vector<std::string> items;
            std::vector<const char *> cstrs;
            for (int i = 0; i < (int)matCount; ++i)
                items.push_back("Slot " + std::to_string(i + 1));
            for (auto &s : items)
                cstrs.push_back(s.c_str());
            ImGui::SetNextItemWidth(-1);
            ImGui::Combo("##matslot", &selMat, cstrs.data(), (int)cstrs.size());
            selMat = std::clamp(selMat, 0, (int)matCount - 1);
        } else {
            selMat = 0;
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
            ImGui::TextUnformatted("  シングルマテリアル");
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // Color
        ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kBgPurple);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.7f, 0.3f, 1.0f, 0.20f});
        if (ImGui::TreeNodeEx("カラー##mc", ImGuiTreeNodeFlags_SpanAvailWidth)) {
            Vector4 cur = GetColor(selMat);
            float c[4] = {cur.x, cur.y, cur.z, cur.w};
            ImGui::SetNextItemWidth(-1);
            if (ImGui::ColorEdit4("##colpicker", c))
                SetColor({c[0], c[1], c[2], c[3]}, selMat);
            if (ImGui::SmallButton("カラーリセット##cr"))
                SetColor({1, 1, 1, 1}, selMat);
            ImGui::TreePop();
        }

        // Texture
        if (ImGui::TreeNodeEx("テクスチャ##tx", ImGuiTreeNodeFlags_SpanAvailWidth)) {
            ShowTextureFile(texturePath_);
            ImGui::Spacing();
            if (ImGui::SmallButton("適用##ta")) {
                SetTexture(texturePath_, selMat);
                texturePaths_[selMat] = texturePath_;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("クリア##tc"))
                texturePath_.clear();
            ImGui::TreePop();
        }

        // Blend mode
        if (ImGui::TreeNodeEx("ブレンドモード##bm", ImGuiTreeNodeFlags_SpanAvailWidth)) {
            ShowBlendModeCombo(blendMode_);
            ImGui::TreePop();
        }
        ImGui::PopStyleColor(2);

        ImGui::Unindent(6.0f);
        ImGui::Spacing();
    }

    // ====================================================
    // [4] Animation
    // ====================================================
    if (obj3d_->GetHaveAnimation()) {
        ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kHeaderYellow);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {1.0f, 0.88f, 0.15f, 0.35f});
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, {1.0f, 0.90f, 0.20f, 0.45f});
        bool animOpen = ImGui::CollapsingHeader("アニメーション##hdr");
        ImGui::PopStyleColor(3);

        if (animOpen) {
            ImGui::Indent(6.0f);
            SectionHeader("[ 制御 ]", DebugTheme::kAccentYellow);

            ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentYellow);
            // 現在のアニメーションのループ設定を取得・変更
            std::string currentModelPath = obj3d_->GetModelFilePath();
            bool loop = obj3d_->GetAnimationLoop(currentModelPath);
            if (ImGui::Checkbox("ループ##lp", &loop)) {
                obj3d_->SetAnimationLoop(currentModelPath, loop);
            }

            ImGui::SameLine(130.0f);
            ImGui::Checkbox("スケルトン表示##sk", &skeletonDraw_);
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // アニメーション速度設定
            float speed = obj3d_->GetAnimationSpeed();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentYellow);
            ImGui::TextUnformatted("再生速度");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, DebugTheme::kBgYellow);
            if (ImGui::DragFloat("##aspeed", &speed, 0.01f, 0.0f, 10.0f, "%.2f")) {
                obj3d_->SetAnimationSpeed(speed);
            }
            ImGui::PopStyleColor();

            // ブレンド時間設定
            float blendDuration = obj3d_->GetAnimationBlendDuration();
            ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kAccentCyan);
            ImGui::TextUnformatted("ブレンド時間 (秒)");
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.10f, 0.85f, 0.90f, 0.12f});
            if (ImGui::DragFloat("##ablend", &blendDuration, 0.01f, 0.0f, 5.0f, "%.2f")) {
                obj3d_->SetAnimationBlendDuration(blendDuration);
            }
            ImGui::PopStyleColor();

            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, {0.25f, 0.55f, 0.20f, 0.8f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.30f, 0.70f, 0.25f, 0.9f});
            if (ImGui::Button("再生##aplay", ImVec2(-1, 0)))
                obj3d_->PlayAnimation();
            ImGui::PopStyleColor(2);

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kBgYellow);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {1.0f, 0.85f, 0.10f, 0.20f});
            if (ImGui::TreeNodeEx("アニメーション設定##as", ImGuiTreeNodeFlags_SpanAvailWidth)) {
                ShowFileSelector();
                ImGui::TreePop();
            }
            ImGui::PopStyleColor(2);
            ImGui::Unindent(6.0f);
        }
    }

    // ====================================================
    // [5] Gizmo
    // ====================================================
    ImGui::PushStyleColor(ImGuiCol_Header, DebugTheme::kBgRed);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {1.0f, 0.30f, 0.30f, 0.20f});
    bool gizOpen = ImGui::CollapsingHeader("ギズモ##hdr");
    ImGui::PopStyleColor(2);

    if (gizOpen) {
        ImGui::Indent(6.0f);
        ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentRed);
        ImGui::Checkbox("ギズモ選択可##gsel", &isGizmoSelectable_);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("オフ: マウスクリック/ギズモ操作の対象外");
        ImGui::Unindent(6.0f);
    }

    // ====================================================
    // [6] Collider
    // ====================================================
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.50f, 0.65f, 0.40f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.60f, 0.75f, 0.45f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.65f, 0.80f, 0.55f));
    bool colOpen = ImGui::CollapsingHeader("コライダー##hdr");
    ImGui::PopStyleColor(3);

    if (colOpen) {
        ImGui::Indent(6.0f);

        // コライダー追加ボタン
        ImGui::PushStyleColor(ImGuiCol_Button, {0.20f, 0.40f, 0.60f, 0.80f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.30f, 0.50f, 0.70f, 0.90f});
        if (ImGui::Button("+ コライダー追加##addcol"))
            ImGui::OpenPopup("AddColliderPopup##acp");
        ImGui::PopStyleColor(2);

        if (ImGui::BeginPopup("AddColliderPopup##acp")) {
            auto makeDefault = [](auto *c) {
                c->SetTag("Environment");
                c->AddCollisionMask("Player");
            };
            if (ImGui::MenuItem("Sphere")) {
                auto *c = AddSphereCollider();
                makeDefault(c);
                c->SetRadius(1.0f);
            }
            if (ImGui::MenuItem("AABB")) {
                auto *c = AddAABBCollider();
                makeDefault(c);
                c->SetSize({2.0f, 2.0f, 2.0f});
            }
            if (ImGui::MenuItem("OBB")) {
                auto *c = AddOBBCollider();
                makeDefault(c);
                c->SetSize({2.0f, 2.0f, 2.0f});
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        DebugCollider();

        ImGui::Unindent(6.0f);
        ImGui::Spacing();
    }

    // ====================================================
    // [7] Scale Easing Test
    // ====================================================
    ImGui::PushStyleColor(ImGuiCol_Header, {0.15f, 0.50f, 0.35f, 0.40f});
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.20f, 0.65f, 0.45f, 0.45f});
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.25f, 0.70f, 0.50f, 0.55f});
    bool seOpen = ImGui::CollapsingHeader("スケールイージングテスト##hdr");
    ImGui::PopStyleColor(3);

    if (seOpen) {
        ImGui::Indent(6.0f);
        SectionHeader("[ イージングタイプ ]", DebugTheme::kAccentGreen);
        DrawScaleEaseImGui();
        ImGui::Unindent(6.0f);
        ImGui::Spacing();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(4);
#endif // _DEBUG
}

void BaseObject::DrawScaleEaseImGui() {
#ifdef _DEBUG
    // EasingType enum（0〜30）＋ Vector3 Amplitude 拡張（31〜33）の表示名
    // 31 = InElasticAmplitude, 32 = OutElasticAmplitude, 33 = InOutElasticAmplitude
    static const char *kModeNames[] = {
        "Linear",
        "InSine",
        "OutSine",
        "InOutSine",
        "InQuad",
        "OutQuad",
        "InOutQuad",
        "InCubic",
        "OutCubic",
        "InOutCubic",
        "InQuart",
        "OutQuart",
        "InOutQuart",
        "InQuint",
        "OutQuint",
        "InOutQuint",
        "InCirc",
        "OutCirc",
        "InOutCirc",
        "InExpo",
        "OutExpo",
        "InOutExpo",
        "InBack",
        "OutBack",
        "InOutBack",
        "InElastic",
        "OutElastic",
        "InOutElastic",
        "InBounce",
        "OutBounce",
        "InOutBounce",
        // Vector3 振幅による加算オフセット系（EasingType 外の拡張）
        "InElastic  [Amplitude]",
        "OutElastic [Amplitude]",
        "InOutElastic [Amplitude]",
    };
    constexpr int kModeCount = IM_ARRAYSIZE(kModeNames);

    // Amplitude 拡張モード（selectedMode >= 31）かどうかを判定する
    auto IsAmplitudeMode = [](int mode) { return mode >= 31; };

    // ----- イージングタイプ選択 -----
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##setype", &scaleEase_.selectedMode, kModeNames, kModeCount);

    ImGui::Spacing();

    // ----- 所要時間：ラベルを別行に表示して幅いっぱいにドラッグフィールドを配置 -----
    ImGui::TextUnformatted("所要時間");
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##seT", &scaleEase_.totalTime, 0.05f, 0.05f, 10.0f, "%.2f s");

    ImGui::Spacing();

    if (IsAmplitudeMode(scaleEase_.selectedMode)) {
        // ----- Elastic Amplitude モード：軸ごとに独立した振幅でスケールを加算する -----
        SectionHeader("[ Elastic Amplitude ]", DebugTheme::kAccentGreen);

        // ラベルを別行に出すことで SetNextItemWidth(-1) でも名称が確認できる
        ImGui::TextUnformatted("振幅 (X / Y / Z)");
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat3("##seAmp", &scaleEase_.amplitude.x,
                          0.01f, -5.0f, 5.0f, "%.2f");

        ImGui::TextUnformatted("周期");
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat("##sePer", &scaleEase_.period,
                         0.01f, 0.01f, 2.0f, "%.2f");

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, DebugTheme::kTextDim);
        ImGui::TextUnformatted(" スタート時の現在スケールを基準に振幅を加算");
        ImGui::PopStyleColor();
    } else {
        // ----- 通常イージングモード（start -> end 補間） -----
        SectionHeader("[ スタート / エンド スケール ]", DebugTheme::kAccentBlue);

        // ラベルをウィジェット上行に表示し、ボタン分の幅を残して配置
        ImGui::TextUnformatted("スタートスケール");
        ImGui::SetNextItemWidth(-80.0f);
        ImGui::DragFloat3("##seSS", &scaleEase_.startScale.x,
                          0.01f, 0.01f, 20.0f, "%.2f");
        ImGui::SameLine();
        // 現在のスケール値をスタート値としてキャプチャする
        if (ImGui::SmallButton("現在##cpSS")) {
            scaleEase_.startScale = transform_->scale_;
        }

        ImGui::TextUnformatted("エンドスケール");
        ImGui::SetNextItemWidth(-80.0f);
        ImGui::DragFloat3("##seES", &scaleEase_.endScale.x,
                          0.01f, 0.01f, 20.0f, "%.2f");
        ImGui::SameLine();
        // 現在のスケール値をエンド値としてキャプチャする
        if (ImGui::SmallButton("現在##cpES")) {
            scaleEase_.endScale = transform_->scale_;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ----- 再生中：プログレスバーとスケール更新 -----
    if (scaleEase_.isActive) {
        float progress = scaleEase_.currentTime / scaleEase_.totalTime;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
        ImGui::Spacing();

        // DeltaTime でフレーム経過時間を加算して再生を進める
        float dt = ImGui::GetIO().DeltaTime;
        scaleEase_.currentTime += dt;

        if (scaleEase_.currentTime >= scaleEase_.totalTime) {
            // 再生完了：停止してスケールを終端値に確定する
            scaleEase_.currentTime = scaleEase_.totalTime;
            scaleEase_.isActive = false;
            if (IsAmplitudeMode(scaleEase_.selectedMode)) {
                transform_->scale_ = scaleEase_.baseScale;
            } else {
                transform_->scale_ = scaleEase_.endScale;
            }
        } else {
            // 再生中のスケール更新
            if (IsAmplitudeMode(scaleEase_.selectedMode)) {
                // Vector3 振幅をオフセットとしてベーススケールに加算する
                Vector3 offset;
                if (scaleEase_.selectedMode == 31) {
                    offset = EaseInElasticAmplitude(
                        scaleEase_.currentTime, scaleEase_.totalTime,
                        scaleEase_.amplitude, scaleEase_.period);
                } else if (scaleEase_.selectedMode == 32) {
                    offset = EaseOutElasticAmplitude(
                        scaleEase_.currentTime, scaleEase_.totalTime,
                        scaleEase_.amplitude, scaleEase_.period);
                } else {
                    offset = EaseInOutElasticAmplitude(
                        scaleEase_.currentTime, scaleEase_.totalTime,
                        scaleEase_.amplitude, scaleEase_.period);
                }
                transform_->scale_ = scaleEase_.baseScale + offset;
            } else {
                // EasingType 範囲（0〜30）は start -> end 補間
                transform_->scale_ = ApplyEasing(
                    static_cast<EasingType>(scaleEase_.selectedMode),
                    scaleEase_.startScale, scaleEase_.endScale,
                    scaleEase_.currentTime, scaleEase_.totalTime);
            }
        }
    }

    // ----- スタート / ストップボタン -----
    if (!scaleEase_.isActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.15f, 0.55f, 0.25f, 0.9f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.20f, 0.70f, 0.30f, 1.0f});
        if (ImGui::Button("スタート##sePlay", ImVec2(-1.0f, 0.0f))) {
            scaleEase_.currentTime = 0.0f;
            scaleEase_.isActive = true;
            // スタート時点のスケールをベースとして記録する
            scaleEase_.baseScale = transform_->scale_;
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.55f, 0.15f, 0.15f, 0.9f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.70f, 0.20f, 0.20f, 1.0f});
        if (ImGui::Button("ストップ##seStop", ImVec2(-1.0f, 0.0f))) {
            scaleEase_.isActive = false;
            // ストップ時はスケールをスタート時点の値に戻す
            transform_->scale_ = scaleEase_.baseScale;
        }
        ImGui::PopStyleColor(2);
    }
#endif
}

void BaseObject::ShowFileSelector() {
#ifdef _DEBUG
    // ShowFolder の GLTF ブラウザで選択パスを保持する
    // 選択確定後に「適用」ボタンで SetAnimation を呼び出す
    static std::string selectedGltfPath;

    ShowGltfFile(selectedGltfPath);

    ImGui::Spacing();

    if (!selectedGltfPath.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.25f, 0.45f, 0.70f, 0.80f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.35f, 0.55f, 0.85f, 0.90f});
        if (ImGui::Button("アニメーション適用##applyAnima", ImVec2(-1.0f, 0.0f))) {
            obj3d_->SetAnimation(selectedGltfPath);
        }
        ImGui::PopStyleColor(2);
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