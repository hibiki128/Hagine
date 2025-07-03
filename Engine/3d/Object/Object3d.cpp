#define NOMINMAX
#include "Object3d.h"
#include "DirectXCommon.h"
#include "Graphics/Model/ModelManager.h"
#include "Object3dCommon.h"
#include "Transform/WorldTransform.h"
#include "cassert"
#include <Frame.h>
#include <line/DrawLine3D.h>
#include <myMath.h>
#include <type/Matrix4x4.h>

void Object3d::Initialize() {
    objectCommon_ = std::make_unique<Object3dCommon>();
    objectCommon_->Initialize();

    dxCommon_ = DirectXCommon::GetInstance();

    lightGroup = LightGroup::GetInstance();

    CreateTransformationMatrix();
}

void Object3d::CreateModel(const std::string &filePath) {
    filePath_ = filePath;

    ModelManager::GetInstance()->LoadModel(filePath_);

    // モデルを検索してセットする
    model = ModelManager::GetInstance()->FindModel(filePath_);

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
    isPrimitive_ = true;
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
    transformationMatrixData->World = worldMatrix;
    Matrix4x4 worldInverseMatrix = Inverse(worldMatrix);
    transformationMatrixData->WorldInverseTranspose = Transpose(worldInverseMatrix);

    if (model && model->IsGltf()) {
        if (currentModelAnimation_->GetAnimator()->HaveAnimation()) {
            objectCommon_->computeSkinningDrawCommonSetting();
            model->Update();
        }
    }
}

