#pragma once
#include "ObjColor.h"
#include"Object/Object3d.h"
#include "ViewProjection/ViewProjection.h"
#include "WorldTransform.h"
#include "externals/nlohmann/json.hpp"
#include "collider/Collider.h"
#include <string>
#include"Data/DataHandler.h"

class BaseObject : public Collider {
  private:
    /// ===================================================
    /// private variaus
    /// ===================================================
     
    std::unique_ptr<DataHandler> TransformDatas_;
    std::unique_ptr<DataHandler> AnimaDatas_;
    
    
  protected:
    /// ===================================================
    /// protected variaus
    /// ===================================================

    // モデル配列データ
    std::unique_ptr<Object3d> obj3d_;
    // ベースのワールド変換データ
    WorldTransform transform_;
    // カラー
    ObjColor objColor_;
    // ライティング
    bool isLighting_;
    bool isLoop_;
    bool skeletonDraw_;

    std::string objectName_;

  private:
    using json = nlohmann::json;

  public:
    /// ===================================================
    /// public method
    /// ===================================================

    // 初期化、更新、描画
    virtual void Init(const std::string className);
    virtual void Update();
    virtual void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f,0.0f,0.0f});

    virtual void CreateModel(const std::string modelname);
    virtual void CreateCollider();

    virtual void DebugImGui();

    Vector3 GetCenterPosition() const override;
    Vector3 GetCenterRotation() const override;

    // 中心座標取得
    virtual Vector3 GetWorldPosition() const;
    virtual const WorldTransform &GetWorldTransform() const { return transform_; }

    /// ===================================================
    /// getter
    /// ===================================================
    const WorldTransform &GetTransform() { return transform_; }
    const Object3d *GetObject3d() { return obj3d_.get(); }
    bool AnimaIsFinish() { return obj3d_->IsFinish(); }

    /// ===================================================
    /// setter
    /// ===================================================
    void SetObjColor(Vector4 c) { objColor_.SetColor(c); }
    void SetWorldPosition(Vector3 pos) { transform_.translation_ = pos; }
    void SetWorldPositionY(float pos) { transform_.translation_.y = pos; }
    void SetWorldPositionX(float pos) { transform_.translation_.x = pos; }
    void SetWorldPositionZ(float pos) { transform_.translation_.z = pos; }
    void AddPosition(Vector3 pos) { transform_.translation_ += pos; }
    void SetRotation(Vector3 rotate) { transform_.rotation_ = rotate; }
    void SetRotationY(float rotate) { transform_.rotation_.y = rotate; }
    void SetRotationX(float rotate) { transform_.rotation_.x = rotate; }
    void SetScale(Vector3 scale) { transform_.scale_ = scale; }
    void SetLighting(bool isLighting) { isLighting_ = isLighting; }
    void SetTexture(const std::string &filePath) { obj3d_->SetTexture(filePath); }
    void SetParent(const WorldTransform *parent) { transform_.parent_ = parent; }
    void SetColor(const Vector4 &color) { objColor_.SetColor(color); }
    void SetModel(std::unique_ptr<Object3d> obj) {
        obj3d_ = std::move(obj);
    }
    void SetModel(const std::string &filePath) { obj3d_->SetModel(filePath); }
    void SetParent(const WorldTransform &wt) { transform_.parent_ = &wt; }
    void SetAnima(const std::string &filePath) { obj3d_->SetAnimation(filePath); }
    void SetLoop(bool flag) { isLoop_ = flag; }
    void AddAnimation(std::string filePath) { obj3d_->AddAnimation(filePath); }


  private:
    void DebugTransform();
    void DebugCollider();
    void SaveToJson();
    void LoadFromJson();
    void AnimaSaveToJson();
    void AnimaLoadFromJson();
    void ShowFileSelector();

    std::vector<std::string> GetGltfFiles();

    bool isCollider = false;
};
