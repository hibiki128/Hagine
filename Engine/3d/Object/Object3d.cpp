#include "Object3d.h"
#include "Object3dCommon.h"
#include "cassert"
#include "myMath.h"
#include <Model/ModelManager.h>
#include <Texture/TextureManager.h>
#include <line/DrawLine3D.h>

void Object3d::Initialize() {
    objectCommon_ = std::make_unique<Object3dCommon>();
    objectCommon_->Initialize();

    dxCommon_ = DirectXCommon::GetInstance();

    lightGroup = LightGroup::GetInstance();

    CreateTransformationMatrix();

    material_ = std::make_unique<Material>();
    material_->Initialize();
}

void Object3d::CreateModel(const std::string &filePath) {

    filePath_ = filePath;

    ModelManager::GetInstance()->LoadModel(filePath_);

    // モデルを検索してセットする
    model = ModelManager::GetInstance()->FindModel(filePath_);

    material_->GetMaterialDataGPU()->textureFilePath = model->GetModelData().material.textureFilePath;
    material_->GetMaterialDataGPU()->textureIndex = model->GetModelData().material.textureIndex;
    if (model->IsGltf()) {
        currentModelAnimation_ = std::make_unique<ModelAnimation>();
        currentModelAnimation_->SetModelData(model->GetModelData());
        currentModelAnimation_->Initialize("resources/models/", filePath_);

        model->SetAnimator(currentModelAnimation_->GetAnimator());
        model->SetBone(currentModelAnimation_->GetBone());
        model->SetSkin(currentModelAnimation_->GetSkin());
    }
}

void Object3d::CreatePrimitiveModel(const PrimitiveType &type) {
    model = ModelManager::GetInstance()->FindModel(ModelManager::GetInstance()->CreatePrimitiveModel(type));
    material_->GetMaterialDataGPU()->color = model->GetModelData().material.color;
    material_->GetMaterialDataGPU()->uvTransform = model->GetModelData().material.uvTransform;
    material_->GetMaterialDataGPU()->textureFilePath = model->GetModelData().material.textureFilePath;
    material_->GetMaterialDataGPU()->textureIndex = model->GetModelData().material.textureIndex;
}

void Object3d::Update(const WorldTransform &worldTransform, const ViewProjection &viewProjection) {
    if (lightGroup) {
        lightGroup->Update(viewProjection);
    }
    Matrix4x4 worldMatrix = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);

    if (worldTransform.parent_) {
        worldMatrix *= worldTransform.parent_->matWorld_;
    }
    Matrix4x4 worldViewProjectionMatrix;
    const Matrix4x4 &viewProjectionMatrix = viewProjection.matView_ * viewProjection.matProjection_;
    worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

    transformationMatrixData->WVP = worldViewProjectionMatrix;
    transformationMatrixData->World = worldTransform.matWorld_;
    Matrix4x4 worldInverseMatrix = Inverse(worldMatrix);
    transformationMatrixData->WorldInverseTranspose = Transpose(worldInverseMatrix);
}

void Object3d::AnimationUpdate(bool roop) {
    if (currentModelAnimation_) {
        currentModelAnimation_->Update(roop);
    }
}

void Object3d::SetAnimation(const std::string &fileName) {
    // すでにセット済みのアニメーションなら何もしない
    if (fileName == filePath_) {
        return;
    }

    // modelAnimations_ 内に fileName に対応するアニメーションがあるか検索
    auto it = modelAnimations_.find(fileName);

    // アニメーションが見つからなかった場合、強制的にプログラムを停止
    assert(it != modelAnimations_.end() && "Error: Animation file not found in modelAnimations_!");

    // 見つかったアニメーションを shared_ptr に格納
    currentModelAnimation_ = it->second;

    // Animator などを model にセット
    model->SetAnimator(currentModelAnimation_->GetAnimator());
    model->SetBone(currentModelAnimation_->GetBone());
    model->SetSkin(currentModelAnimation_->GetSkin());
    currentModelAnimation_->GetAnimator()->SetIsAnimation(true);
    currentModelAnimation_->GetAnimator()->SetAnimationTime(0.0f);

    // ファイルパスを更新
    filePath_ = fileName;
}