void Object3d::Draw(const WorldTransform &worldTransform, const ViewProjection &viewProjection, bool reflect, ObjColor *color, bool lighting) {
    objectCommon_->SetBlendMode(blendMode_);
    Update(worldTransform, viewProjection);

    // アニメーション設定
    if (model && model->IsGltf()) {
        if (currentModelAnimation_->GetAnimator()->HaveAnimation()) {
            HaveAnimation = true;
            objectCommon_->skinningDrawCommonSetting();
        } else {
            HaveAnimation = false;
        }
    }

    // 変換行列設定
    dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

    // ライティング設定
    if (lightGroup) {
        lightGroup->Draw();
    }

    // モデル描画
    if (model) {
        Vector4 drawColor = color ? color->GetColor() : Vector4{1.0f, 1.0f, 1.0f, 1.0f};
        model->Draw(drawColor, lighting, reflect);
    }
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

void Object3d::DrawWireframe(const WorldTransform &worldTransform, const ViewProjection &viewProjection) {
    // worldTransformを更新
    Update(worldTransform, viewProjection);
    if (!model) {
        return;
    }

    const ModelData &modelData = model->GetModelData();

    // ====== フラグで切り替え可能 ======
    static bool gamingMode = false;

    // ====== 時間カウンター（時間ベースで変化）======
    static float timeCounter = 0.0f;
    timeCounter += Frame::DeltaTime() / 7.0f;
    if (timeCounter > 100.0f)
        timeCounter = 0.0f;

    // ====== HSV -> RGB変換関数 ======
    auto HSVtoRGB = [](float h, float s, float v) -> Vector4 {
        float c = v * s;
        float x = c * (1.0f - abs(fmod(h * 6.0f, 2.0f) - 1.0f));
        float m = v - c;
        float r, g, b;

        if (h < 1.0f / 6.0f) {
            r = c;
            g = x;
            b = 0;
        } else if (h < 2.0f / 6.0f) {
            r = x;
            g = c;
            b = 0;
        } else if (h < 3.0f / 6.0f) {
            r = 0;
            g = c;
            b = x;
        } else if (h < 4.0f / 6.0f) {
            r = 0;
            g = x;
            b = c;
        } else if (h < 5.0f / 6.0f) {
            r = x;
            g = 0;
            b = c;
        } else {
            r = c;
            g = 0;
            b = x;
        }

        return {r + m, g + m, b + m, 1.0f};
    };

    // ====== グラデーション用関数（時間ベース） ======
    auto GetTimeGradientColor = [&](const Vector3 &worldPos) -> Vector4 {
        Vector4 clipPos = Transformation(Vector4{worldPos.x, worldPos.y, worldPos.z, 1.0f},
                                         viewProjection.matView_ * viewProjection.matProjection_);

        Vector2 ndc = {clipPos.x / clipPos.w, clipPos.y / clipPos.w};
        Vector2 screenUV = {(ndc.x + 1.0f) * 0.5f, (1.0f - ndc.y) * 0.5f};

        float distance = (screenUV.x + screenUV.y) / 2.0f;
        float hue = fmod(timeCounter + distance * 0.5f, 1.0f);
        return HSVtoRGB(hue, 1.0f, 1.0f);
    };

    for (const auto &mesh : modelData.meshes) {
        const std::vector<VertexData> &vertices = mesh.vertices;
        const std::vector<uint32_t> &indices = mesh.indices;

        auto drawTriangle = [&](const Vector3 &v0, const Vector3 &v1, const Vector3 &v2) {
            if (gamingMode) {
                Vector4 c0 = GetTimeGradientColor(v0);
                Vector4 c1 = GetTimeGradientColor(v1);
                Vector4 c2 = GetTimeGradientColor(v2);
                DrawLine3D::GetInstance()->SetPoints(v0, v1, c0);
                DrawLine3D::GetInstance()->SetPoints(v1, v2, c1);
                DrawLine3D::GetInstance()->SetPoints(v2, v0, c2);
            } else {
                Vector4 wireframeColor = {1.0f, 1.0f, 1.0f, 1.0f};
                DrawLine3D::GetInstance()->SetPoints(v0, v1, wireframeColor);
                DrawLine3D::GetInstance()->SetPoints(v1, v2, wireframeColor);
                DrawLine3D::GetInstance()->SetPoints(v2, v0, wireframeColor);
            }
        };

        if (indices.empty()) {
            for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
                // 修正：transformationMatrixData->Worldを使用
                Vector4 v0_4 = Transformation(Vector4{vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, 1.0f}, transformationMatrixData->World);
                Vector4 v1_4 = Transformation(Vector4{vertices[i + 1].position.x, vertices[i + 1].position.y, vertices[i + 1].position.z, 1.0f}, transformationMatrixData->World);
                Vector4 v2_4 = Transformation(Vector4{vertices[i + 2].position.x, vertices[i + 2].position.y, vertices[i + 2].position.z, 1.0f}, transformationMatrixData->World);

                drawTriangle({v0_4.x, v0_4.y, v0_4.z}, {v1_4.x, v1_4.y, v1_4.z}, {v2_4.x, v2_4.y, v2_4.z});
            }
        } else {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                uint32_t idx0 = indices[i];
                uint32_t idx1 = indices[i + 1];
                uint32_t idx2 = indices[i + 2];

                if (idx0 >= vertices.size() || idx1 >= vertices.size() || idx2 >= vertices.size())
                    continue;

                // 修正：transformationMatrixData->Worldを使用
                Vector4 v0_4 = Transformation(Vector4{vertices[idx0].position.x, vertices[idx0].position.y, vertices[idx0].position.z, 1.0f}, transformationMatrixData->World);
                Vector4 v1_4 = Transformation(Vector4{vertices[idx1].position.x, vertices[idx1].position.y, vertices[idx1].position.z, 1.0f}, transformationMatrixData->World);
                Vector4 v2_4 = Transformation(Vector4{vertices[idx2].position.x, vertices[idx2].position.y, vertices[idx2].position.z, 1.0f}, transformationMatrixData->World);

                drawTriangle({v0_4.x, v0_4.y, v0_4.z}, {v1_4.x, v1_4.y, v1_4.z}, {v2_4.x, v2_4.y, v2_4.z});
            }
        }
    }
}

void Object3d::DrawSkeleton(const WorldTransform &worldTransform, const ViewProjection &viewProjection) {
    const Skeleton &skeleton = currentModelAnimation_->GetSkeletonData();

    // モデルに適用されているワールド変換を生成
    Matrix4x4 worldMatrix = MakeAffineMatrix(
        worldTransform.scale_,
        worldTransform.rotation_,
        worldTransform.translation_);
    if (worldTransform.parent_) {
        worldMatrix *= worldTransform.parent_->matWorld_;
    }

    for (const auto &joint : skeleton.joints) {
        // ローカル座標 → ワールド座標に変換
        Matrix4x4 jointWorldMat = joint.skeletonSpaceMatrix * worldMatrix;
        Vector3 jointPosition = ExtractTranslation(jointWorldMat);

        Vector4 jointColor = {0.8f, 0.2f, 0.2f, 1.0f};
        float jointRadius = 0.03f;
        DrawLine3D::GetInstance()->DrawSphere(jointPosition, jointColor, jointRadius, 8);

        if (!joint.parent.has_value()) {
            continue;
        }

        const auto &parentJoint = skeleton.joints[*joint.parent];
        Matrix4x4 parentWorldMat = parentJoint.skeletonSpaceMatrix * worldMatrix;
        Vector3 parentPosition = ExtractTranslation(parentWorldMat);

        DrawBoneArmature(parentPosition, jointPosition);
    }
}

