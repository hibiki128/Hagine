#define NOMINMAX
#include "BaseObject.h"
#include "Scene/SceneManager.h"
#include "ShowFolder/ShowFolder.h"
#include"Collider/CollisionManager.h"

namespace Hagine::Graphics {
BaseObject::~BaseObject() {
    // すべてのコライダーを削除
    for (auto *collider : colliders_) {
        if (collider) {
            CollisionManager::GetInstance()->Unregister(collider);
            delete collider;
        }
    }
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
        obj3d_->AnimationUpdate(isLoop_);
    }
    SetBlendMode(blendMode_);
}

void BaseObject::Draw(const Camera::ViewProjection &viewProjection, Vector3 offSet) {
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
        auto *collider = colliders_[i];
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

    // 既存のコライダーをクリア
    for (auto *collider : colliders_) {
        if (collider) {
            CollisionManager::GetInstance()->Unregister(collider);
            delete collider;
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

        // 型に応じてコライダーを作成
        switch (type) {
        case ColliderType::Sphere: {
            auto *sphere = new SphereCollider();
            sphere->SetRadius(ObjectDatas_->Load<float>(prefix + "radius", 1.0f));
            sphere->SetOffset(ObjectDatas_->Load<Vector3>(prefix + "offset", {0.0f, 0.0f, 0.0f}));
            collider = sphere;
            break;
        }
        case ColliderType::AABB: {
            auto *aabb = new AABBCollider();
            aabb->SetSize(ObjectDatas_->Load<Vector3>(prefix + "size", {1.0f, 1.0f, 1.0f}));
            aabb->SetOffset(ObjectDatas_->Load<Vector3>(prefix + "offset", {0.0f, 0.0f, 0.0f}));
            collider = aabb;
            break;
        }
        case ColliderType::OBB: {
            auto *obb = new OBBCollider();
            obb->SetSize(ObjectDatas_->Load<Vector3>(prefix + "size", {1.0f, 1.0f, 1.0f}));
            obb->SetRotationOffset(ObjectDatas_->Load<Vector3>(prefix + "rotationOffset", {0.0f, 0.0f, 0.0f}));
            obb->SetPositionOffSet(ObjectDatas_->Load<Vector3>(prefix + "scaleOffset", {0.0f, 0.0f, 0.0f}));
            collider = obb;
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
        colliders_.push_back(collider);
        CollisionManager::GetInstance()->Register(collider);
    }
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
#ifdef _DEBUG
    if (colliders_.empty()) {
        ImGui::Text("コライダーなし");
        return;
    }

    for (size_t i = 0; i < colliders_.size(); ++i) {
        auto *collider = colliders_[i];
        if (!collider)
            continue;

        ImGui::PushID(static_cast<int>(i));

        if (ImGui::TreeNode(collider->GetName().c_str())) {
            // 有効/無効
            bool enabled = collider->IsEnabled();
            if (ImGui::Checkbox("有効", &enabled)) {
                collider->SetEnabled(enabled);
            }

            // 可視性
            bool visible = collider->IsVisible();
            if (ImGui::Checkbox("表示", &visible)) {
                collider->SetVisible(visible);
            }

            ImGui::Spacing();

            // 衝突状態の表示（新規追加）
            bool isColliding = collider->IsCollidingInCurrentFrame();
            ImGui::PushStyleColor(ImGuiCol_Text, isColliding ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            ImGui::Text("衝突状態: %s", isColliding ? "衝突中" : "非衝突");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // タグ設定UI
            collider->ImGuiTagSettings();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 型に応じた詳細設定
            if (auto *sphere = dynamic_cast<SphereCollider *>(collider)) {
                ImGui::Text("【球体コライダー】");
                float radius = sphere->GetRadius();
                if (ImGui::DragFloat("半径", &radius, 0.1f, 0.1f, 100.0f)) {
                    sphere->SetRadius(radius);
                }

                Vector3 offset = sphere->GetOffset();
                if (ImGui::DragFloat3("オフセット", &offset.x, 0.1f)) {
                    sphere->SetOffset(offset);
                }
            } else if (auto *aabb = dynamic_cast<AABBCollider *>(collider)) {
                ImGui::Text("【AABBコライダー】");
                Vector3 size = aabb->GetSize();
                if (ImGui::DragFloat3("サイズ", &size.x, 0.1f, 0.1f, 100.0f)) {
                    aabb->SetSize(size);
                }

                Vector3 offset = aabb->GetOffset();
                if (ImGui::DragFloat3("オフセット", &offset.x, 0.1f)) {
                    aabb->SetOffset(offset);
                }
            } else if (auto *obb = dynamic_cast<OBBCollider *>(collider)) {
                ImGui::Text("【OBBコライダー】");
                Vector3 size = obb->GetSize();
                if (ImGui::DragFloat3("サイズ", &size.x, 0.1f, 0.1f, 100.0f)) {
                    obb->SetSize(size);
                }

                Vector3 rotOffset = obb->GetRotationOffset();
                if (ImGui::DragFloat3("回転オフセット", &rotOffset.x, 0.1f)) {
                    obb->SetRotationOffset(rotOffset);
                }

                Vector3 scaleOffset = obb->GetPositionOffset();
                if (ImGui::DragFloat3("スケールオフセット", &scaleOffset.x, 0.1f)) {
                    obb->SetPositionOffSet(scaleOffset);
                }
            }

            ImGui::Spacing();

            // セーブボタン
            if (ImGui::Button("保存", ImVec2(80, 0))) {
                collider->SaveToJson();
            }

            // 削除ボタン
            ImGui::SameLine();
            if (ImGui::Button("削除", ImVec2(80, 0))) {
                CollisionManager::GetInstance()->Unregister(collider);
                delete collider;
                colliders_.erase(colliders_.begin() + i);
                ImGui::TreePop();
                ImGui::PopID();
                break;
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
#endif
}

void BaseObject::ImGui() {
#ifdef _DEBUG
    if (ImGui::BeginTabBar(objectName_.c_str())) {
        if (ImGui::BeginTabItem(objectName_.c_str())) {
            DebugObject();

            if (ImGui::CollapsingHeader("コライダー設定", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(10);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // コライダー追加ボタン
                if (ImGui::Button("コライダー追加")) {
                    ImGui::OpenPopup("AddColliderPopup");
                }

                if (ImGui::BeginPopup("AddColliderPopup")) {
                    if (ImGui::MenuItem("球体コライダー")) {
                        auto *collider = AddSphereCollider();
                        collider->SetTag("Environment");
                        collider->AddCollisionMask("Player");
                        collider->SetRadius(1.0f);
                    }

                    if (ImGui::MenuItem("AABBコライダー")) {
                        auto *collider = AddAABBCollider();
                        collider->SetTag("Environment");
                        collider->AddCollisionMask("Player");
                        collider->SetSize({2.0f, 2.0f, 2.0f});
                    }

                    if (ImGui::MenuItem("OBBコライダー")) {
                        auto *collider = AddOBBCollider();
                        collider->SetTag("Environment");
                        collider->AddCollisionMask("Player");
                        collider->SetSize({2.0f, 2.0f, 2.0f});
                    }

                    ImGui::EndPopup();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // コライダーのデバッグ情報を表示
                DebugCollider();

                ImGui::Unindent(10);
            }

            ImGui::Spacing();

            // モデル描画チェックボックス
            bool modelDrawChanged = ImGui::Checkbox("モデル描画", &isModelDraw_);
            if (modelDrawChanged && isModelDraw_) {
                isWireframe_ = false;
            }

            // ワイヤーフレームチェックボックス
            bool wireframeChanged = ImGui::Checkbox("ワイヤーフレーム", &isWireframe_);
            if (wireframeChanged && isWireframe_) {
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

SphereCollider *BaseObject::AddSphereCollider(const std::string &name) {
    auto *collider = new SphereCollider();

    std::string colliderName = name.empty() ? objectName_ + "_SphereCollider" : name;
    collider->SetName(colliderName);

    // 位置と回転の取得関数を設定
    collider->SetPositionGetter([this]() { return this->GetWorldPosition(); });
    collider->SetRotationGetter([this]() { return this->GetWorldRotation(); });

    colliders_.push_back(collider);
    CollisionManager::GetInstance()->Register(collider);

    return collider;
}

AABBCollider *BaseObject::AddAABBCollider(const std::string &name) {
    auto *collider = new AABBCollider();

    std::string colliderName = name.empty() ? objectName_ + "_AABBCollider" : name;
    collider->SetName(colliderName);

    collider->SetPositionGetter([this]() { return this->GetWorldPosition(); });
    collider->SetRotationGetter([this]() { return this->GetWorldRotation(); });

    colliders_.push_back(collider);
    CollisionManager::GetInstance()->Register(collider);

    return collider;
}

OBBCollider *BaseObject::AddOBBCollider(const std::string &name) {
    auto *collider = new OBBCollider();

    std::string colliderName = name.empty() ? objectName_ + "_OBBCollider" : name;
    collider->SetName(colliderName);

    collider->SetPositionGetter([this]() { return this->GetWorldPosition(); });
    collider->SetRotationGetter([this]() { return this->GetWorldRotation(); });

    colliders_.push_back(collider);
    CollisionManager::GetInstance()->Register(collider);

    return collider;
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
}