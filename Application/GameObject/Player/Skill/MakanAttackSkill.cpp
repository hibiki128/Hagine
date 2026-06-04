#include "MakanAttackSkill.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "Object/Base/BaseObjectManager.h"
#include "Particle/CSParticle/ParticleCSEditor.h"
#include <Frame.h>
#include <cmath>
using namespace Hagine;
void MakanAttackSkill::Init(const std::string objectName) {
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cube);
    makanCollider_ = AddOBBCollider("makan_Collider");
    makanCollider_->SetTag("Makan");
    makanCollider_->AddCollisionMask("Enemy");

    makanCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    /// メインビーム
    makanMainEffect_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("makan_main");

    /// らせん状エミッター
    makanAroundEffect_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("makan_around");
}

void MakanAttackSkill::Update() {
    BaseObject::Update();
    makanMainEffect_->Update();
    makanAroundEffect_->Update();

    if (!isActive_ || !playerTransform_) {
        makanCollider_->SetEnabled(false);
        return;
    }

    // 長さを伸ばす
    currentLength_ += extendSpeed_ * Frame::DeltaTime();
    if (currentLength_ > maxLength_) {
        currentLength_ = maxLength_;
    }

    transform_->translation_ = playerTransform_->translation_;
    transform_->quateRotation_ = playerTransform_->quateRotation_;

    // コライダー設定
    makanCollider_->SetEnabled(true);
    makanCollider_->SetSize(Vector3(beamWidth_, beamHeight_, currentLength_));
    makanCollider_->SetAnchorPoint(Vector3(0.5f, 0.5f, 1.0f));

    // メインビーム設定
    if (makanMainEffect_) {
        makanMainEffect_->SetAuto(true);
        makanMainEffect_->SetScale(Vector3(0.0f, 0.0f, currentLength_));
        makanMainEffect_->SetAnchorPoint(Vector3(0.5f, 0.5f, 0.75f));
        makanMainEffect_->SetTranslate(playerTransform_->translation_);
        makanMainEffect_->SetRotation(playerTransform_->quateRotation_);
    }

    // らせん状エミッター（新しいアプローチ）
    if (makanAroundEffect_) {
        makanAroundEffect_->SetAuto(true);
        spiralTime_ += Frame::DeltaTime();

        // プレイヤーのローカル座標系の基底ベクトルを直接計算
        // クォータニオンから回転行列を作成
        Quaternion q = playerTransform_->quateRotation_;

        // ローカルX軸(右方向)
        Vector3 localRight(
            1.0f - 2.0f * (q.y * q.y + q.z * q.z),
            2.0f * (q.x * q.y - q.w * q.z),
            2.0f * (q.x * q.z + q.w * q.y));

        // ローカルY軸(上方向)
        Vector3 localUp(
            2.0f * (q.x * q.y + q.w * q.z), 
            1.0f - 2.0f * (q.x * q.x + q.z * q.z),
            2.0f * (q.y * q.z - q.w * q.x)); 

        // ローカルZ軸(前方向)
        Vector3 localForward(
            2.0f * (q.x * q.z - q.w * q.y), 
            2.0f * (q.y * q.z + q.w * q.x), 
            1.0f - 2.0f * (q.x * q.x + q.y * q.y));

        // 前進距離
        float forwardDistance = (spiralTime_ * spiralForwardSpeed_) * 2.0f;
        if (forwardDistance > currentLength_ * 2.0f)
            forwardDistance = currentLength_ * 2.0f;

        // 角度計算
        float t = forwardDistance / maxLength_;
        float angle = t * spiralRevolution_ * (2.0f * std::numbers::pi_v<float>);

        // らせんオフセット（ローカル座標系で計算）
        Vector3 spiralOffset = localRight * (std::cos(angle) * spiralRadius_) +
                               localUp * (std::sin(angle) * spiralRadius_);

        // エミッター位置（ワールド座標）
        Vector3 emitterPos = playerTransform_->translation_ +
                             localForward * forwardDistance +
                             spiralOffset;

        makanAroundEffect_->SetTranslate(emitterPos);
        makanAroundEffect_->SetScale(Vector3(0.3f, 0.3f, 0.3f));
    }

    // 持続時間チェック
    activeTime_ += Frame::DeltaTime();
    if (activeTime_ >= duration_) {
        Deactivate();
    }
}

