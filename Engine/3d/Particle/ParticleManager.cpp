#include "ParticleManager.h"
#include "Engine/Frame/Frame.h"
#include "Texture/TextureManager.h"
#include "fstream"
#include <random>

void ParticleManager::Initialize(SrvManager *srvManager) {
    particleCommon = ParticleCommon::GetInstance();
    srvManager_ = srvManager;
    randomEngine.seed(seedGenerator());
}

void ParticleManager::Update(const ViewProjection &viewProjection) {
    // ビュー射影行列の計算
    Matrix4x4 viewProjectionMatrix = viewProjection.matView_ * viewProjection.matProjection_;

    // カメラの回転成分のみを抽出してビルボード行列を作成
    Matrix4x4 billboardMatrix = viewProjection.matView_;
    billboardMatrix.m[3][0] = 0.0f;
    billboardMatrix.m[3][1] = 0.0f;
    billboardMatrix.m[3][2] = 0.0f;
    billboardMatrix.m[3][3] = 1.0f;

    // ビルボード行列を逆行列にすることで、カメラに対して正しい向きにする
    billboardMatrix = Inverse(billboardMatrix);

    for (auto &[groupName, particleGroup] : particleGroups_) {

        uint32_t numInstance = 0;

        // 各パーティクルの更新
        for (auto particleIterator = particleGroup->GetParticleGroupData().particles.begin();
             particleIterator != particleGroup->GetParticleGroupData().particles.end();) {

            // パーティクルの生存時間チェック
            if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) {
                particleIterator = particleGroup->GetParticleGroupData().particles.erase(particleIterator);
                continue;
            }
            // パーティクルの生存時間に基づく進行度 t を計算
            float t = (*particleIterator).currentTime / (*particleIterator).lifeTime;
            t = std::clamp(t, 0.0f, 1.0f);

            // 拡縮処理
            if (isSinMove_) {
                // Sin波の周波数制御 (速度調整)
                float waveScale = 0.5f * (sin(t * DirectX::XM_PI * 18.0f) + 1.0f); // 0 ～ 1 の範囲

                // 最大スケールが寿命に応じて縮小し、最終的に0になる
                float maxScale = (1.0f - t); // 1 -> 0 に線形に縮小

                // Sin波スケールと最大スケールの積を適用
                (*particleIterator).transform.scale_ =
                    (*particleIterator).startScale * waveScale * maxScale;
            } else {
                // 通常の線形補間
                (*particleIterator).transform.scale_ =
                    (1.0f - t) * (*particleIterator).startScale + t * (*particleIterator).endScale;

                // ギャザーモードが有効でない場合のみ、通常のアルファ値計算を行う
                if (!(isGatherMode_ && t >= gatherStartRatio_)) {
                    // アルファ値の計算
                    (*particleIterator).color.w = (*particleIterator).initialAlpha - ((*particleIterator).currentTime / (*particleIterator).lifeTime);
                }
            }

            // ギャザーモードとそれ以外の移動処理を分岐
            bool isGathering = false;
            if (isGatherMode_ && t >= gatherStartRatio_) {
                isGathering = true;
                // ギャザー効果の強さを計算（ライフタイム終盤ほど強く）
                float gatherFactor = (t - gatherStartRatio_) / (1.0f - gatherStartRatio_);
                gatherFactor = std::clamp(gatherFactor, 0.0f, 1.0f);

                // エミッター中心（パーティクル発生元）への方向ベクトル
                Vector3 toEmitter = (*particleIterator).emitterPosition - (*particleIterator).transform.translation_;
                float distance = toEmitter.Length();

                // 中心に近づくほどアルファ値を下げる（フェードアウト効果）
                // 距離が0に近づくほど透明に
                float distanceBasedAlpha = distance / (distance + 0.5f); // 調整可能なソフトニング係数
                (*particleIterator).color.w = (*particleIterator).initialAlpha * (1.0f - gatherFactor) * distanceBasedAlpha;

                // 中心に非常に近い場合はパーティクルを削除（震え防止）
                if (distance < 0.05f) {
                    // 非常に近い場合は生存時間を終了させて次の更新で削除されるようにする
                    (*particleIterator).currentTime = (*particleIterator).lifeTime;
                    ++particleIterator;
                    continue;
                }

                // 距離に応じてギャザー速度を緩和（近いほど緩やかに）
                float distanceFactor = std::min(1.0f, distance); // 距離が1以下の場合は距離自体を係数に

                // 方向ベクトルを正規化
                toEmitter = toEmitter.Normalize();

                // 中心に向かう速度を計算（近づくほど減速）
                float gatherSpeed = gatherStrength_ * gatherFactor * distanceFactor * 3.0f; // 調整済み係数
                Vector3 gatherVelocity = toEmitter * gatherSpeed * Frame::DeltaTime();

                // ギャザー中は通常の速度を無視して、エミッターに向かう速度のみを設定
                (*particleIterator).velocity = gatherVelocity;

                // ギャザー中のパーティクル位置を更新
                (*particleIterator).transform.translation_ += (*particleIterator).velocity;
            }

            // ギャザー中でない場合のみ通常の移動処理を行う
            if (!isGathering) {
                (*particleIterator).Acce = (1.0f - t) * (*particleIterator).startAcce + t * (*particleIterator).endAcce;

                if (isFaceDirection_) {
                    // 固定された進行方向を使用して回転を設定
                    Vector3 forward = (*particleIterator).fixedDirection; // 初期に保存された進行方向
                    Vector3 initialUp = {0.0f, 1.0f, 0.0f};

                    // 回転軸を計算
                    Vector3 rotationAxis = initialUp.Cross(forward).Normalize();
                    float dotProduct = initialUp.Dot(forward);
                    float angle = acosf(std::clamp(dotProduct, -1.0f, 1.0f)); // 安全な範囲にクランプ

                    // 回転を設定
                    (*particleIterator).transform.rotation_.x = rotationAxis.x * angle;
                    (*particleIterator).transform.rotation_.y = rotationAxis.y * angle;
                    (*particleIterator).transform.rotation_.z = rotationAxis.z * angle;

                } else if (isRandomRotate_) {
                    // ランダム回転の場合の処理
                    (*particleIterator).transform.rotation_ += (*particleIterator).rotateVelocity;
                } else {
                    // 通常の回転補間
                    (*particleIterator).transform.rotation_ =
                        (1.0f - t) * (*particleIterator).startRote + t * (*particleIterator).endRote;
                }

                // 加速度処理
                if (isAcceMultipy_) {
                    (*particleIterator).velocity *= (*particleIterator).Acce;
                } else {
                    (*particleIterator).velocity += (*particleIterator).Acce;
                }

                // パーティクルの移動
                (*particleIterator).transform.translation_ +=
                    (*particleIterator).velocity * Frame::DeltaTime();
            }

            // 時間更新はギャザーモードに関わらず行う
            (*particleIterator).currentTime += Frame::DeltaTime();

            // ワールド行列の計算
            Matrix4x4 worldMatrix{};
            if (isBillboard) {
                // ビルボード処理の場合、カメラ方向に向ける
                worldMatrix = MakeScaleMatrix((*particleIterator).transform.scale_) * billboardMatrix *
                              MakeTranslateMatrix((*particleIterator).transform.translation_);
            } else {
                // 通常のアフィン変換行列を使用
                worldMatrix = MakeAffineMatrix((*particleIterator).transform.scale_,
                                               (*particleIterator).transform.rotation_,
                                               (*particleIterator).transform.translation_);
            }

            // パーティクルのワールド位置をチェック
            WorldTransform particleWorldTransform;
            particleWorldTransform.translation_ = (*particleIterator).transform.translation_;

            //// 視野外判定
            // if (viewProjection.IsOutsideViewFrustum(particleWorldTransform)) {
            //	// 視野外であれば、パーティクルを削除
            //	particleIterator = particleGroup.particles.erase(particleIterator);
            //	continue;
            // }

            // ワールド・ビュー・プロジェクション行列の計算
            Matrix4x4 worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

            // インスタンスデータに設定
            if (numInstance < particleGroup->GetMaxInstance()) {
                particleGroup->GetParticleGroupData().instancingData[numInstance].WVP = worldViewProjectionMatrix;
                particleGroup->GetParticleGroupData().instancingData[numInstance].World = worldMatrix;
                particleGroup->GetParticleGroupData().instancingData[numInstance].color = (*particleIterator).color;
                particleGroup->GetParticleGroupData().instancingData[numInstance].color.w = (*particleIterator).color.w; // アルファ値の設定
                ++numInstance;
            }

            ++particleIterator;
        }

        // インスタンス数の更新
        particleGroup->GetParticleGroupData().instanceCount = numInstance;
    }
}

