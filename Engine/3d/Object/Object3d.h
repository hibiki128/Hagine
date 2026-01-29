#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Object/Object3dCommon.h"
#include "animation/ModelAnimation.h"
#include "light/LightGroup.h"
#include "string"
#include "type/Matrix4x4.h"
#include "type/Vector2.h"
#include "type/Vector3.h"
#include "type/Vector4.h"
#include "vector"
#include <Graphics/PipeLine/PipeLineManager.h>
#include <Model/Material/Material.h>
#include <Model/Model.h>
#include <Transform/ObjColor.h>

class ModelCommon;
namespace Hagine::Graphics {
class Object3d {
  private: // メンバ変数
    struct Transform {
        Math::Vector3 scale;
        Math::Vector3 rotate;
        Math::Vector3 translate;
    };

    // 座標変換行列データ
    struct TransformationMatrix {
        Math::Matrix4x4 WVP;
        Math::Matrix4x4 World;
        Math::Matrix4x4 WorldInverseTranspose;
    };

    Core::DirectXCommon *dxCommon_ = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
    // バッファリソース内のデータを指すポインタ
    TransformationMatrix *transformationMatrixData = nullptr;

    Transform transform;

    Model *model = nullptr;
    std::shared_ptr<Animation::ModelAnimation> currentModelAnimation_ = nullptr;
    std::map<std::string, std::shared_ptr<Animation::ModelAnimation>> modelAnimations_;
    std::vector<std::unique_ptr<Material>> materials_;
    std::vector<Hagine::Transform::ObjColor> color_;
    ModelCommon *modelCommon = nullptr;
    Light::LightGroup *lightGroup = nullptr;

    // 移動させる用各SRT
    Math::Vector3 position = {0.0f, 0.0f, 0.0f};
    Math::Vector3 rotation = {0.0f, 0.0f, 0.0f};
    Math::Vector3 size = {1.0f, 1.0f, 1.0f};
    bool isPrimitive_ = false;
    bool isAnimationSwitchPending_ = false;
    std::string nextAnimationFileName_;

    std::string modelFilePath_;
    std::unique_ptr<Object3dCommon> objectCommon_;
    BlendMode blendMode_ = BlendMode::kNone;

  public: // メンバ関数
    void Initialize();

    /// <summary>
    /// 初期化
    /// </summary>
    void CreateModel(const std::string &filePath);

    void CreatePrimitiveModel(const PrimitiveType &type, std::string texPath);

    /// <summary>
    /// 更新
    /// </summary>
    void Update(const Hagine::Transform::WorldTransform &worldTransform, const Camera::ViewProjection &viewProjection);

    /// <summary>
    /// アニメーションの更新
    /// </summary>
    void AnimationUpdate(bool roop);

    /// <summary>
    /// 補間状態を取得
    /// </summary>
    bool IsAnimationBlending() const;

    /// <summary>
    /// 即座にアニメーション切り替え（補間なし、デバッグ用）
    /// </summary>
    void SetAnimationImmediate(const std::string &fileName);

    void SetAnimation(const std::string &animationFileName);

    /// <summary>
    /// アニメーションの有無
    /// </summary>
    /// <param name="anime"></param>
    void SetStopAnimation(bool anime) { currentModelAnimation_->SetIsAnimation(anime); }

    void DrawWireframe(const Hagine::Transform::WorldTransform &worldTransform, const Camera::ViewProjection &viewProjection, bool isRainbow = false);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const Hagine::Transform::WorldTransform &worldTransform, const Camera::ViewProjection &viewProjection, bool reflect, bool Lighting = true, bool modelDraw = true);

    /// <summary>
    /// スケルトン描画
    /// </summary>
    void DrawSkeleton(const Hagine::Transform::WorldTransform &worldTransform, const Camera::ViewProjection &viewProjection);

    void PlayAnimation() { currentModelAnimation_->PlayAnimation(); }

    /// <summary>
    /// getter
    /// </summary>
    /// <returns></returns>
    const Math::Vector3 &GetPosition() const { return position; }
    const Math::Vector3 &GetRotation() const { return rotation; }
    const Math::Vector3 &GetSize() const { return size; }
    size_t GetMaterialCount() const { return materials_.size(); }
    std::string GetModelFilePath() const { return modelFilePath_; }
    std::string GetTextureFilePath(uint32_t materialIndex) const {
        return materials_[materialIndex]->GetMaterialData().textureFilePath;
    }
    std::vector<std::string> GetAllTextruePath() {
        std::vector<std::string> texturePaths = {};
        for (int i = 0; i < GetMaterialCount(); i++) {
            texturePaths.push_back(materials_[i]->GetMaterialData().textureFilePath);
        }
        return texturePaths;
    }
    Animation::ModelAnimation *GetCurrentModelAnimation() const {
        return currentModelAnimation_.get();
    }

    const bool GetHaveAnimation() const { return model->GetModelData().hasAnimations; }
    bool IsFinish() { return currentModelAnimation_->IsFinish(); }

    Material *GetMaterial(uint32_t index) {
        return (index < materials_.size()) ? materials_[index].get() : nullptr;
    }
    Math::Vector4 GetColor(int index = 0) { return color_[index].GetColor(); }

    /// <summary>
    /// setter
    /// </summary>
    /// <param name="position"></param>
    void SetModel(Model *model) { this->model = model; }
    void SetPosition(const Math::Vector3 &position) { this->position = position; }
    void SetRotation(const Math::Vector3 &rotation) { this->rotation = rotation; }
    void SetSize(const Math::Vector3 &size) { this->size = size; }
    void SetModel(const std::string &filePath);
    void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
    void SetColor(Math::Vector4 color, int index = 0) { color_[index].SetColor(color); }

    // マルチマテリアル用のsetter
    void SetTexture(const std::string &filePath, uint32_t materialIndex);

    void SetEnvironmentCoefficients(float value);

  private: // メンバ関数
    /// <summary>
    /// アニメーション追加
    /// </summary>
    /// <param name="fileName"></param>
    void AddAnimation(const std::string &fileName);

    /// <summary>
    /// 座標変換行列データ作成
    /// </summary>
    void CreateTransformationMatrix();

    void CreateIndependentMaterials();

    void DrawBoneArmature(const Math::Vector3 &parentPos, const Math::Vector3 &childPos, float scale);

    void DrawArmatureShape(const Math::Vector3 &startPos, const Math::Vector3 &endPos, float baseWidth, float tipWidth, const Math::Vector4 &color);

    Math::Vector3 ExtractTranslation(const Math::Matrix4x4 &matrix) {
        return Math::Vector3(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]);
    }
};
} // namespace Hagine::Graphics