void Object3d::DrawBoneArmature(const Vector3 &parentPos, const Vector3 &childPos) {
    // ボーンの方向ベクトル
    Vector3 boneDirection = childPos - parentPos;
    float boneLength = boneDirection.Length();

    if (boneLength < 0.001f)
        return; // 長さが短すぎる場合はスキップ

    // 正規化された方向ベクトル
    Vector3 normalizedDirection = boneDirection.Normalize();

    // ボーンの太さ（長さに比例）
    float baseWidth = boneLength * 0.1f; // 基部の太さ
    float tipWidth = boneLength * 0.02f; // 先端の太さ

    // 最小・最大の太さを制限
    baseWidth = std::max(0.02f, std::min(baseWidth, 0.15f));
    tipWidth = std::max(0.005f, std::min(tipWidth, 0.05f));

    // ボーンの色
    Vector4 boneColor = {0.2f, 0.6f, 1.0f, 1.0f}; // 青系の色

    // アーマチュア形状を描画
    DrawArmatureShape(parentPos, childPos, baseWidth, tipWidth, boneColor);
}

void Object3d::DrawArmatureShape(const Vector3 &startPos, const Vector3 &endPos,
                                 float baseWidth, float tipWidth, const Vector4 &color) {
    // ボーンの方向ベクトル
    Vector3 direction = endPos - startPos;
    float length = direction.Length();

    if (length < 0.001f)
        return;

    Vector3 normalizedDir = direction.Normalize();

    // 垂直なベクトルを2つ作成（ボーンの断面用）
    Vector3 up = {0.0f, 1.0f, 0.0f};
    if (std::abs(normalizedDir.Dot(up)) > 0.9f) {
        up = {1.0f, 0.0f, 0.0f}; // 方向がY軸に近い場合はX軸を使用
    }

    Vector3 right = (normalizedDir.Cross(up)).Normalize();
    up = (right.Cross(normalizedDir)).Normalize();

    // 分割数
    const int segments = 8;       // 断面の分割数
    const int lengthSegments = 4; // 長さ方向の分割数

    // 各セグメントでボーンを描画
    for (int i = 0; i < lengthSegments; i++) {
        float t1 = (float)i / lengthSegments;
        float t2 = (float)(i + 1) / lengthSegments;

        // 現在の位置と次の位置
        Vector3 pos1 = startPos + direction * t1;
        Vector3 pos2 = startPos + direction * t2;

        // 現在の太さ（線形補間で基部から先端に向かって細くなる）
        float width1 = baseWidth * (1.0f - t1) + tipWidth * t1;
        float width2 = baseWidth * (1.0f - t2) + tipWidth * t2;

        // 断面の円を描画
        for (int j = 0; j < segments; j++) {
            float angle1 = (float)j / segments * 2.0f * 3.14159f;
            float angle2 = (float)(j + 1) / segments * 2.0f * 3.14159f;

            // 現在のセグメントの円周上の点
            Vector3 p1_1 = pos1 + (right * cosf(angle1) + up * sinf(angle1)) * width1;
            Vector3 p1_2 = pos1 + (right * cosf(angle2) + up * sinf(angle2)) * width1;
            Vector3 p2_1 = pos2 + (right * cosf(angle1) + up * sinf(angle1)) * width2;
            Vector3 p2_2 = pos2 + (right * cosf(angle2) + up * sinf(angle2)) * width2;

            // 円周の線
            DrawLine3D::GetInstance()->SetPoints(p1_1, p1_2, color);
            DrawLine3D::GetInstance()->SetPoints(p2_1, p2_2, color);

            // 縦の線（長さ方向）
            DrawLine3D::GetInstance()->SetPoints(p1_1, p2_1, color);

            // 最後のセグメントの場合、先端を中心点に収束させる
            if (i == lengthSegments - 1) {
                DrawLine3D::GetInstance()->SetPoints(p2_1, endPos, color);
            }
        }
    }

    // 基部から中心軸への線も描画（強調用）
    DrawLine3D::GetInstance()->SetPoints(startPos, endPos, {color.x * 1.2f, color.y * 1.2f, color.z * 1.2f, color.w});
}

void Object3d::SetModel(const std::string &filePath) {
    // モデルを検索してセットする
    ModelManager::GetInstance()->LoadModel(filePath);
    model = ModelManager::GetInstance()->FindModel(filePath);

    if (model->IsGltf()) {
        currentModelAnimation_->SetModelData(model->GetModelData());
        currentModelAnimation_->Initialize("resources/models/", filePath);

        model->SetAnimator(currentModelAnimation_->GetAnimator());
        model->SetBone(currentModelAnimation_->GetBone());
        model->SetSkin(currentModelAnimation_->GetSkin());
    }
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