void ParticleManager::Draw() {

    for (auto &[groupName, particleGroup] : particleGroups_) {
        particleCommon->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&particleGroup->GetIndexBufferView());
        particleCommon->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &particleGroup->GetVertexBufferView());

        if (particleGroup->GetParticleGroupData().instanceCount > 0) {
            particleCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, particleGroup->GetmaterialResource()->GetGPUVirtualAddress());

            srvManager_->SetGraphicsRootDescriptorTable(1, particleGroup->GetParticleGroupData().instancingSRVIndex);

            srvManager_->SetGraphicsRootDescriptorTable(2, particleGroup->GetParticleGroupData().material.textureIndex);

            particleCommon->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(particleGroup->GetModelData().indices.size()), particleGroup->GetParticleGroupData().instanceCount, 0, 0,0);
        }
    }
}

void ParticleManager::AddParticleGroup(ParticleGroup *particleGroup) {
    assert(particleGroup);
    particleGroups_.insert(std::pair(particleGroup->GetGroupName(), particleGroup));
    particleGroupNames_.push_back(particleGroup->GetGroupName());
}

Particle ParticleManager::MakeNewParticle(
    std::mt19937 &randomEngine,
    const Vector3 &translate,
    const Vector3 &rotation,
    const Vector3 &scale,                                   // スケールを引数として受け取る
    const Vector3 &velocityMin, const Vector3 &velocityMax, // 速度の範囲
    float lifeTimeMin, float lifeTimeMax,
    const Vector3 &particleStartScale, const Vector3 &particleEndScale,
    const Vector3 &startAcce, const Vector3 &endAcce,
    const Vector3 &startRote, const Vector3 &endRote,
    bool isRamdomColor, float alphaMin, float alphaMax,
    const Vector3 &rotateVelocityMin, const Vector3 &rotateVelocityMax,
    const Vector3 &allScaleMax, const Vector3 &allScaleMin,
    const float &scaleMin, const float &scaleMax,
    const Vector3 &rotateStartMax, const Vector3 &rotateStartMin) {
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distVelocityX(velocityMin.x, velocityMax.x);
    std::uniform_real_distribution<float> distVelocityY(velocityMin.y, velocityMax.y);
    std::uniform_real_distribution<float> distVelocityZ(velocityMin.z, velocityMax.z);
    std::uniform_real_distribution<float> distLifeTime(lifeTimeMin, lifeTimeMax);
    std::uniform_real_distribution<float> distAlpha(alphaMin, alphaMax);

    Particle particle;
    Vector3 randomTranslate;
    particle.emitterPosition = translate;
    if (isEmitOnEdge_) {
        // 立方体の12本のエッジ上にパーティクルを生成する場合
        std::uniform_int_distribution<int> edgeSelector(0, 11);         // 12本のエッジからランダム選択
        std::uniform_real_distribution<float> edgePosition(0.0f, 1.0f); // エッジ上の位置（0〜1）

        int selectedEdge = edgeSelector(randomEngine);
        float position = edgePosition(randomEngine);

        // 立方体の8つの頂点の相対座標（スケール適用前）
        const Vector3 v0 = {-1.0f, -1.0f, -1.0f}; // 左下手前
        const Vector3 v1 = {1.0f, -1.0f, -1.0f};  // 右下手前
        const Vector3 v2 = {-1.0f, 1.0f, -1.0f};  // 左上手前
        const Vector3 v3 = {1.0f, 1.0f, -1.0f};   // 右上手前
        const Vector3 v4 = {-1.0f, -1.0f, 1.0f};  // 左下奥
        const Vector3 v5 = {1.0f, -1.0f, 1.0f};   // 右下奥
        const Vector3 v6 = {-1.0f, 1.0f, 1.0f};   // 左上奥
        const Vector3 v7 = {1.0f, 1.0f, 1.0f};    // 右上奥

        // エッジの定義（始点と終点のインデックス）
        const std::pair<Vector3, Vector3> edges[] = {
            {v0, v1}, {v1, v3}, {v3, v2}, {v2, v0}, // 前面
            {v4, v5},
            {v5, v7},
            {v7, v6},
            {v6, v4}, // 背面
            {v0, v4},
            {v1, v5},
            {v2, v6},
            {v3, v7} // 側面
        };

        // 選択されたエッジの始点と終点
        const Vector3 &start = edges[selectedEdge].first;
        const Vector3 &end = edges[selectedEdge].second;

        // エッジ上の位置を線形補間で計算
        randomTranslate = {
            start.x + (end.x - start.x) * position,
            start.y + (end.y - start.y) * position,
            start.z + (end.z - start.z) * position};

        // スケールを適用
        randomTranslate.x *= scale.x;
        randomTranslate.y *= scale.y;
        randomTranslate.z *= scale.z;
    } else {
        // 通常のランダムな位置生成
        randomTranslate = {
            distribution(randomEngine) * scale.x,
            distribution(randomEngine) * scale.y,
            distribution(randomEngine) * scale.z};
    }

    // 回転行列を適用してランダムな位置を回転
    Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(rotation);

    // ランダム位置を回転行列で変換
    Vector3 rotatedPosition = {
        randomTranslate.x * rotationMatrix.m[0][0] + randomTranslate.y * rotationMatrix.m[1][0] + randomTranslate.z * rotationMatrix.m[2][0],
        randomTranslate.x * rotationMatrix.m[0][1] + randomTranslate.y * rotationMatrix.m[1][1] + randomTranslate.z * rotationMatrix.m[2][1],
        randomTranslate.x * rotationMatrix.m[0][2] + randomTranslate.y * rotationMatrix.m[1][2] + randomTranslate.z * rotationMatrix.m[2][2]};

    // 回転されたランダムな位置をトランスレーションに加算
    particle.transform.translation_ = translate + rotatedPosition;

    if (isRandomAllSize_) {
        std::uniform_real_distribution<float> distScaleX(allScaleMin.x, allScaleMax.x);
        std::uniform_real_distribution<float> distScaleY(allScaleMin.y, allScaleMax.y);
        std::uniform_real_distribution<float> distScaleZ(allScaleMin.z, allScaleMax.z);
        particle.startScale = {distScaleX(randomEngine), distScaleY(randomEngine), distScaleZ(randomEngine)};
        if (isEndScale_) {
            particle.endScale = particle.startScale;
        }
    } else if (isRandomSize_) {
        std::uniform_real_distribution<float> distScale(scaleMin, scaleMax);
        particle.startScale.x = distScale(randomEngine);
        particle.startScale.y = particle.startScale.x;
        particle.startScale.z = particle.startScale.x;
    } else {
        particle.startScale = particleStartScale;
    }
    if (!isEndScale_) {
        particle.endScale = particleEndScale;
    }

    particle.startAcce = startAcce;
    particle.endAcce = endAcce;

    // パーティクルの速度をランダムに設定
    Vector3 randomVelocity = {
        distVelocityX(randomEngine),
        distVelocityY(randomEngine),
        distVelocityZ(randomEngine)};

    // エミッターの回転を速度ベクトルに適用
    particle.velocity = {
        randomVelocity.x * rotationMatrix.m[0][0] + randomVelocity.y * rotationMatrix.m[1][0] + randomVelocity.z * rotationMatrix.m[2][0],
        randomVelocity.x * rotationMatrix.m[0][1] + randomVelocity.y * rotationMatrix.m[1][1] + randomVelocity.z * rotationMatrix.m[2][1],
        randomVelocity.x * rotationMatrix.m[0][2] + randomVelocity.y * rotationMatrix.m[1][2] + randomVelocity.z * rotationMatrix.m[2][2]};

    if (isRandomRotate_) {
        std::uniform_real_distribution<float> distRotateX(rotateStartMin.x, rotateStartMax.x);
        std::uniform_real_distribution<float> distRotateY(rotateStartMin.y, rotateStartMax.y);
        std::uniform_real_distribution<float> distRotateZ(rotateStartMin.z, rotateStartMax.z);
        particle.transform.rotation_.x = distRotateX(randomEngine);
        particle.transform.rotation_.y = distRotateY(randomEngine);
        particle.transform.rotation_.z = distRotateZ(randomEngine);
        if (isRotateVelocity_) {
            // 回転速度をランダムに設定
            std::uniform_real_distribution<float> distRotateXVelocity(rotateVelocityMin.x, rotateVelocityMax.x);
            std::uniform_real_distribution<float> distRotateYVelocity(rotateVelocityMin.y, rotateVelocityMax.y);
            std::uniform_real_distribution<float> distRotateZVelocity(rotateVelocityMin.z, rotateVelocityMax.z);
            particle.rotateVelocity.x = distRotateXVelocity(randomEngine);
            particle.rotateVelocity.y = distRotateYVelocity(randomEngine);
            particle.rotateVelocity.z = distRotateZVelocity(randomEngine);
        }
    } else {
        particle.startRote = startRote;
        particle.endRote = endRote;
    }

    if (isRamdomColor) {
        // ランダムな色を設定
        std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
        particle.color = {distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), distAlpha(randomEngine)};
    } else {
        particle.color = {1.0f, 1.0f, 1.0f, distAlpha(randomEngine)};
    }
    if (isFaceDirection_) {

        // 初期向きは上方向（0, 1, 0）
        Vector3 initialUp = {0.0f, 1.0f, 0.0f};

        // パーティクルの進行方向（velocity）を正規化
        Vector3 forward = particle.velocity.Normalize();

        // 固定された進行方向として保存
        particle.fixedDirection = forward;

        // 回転軸を計算（初期向きと進行方向の外積）
        Vector3 rotationAxis = initialUp.Cross(forward).Normalize();

        // 初期向きと進行方向のなす角を計算
        float dotProduct = initialUp.Dot(forward);
        float angle = acosf(std::clamp(dotProduct, -1.0f, 1.0f)); // 安全のためclamp

        // 回転軸と角度から回転を生成
        particle.transform.rotation_.x = rotationAxis.x * angle; // 軸回転の簡易的な設定
        particle.transform.rotation_.y = rotationAxis.y * angle;
        particle.transform.rotation_.z = rotationAxis.z * angle;
    }

    particle.initialAlpha = distAlpha(randomEngine);
    // ライフタイムをランダムに設定
    particle.lifeTime = distLifeTime(randomEngine);
    particle.currentTime = 0.0f;

    return particle;
}