void MakanAttackSkill::Activate(WorldTransform *playerTransform) {
    if (isActive_)
        return;

    isActive_ = true;
    playerTransform_ = playerTransform;
    currentLength_ = 0.0f;
    activeTime_ = 0.0f;
    spiralTime_ = 0.0f;
}

void MakanAttackSkill::Deactivate() {
    isActive_ = false;
    currentLength_ = 0.0f;
    activeTime_ = 0.0f;
    spiralTime_ = 0.0f;
    playerTransform_ = nullptr;
    makanCollider_->SetEnabled(false);
    makanMainEffect_->SetAuto(false);
    makanAroundEffect_->SetAuto(false);
}

void MakanAttackSkill::Draw(const ViewProjection &viewProjection) {
}

void MakanAttackSkill::DrawParticle(const ViewProjection &viewProjection) {
    if (makanMainEffect_) {
        makanMainEffect_->Draw(viewProjection);
    }
    if (makanAroundEffect_) {
        makanAroundEffect_->Draw(viewProjection);
    }
}

void MakanAttackSkill::OnCollisionEnter(ColliderBase *other) {
    if (other->GetTag() == "Enemy") {
        // プレイヤーの敵が存在し、生きている場合
        if (player_ && player_->GetEnemy() && player_->GetEnemy()->GetAlive()) {
            isAlive_ = false;

            // チャージ度合いに応じたダメージを計算して適用
            float damage = 37.5f;
            player_->GetEnemy()->SetDamage(damage);
        }
    }
}

void MakanAttackSkill::DebugImGui() {
#ifdef _DEBUG


    if (ImGui::TreeNode("MakanAttackSkill Debug")) {
        ImGui::Checkbox("Is Active", &isActive_);
        ImGui::Text("Current Length: %.2f / %.2f", currentLength_, maxLength_);
        ImGui::Text("Active Time: %.2f / %.2f", activeTime_, duration_);
        ImGui::Text("Spiral Time: %.2f", spiralTime_);

        ImGui::Separator();
        ImGui::Text("Parameters");
        ImGui::DragFloat("Extend Speed", &extendSpeed_, 1.0f, 0.0f, 500.0f);
        ImGui::DragFloat("Spiral Radius", &spiralRadius_, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Spiral Revolution", &spiralRevolution_, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Spiral Forward Speed", &spiralForwardSpeed_, 1.0f, 0.0f, 100.0f);

        if (playerTransform_) {
            ImGui::Separator();
            ImGui::Text("Player Transform");
            ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                        playerTransform_->translation_.x,
                        playerTransform_->translation_.y,
                        playerTransform_->translation_.z);
            ImGui::Text("Rotation: (%.2f, %.2f, %.2f, %.2f)",
                        playerTransform_->quateRotation_.x,
                        playerTransform_->quateRotation_.y,
                        playerTransform_->quateRotation_.z,
                        playerTransform_->quateRotation_.w);

            // ローカル座標系の基底ベクトル計算
            Quaternion q = playerTransform_->quateRotation_;
            Vector3 localRight(
                1.0f - 2.0f * (q.y * q.y + q.z * q.z),
                2.0f * (q.x * q.y + q.w * q.z),
                2.0f * (q.x * q.z - q.w * q.y));
            Vector3 localUp(
                2.0f * (q.x * q.y - q.w * q.z),
                1.0f - 2.0f * (q.x * q.x + q.z * q.z),
                2.0f * (q.y * q.z + q.w * q.x));
            Vector3 localForward(
                2.0f * (q.x * q.z + q.w * q.y),
                2.0f * (q.y * q.z - q.w * q.x),
                1.0f - 2.0f * (q.x * q.x + q.y * q.y));

            ImGui::Separator();
            ImGui::Text("Local Axes");
            ImGui::Text("Right:   (%.2f, %.2f, %.2f)", localRight.x, localRight.y, localRight.z);
            ImGui::Text("Up:      (%.2f, %.2f, %.2f)", localUp.x, localUp.y, localUp.z);
            ImGui::Text("Forward: (%.2f, %.2f, %.2f)", localForward.x, localForward.y, localForward.z);

            if (makanAroundEffect_) {
                ImGui::Separator();
                ImGui::Text("Spiral Emitter Position: (%.2f, %.2f, %.2f)",
                            makanAroundEffect_->GetTranslate().x,
                            makanAroundEffect_->GetTranslate().y,
                            makanAroundEffect_->GetTranslate().z);
            }
        }

        ImGui::TreePop();
    }
#endif // _DEBUG
}