#pragma once
#include "Matrix4x4.h"
#include "Model.h"
#include "ObjColor.h"
#include "Object/Object3dCommon.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "ViewProjection/ViewProjection.h"
#include "WorldTransform.h"
#include "animation/ModelAnimation.h"
#include "light/LightGroup.h"
#include "string"
#include "vector"
#include"Material/Material.h"

class ModelCommon;
class Object3d {
  private: // メンバ変数
    struct Transform {
        Vector3 scale;
        Vector3 rotate;
        Vector3 translate;
    };

    // 座標変換行列データ
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
    };

    DirectXCommon *dxCommon_ = nullptr;
 
    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
    // バッファリソース内のデータを指すポインタ
    TransformationMatrix *transformationMatrixData = nullptr;

    //// バッファリソース
    //Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    //// バッファリソース内のデータを指すポインタ
    //Material *materialData = nullptr;

    std::unique_ptr<Material> material_;

    Transform transform;

    Model *model = nullptr;
    std::shared_ptr<ModelAnimation> currentModelAnimation_ = nullptr;
    std::map<std::string, std::shared_ptr<ModelAnimation>> modelAnimations_;
    ModelCommon *modelCommon = nullptr;
    LightGroup *lightGroup = nullptr;

    // 移動させる用各SRT
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 rotation = {0.0f, 0.0f, 0.0f};
    Vector3 size = {1.0f, 1.0f, 1.0f};
    bool HaveAnimation;

    std::string filePath_;
    std::unique_ptr<Object3dCommon> objectCommon_;
    BlendMode blendMode_ = BlendMode::kNone;

  public: // メンバ関数
    void Initialize();

    /// <summary>
    /// 初期化
    /// </summary>
    void CreateModel(const std::string &filePath);

    void CreatePrimitiveModel(const PrimitiveType &type);

    /// <summary>
    /// 更新
    /// </summary>
    void Update(const WorldTransform &worldTransform, const ViewProjection &viewProjection);

    /// <summary>
    /// アニメーションの更新
    /// </summary>
    void AnimationUpdate(bool roop);

    /// <summary>
    /// アニメーションの有無
    /// </summary>
    /// <param name="anime"></param>
    void SetStopAnimation(bool anime) { currentModelAnimation_->SetIsAnimation(anime); }

    /// <summary>
    /// アニメーションのセット
    /// </summary>
    /// <param name="fileName"></param>
    void SetAnimation(const std::string &fileName);

    /// <summary>
    /// アニメーション追加
    /// </summary>
    /// <param name="fileName"></param>
    void AddAnimation(const std::string &fileName);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const WorldTransform &worldTransform, const ViewProjection &viewProjection, ObjColor *color = nullptr, bool Lighting = true);

    /// <summary>
    /// スケルトン描画
    /// </summary>
    void DrawSkeleton(const WorldTransform &worldTransform, const ViewProjection &viewProjection);

    void PlayAnimation() { currentModelAnimation_->PlayAnimation(); }

    /// <summary>
    /// getter
    /// </summary>
    /// <returns></returns>
    const Vector3 &GetPosition() const { return position; }
    const Vector3 &GetRotation() const { return rotation; }
    const Vector3 &GetSize() const { return size; }
    const std::string GetTexture() const { return material_->GetMaterialDataGPU()->textureFilePath; }
    const bool &GetHaveAnimation() const { return HaveAnimation; }
    bool IsFinish() { return currentModelAnimation_->IsFinish(); }

    /// <summary>
    /// setter
    /// </summary>
    /// <param name="position"></param>
    void SetModel(Model *model) { this->model = model; }
    void SetPosition(const Vector3 &position) { this->position = position; }
    void SetRotation(const Vector3 &rotation) { this->rotation = rotation; }
    void SetSize(const Vector3 &size) { this->size = size; }
    void SetModel(const std::string &filePath);
    void SetTexture(const std::string &filePath);
    void SetUVTransform(const Matrix4x4 &mat) { material_->GetMaterialDataGPU()->uvTransform = mat; }
    void SetColor(const Vector4 &color) { material_->GetMaterialDataGPU()->color = color; }
    void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }

    /// <summary>
    /// 光沢度の設定
    /// </summary>
    /// <param name="shininess">マテリアルの光沢度</param>
    void SetShininess(float shininess = 20.0f);

  private: // メンバ関数
    /// <summary>
    /// 座標変換行列データ作成
    /// </summary>
    void CreateTransformationMatrix();

    /// <summary>
    /// マテリアルデータ作成
    /// </summary>
   // void CreateMaterial();

    Vector3 ExtractTranslation(const Matrix4x4 &matrix) {
        return Vector3(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]);
    }
};