// 全てのパーティクルグループに対してパーティクルを生成する関数
std::list<Particle> ParticleManager::Emit(
    const Vector3 &position,
    uint32_t count,
    const Vector3 &scale,
    const Vector3 &velocityMin, const Vector3 &velocityMax,
    float lifeTimeMin, float lifeTimeMax,
    const Vector3 &particleStartScale, const Vector3 &particleEndScale,
    const Vector3 &startAcce, const Vector3 &endAcce,
    const Vector3 &startRote, const Vector3 &endRote,
    bool isRandomColor, float alphaMin, float alphaMax,
    const Vector3 &rotateVelocityMin, const Vector3 &rotateVelocityMax,
    const Vector3 &allScaleMax, const Vector3 &allScaleMin,
    const float &scaleMin, const float &scaleMax, const Vector3 &rotation,
    const Vector3 &rotateStartMax, const Vector3 &rotateStartMin) {

    std::list<Particle> allNewParticles; // 生成された全パーティクルを格納

    // 全てのパーティクルグループに対してループ
    for (auto &[groupName, particleGroup] : particleGroups_) {
        std::list<Particle> newParticles; // 各グループごとの新規パーティクルリスト

        for (uint32_t nowCount = 0; nowCount < count; ++nowCount) {
            // 新しいパーティクルを生成
            Particle particle = MakeNewParticle(
                randomEngine,
                position,
                rotation,
                scale,
                velocityMin, velocityMax,
                lifeTimeMin, lifeTimeMax,
                particleStartScale, particleEndScale,
                startAcce, endAcce,
                startRote, endRote,
                isRandomColor,
                alphaMin, alphaMax,
                rotateVelocityMin, rotateVelocityMax,
                allScaleMax, allScaleMin,
                scaleMin, scaleMax,
                rotateStartMax, rotateStartMin);

            newParticles.push_back(particle); // 生成したパーティクルをリストに追加
        }

        // 各グループのパーティクルデータに追加
        particleGroup->GetParticleGroupData().particles.splice(
            particleGroup->GetParticleGroupData().particles.end(),
            newParticles);

        // 全体の返却用リストにも追加
        allNewParticles.splice(allNewParticles.end(), newParticles);
    }

    return allNewParticles; // 全グループに生成されたパーティクルをまとめて返す
}
