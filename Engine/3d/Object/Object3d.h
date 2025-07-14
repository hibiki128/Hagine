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
#include <Model/Model.h>
#include <Transform/ObjColor.h>

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

    void DrawWireframe(const WorldTransform &worldTransform, const ViewProjection &viewProjection, bool isRainbow = false);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const WorldTransform &worldTransform, const ViewProjection &viewProjection, bool reflect, ObjColor *color = nullptr, bool Lighting = true, bool modelDraw = true);

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
    size_t GetMaterialCount() const { return model->GetMaterialCount(); }
    std::string GetModelFilePath() const { return modelFilePath_; }
    std::string GetTextureFilePath(uint32_t materialIndex) const {
        return model->GetMaterial(materialIndex)->GetMaterialData().textureFilePath;
    }
    ModelAnimation *GetCurrentModelAnimation() const {
        return currentModelAnimation_.get();
    }

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
    void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }

    // マルチマテリアル用のsetter
    void SetTexture(const std::string &filePath, uint32_t materialIndex) {
        model->SetTexture(filePath, materialIndex);
    }

    void SetEnvironmentCoefficients(float value) {
        model->SetEnvironmentCoefficients(value);
    }

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

    void DrawBoneArmature(const Vector3 &parentPos, const Vector3 &childPos, float scale);

    void DrawArmatureShape(const Vector3 &startPos, const Vector3 &endPos, float baseWidth, float tipWidth, const Vector4 &color);

    Vector3 ExtractTranslation(const Matrix4x4 &matrix) {
        return Vector3(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]);
    }
};