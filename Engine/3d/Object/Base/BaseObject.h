#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Data/DataHandler.h"
#include "Object/Object3d.h"
#include "Transform/ObjColor.h"
#include "Transform/WorldTransform.h"
#include "collider/Collider.h"
#include "externals/nlohmann/json.hpp"
#include <string>

class SkyBox;
class BaseObject : public Collider {
  public:
    /// ===================================================
    /// private variaus
    /// ===================================================

    std::unique_ptr<DataHandler> ObjectDatas_;
    std::unique_ptr<DataHandler> AnimaDatas_;

  protected:
    /// ===================================================
    /// protected variaus
    /// ===================================================

    // モデル配列データ
    std::unique_ptr<Object3d> obj3d_;
    // ベースのワールド変換データ
    std::unique_ptr<WorldTransform> transform_;

    Vector3 worldPos;
    Quaternion q;
    Vector3 worldScale;
    // カラー
    ObjColor objColor_;
    // ライティング
    bool isLighting_ = true;
    bool isLoop_ = true;
    bool skeletonDraw_ = false;
    bool isModelDraw_ = true;
    bool isWireframe_ = false;
    bool reflect_ = false;
    bool isPrimitive_ = false;
    bool isRainbow_ = false;

    std::string objectName_;
    std::string modelPath_;
    std::string texturePath_;
    std::string foldarPath_ = "SceneData/Title/ObjectData";

    BaseObject *parent_ = nullptr;
    std::list<BaseObject *> children_;

    PrimitiveType type_ = PrimitiveType::kCount;

  private:
    using json = nlohmann::json;

  public:
    /// ===================================================
    /// public method
    /// ===================================================

    // 初期化、更新、描画
    virtual void Init(const std::string className);
    virtual void Update();
    virtual void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f});
    void UpdateWorldTransformHierarchy();
    void UpdateHierarchy();

    virtual void CreateModel(const std::string modelname);
    virtual void CreatePrimitiveModel(const PrimitiveType &type);
    virtual void AddCollider();

    virtual void ImGui();

    Vector3 GetCenterPosition() override;
    Quaternion GetCenterRotation() override;

    // 中心座標取得
    WorldTransform *GetWorldTransform() { return transform_.get(); }
    ModelAnimation *GetModelAnimation() { return obj3d_->GetCurrentModelAnimation(); }

    /// =================================================
    /// 親子付け
    /// =================================================

    void SetParent(BaseObject *parent);

    void AddChild(BaseObject *child);

    void DetachParent();

    void DetachChild(BaseObject *child);

    BaseObject *GetParent();

    std::list<BaseObject *> *GetChildren();

    BaseObject *GetChildByName(const std::string &name);

    /// ===================================================
    /// セーブロード
    /// ===================================================

    void SaveToJson();
    void LoadFromJson();
    void AnimaSaveToJson();
    void AnimaLoadFromJson();
    void SaveParentChildRelationship();
    void LoadParentChildRelationship();
    void SetFolderPath(const std::string &folderPath) { foldarPath_ = folderPath; }

    /// ===================================================
    /// getter
    /// ===================================================
    const WorldTransform &GetTransform() { return *transform_; }
    std::string &GetName() { return objectName_; }
    std::string &GetModelPath() { return modelPath_; }
    std::string &GetTexturePath() { return texturePath_; }
    std::string GetParentName() const;
    std::vector<std::string> GetChildrenNames() const;
    Object3d *GetObject3d() { return obj3d_.get(); }
    PrimitiveType GetPrimitiveType() { return type_; }
    Vector3 &GetLocalPosition() { return transform_->translation_; }
    Quaternion &GetLocalRotation() { return transform_->rotation_; }
    Vector3 &GetLocalScale() { return transform_->scale_; }
    Vector3 GetWorldPosition();
    Quaternion GetWorldRotation();
    Vector3 GetWorldScale();
    bool AnimaIsFinish() { return obj3d_->IsFinish(); }
    bool &GetLighting() { return isLighting_; }
    bool &GetLoop() { return isLoop_; }

    /// ===================================================
    /// setter
    /// ===================================================
    void SetTexture(const std::string &filePath, uint32_t index = 0) {
        if (filePath.empty()) {
            return; // ファイルパスが空なら何もしない
        }
        obj3d_->SetTexture(filePath, index);
    }
    void SetModel(std::unique_ptr<Object3d> obj) {
        obj3d_ = std::move(obj);
    }
    void SetModel(const std::string &filePath) { obj3d_->SetModel(filePath); }
    void SetAnima(const std::string &filePath) { obj3d_->SetAnimation(filePath); }
    // void AddAnimation(std::string filePath) { obj3d_->AddAnimation(filePath); }
    void SetBlendMode(BlendMode blendMode) { obj3d_->SetBlendMode(blendMode); }
    void SetReflect(bool reflect) { reflect_ = reflect; }
    void SetColor(const Vector4 &color) { objColor_.GetColor() = color; }

  private:
    void DebugObject();
    void DebugCollider();
    void ShowFileSelector();
    // ブレンドモードの選択UI
    void ShowBlendModeCombo(BlendMode &currentMode);

    std::vector<std::string> GetGltfFiles();
    std::vector<Collider *> colliders_;

    bool isCollider = false;
    BlendMode blendMode_;
};