void Object3d::AddAnimation(const std::string &fileName) {
    auto animation = std::make_unique<ModelAnimation>();

    animation->SetModelData(model->GetModelData());
    animation->Initialize("resources/models/", fileName);
    animation->GetAnimator()->SetAnimationTime(0.0f);

    modelAnimations_.emplace(fileName, std::move(animation));
}

void Object3d::Draw(const WorldTransform &worldTransform, const ViewProjection &viewProjection, ObjColor *color, bool Lighting) {

    /*if (viewProjection.IsOutsideViewFrustum(worldTransform)) {
            return;
    }*/

    objectCommon_->SetBlendMode(blendMode_);

    Update(worldTransform, viewProjection);

    if (model->IsGltf()) {
        if (currentModelAnimation_->GetAnimator()->HaveAnimation()) {
            HaveAnimation = true;
            objectCommon_->skinningDrawCommonSetting();
        } else {
            HaveAnimation = false;
        }
    }

    // wvp用のCBufferの場所を設定
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
    material_->Draw(color->GetColor(), Lighting);
    if (material_->GetMaterialDataGPU()->enableLighting != 0 && lightGroup) {
        lightGroup->Draw();
    }
    // マテリアルCBufferの場所を設定
    if (model) {
        model->Draw(objectCommon_.get());
    }
}

void Object3d::DrawSkeleton(const WorldTransform &worldTransform, const ViewProjection &viewProjection) {
    Update(worldTransform, viewProjection);
    // スケルトンデータを取得
    const Skeleton &skeleton = currentModelAnimation_->GetSkeletonData();

    // 各ジョイントを巡回して親子関係の線を生成
    for (const auto &joint : skeleton.joints) {
        // 親がいない場合、このジョイントはルートなのでスキップ
        if (!joint.parent.has_value()) {
            continue;
        }

        // 親ジョイントを取得
        const auto &parentJoint = skeleton.joints[*joint.parent];

        // 親と子のスケルトン空間座標を取得
        Vector3 parentPosition = ExtractTranslation(parentJoint.skeletonSpaceMatrix);
        Vector3 childPosition = ExtractTranslation(joint.skeletonSpaceMatrix);

        // 線の色を設定（デフォルトで白色）
        Vector4 lineColor = {1.0f, 1.0f, 1.0f, 1.0f};

        // LineManagerに現在の線分を登録
        DrawLine3D::GetInstance()->SetPoints(parentPosition, childPosition, lineColor);
    }
}

void Object3d::SetModel(const std::string &filePath) {
    // モデルを検索してセットする
    ModelManager::GetInstance()->LoadModel(filePath);
    model = ModelManager::GetInstance()->FindModel(filePath);

    material_->GetMaterialDataGPU()->textureFilePath = model->GetModelData().material.textureFilePath;
    material_->GetMaterialDataGPU()->textureIndex = model->GetModelData().material.textureIndex;
    if (model->IsGltf()) {

        currentModelAnimation_->SetModelData(model->GetModelData());

        currentModelAnimation_->Initialize("resources/models/", filePath);

        model->SetAnimator(currentModelAnimation_->GetAnimator());
        model->SetBone(currentModelAnimation_->GetBone());
        model->SetSkin(currentModelAnimation_->GetSkin());
    }
}

void Object3d::SetTexture(const std::string &filePath) {
    material_->GetMaterialDataGPU()->textureFilePath = filePath;
    TextureManager::GetInstance()->LoadTexture(filePath);
    material_->GetMaterialDataGPU()->textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);
    model->SetMaterialData(*material_->GetMaterialDataGPU());
}

void Object3d::SetShininess(float shininess) {
    material_->GetMaterialDataGPU()->shininess = shininess;
}

void Object3d::CreateTransformationMatrix() {
    transformationMatrixResource = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    // 書き込むかめのアドレスを取得
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));
    // 単位行列を書き込んでおく
    transformationMatrixData->WVP = MakeIdentity4x4();
    transformationMatrixData->World = MakeIdentity4x4();
    transformationMatrixData->WorldInverseTranspose = MakeIdentity4x4();
}
