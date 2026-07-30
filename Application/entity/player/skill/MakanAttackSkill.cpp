#include "MakanAttackSkill.h"
#include "Application/entity/enemy/Enemy.h"
#include "Application/entity/player/Player.h"
#include "object/base/BaseObjectManager.h"
#include "particle/gpu/ParticleCSSpawner.h"
#include <Frame.h>
#include <cmath>
using namespace Hagine;
void MakanAttackSkill::Init(const std::string objectName)
{
    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cube);
    pMakanCollider_ = AddOBBCollider("makan_Collider");
    pMakanCollider_->SetTag("Makan");
    pMakanCollider_->AddCollisionMask("Enemy");

    pMakanCollider_->SetOnCollisionEnter([this](ColliderBase *pOther) {
        this->OnCollisionEnter(pOther);
    });

    /// メインビーム（Spawn したエミッターの更新・描画はエンジンが自動で回す）
    pMakanMainEffect_ = ParticleCSSpawner::GetInstance()->Spawn("makan_main");

    /// らせん状エミッター
    pMakanAroundEffect_ = ParticleCSSpawner::GetInstance()->Spawn("makan_around");
}

void MakanAttackSkill::Update()
{
    BaseObject::Update();

    if (!isActive_ || !pPlayerTransform_)
    {
        pMakanCollider_->SetEnabled(false);
        return;
    }

    // 長さを伸ばす
    currentLength_ += extendSpeed_ * Frame::DeltaTime();
    if (currentLength_ > maxLength_)
    {
        currentLength_ = maxLength_;
    }

    // 発射起点は手（右手ジョイント）に追従させる。向きは発動時のまま固定
    transform_->translation_ = GetBeamOrigin();
    transform_->quaternionRotation_ = lockedRotation_; // 発動時の向きで固定（発動後は追従しない）

    // コライダー設定
    pMakanCollider_->SetEnabled(true);
    pMakanCollider_->SetSize(Vector3(beamWidth_, beamHeight_, currentLength_));
    pMakanCollider_->SetAnchorPoint(Vector3(0.5f, 0.5f, 1.0f));

    // メインビーム設定
    if (pMakanMainEffect_)
    {
        pMakanMainEffect_->SetAuto(true);
        pMakanMainEffect_->SetScale(Vector3(0.0f, 0.0f, currentLength_));
        pMakanMainEffect_->SetAnchorPoint(Vector3(0.5f, 0.5f, 0.75f));
        pMakanMainEffect_->SetTranslate(transform_->translation_);
        pMakanMainEffect_->SetRotation(lockedRotation_);
    }

    // らせん状エミッター（新しいアプローチ）
    if (pMakanAroundEffect_)
    {
        pMakanAroundEffect_->SetAuto(true);
        spiralTime_ += Frame::DeltaTime();

        // 発動時に固定した向きのローカル座標系の基底ベクトルを直接計算
        // クォータニオンから回転行列を作成
        Quaternion q = lockedRotation_;

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
        Vector3 emitterPos = transform_->translation_ +
                             localForward * forwardDistance +
                             spiralOffset;

        pMakanAroundEffect_->SetTranslate(emitterPos);
        pMakanAroundEffect_->SetScale(Vector3(0.3f, 0.3f, 0.3f));
    }

    // 持続時間チェック
    activeTime_ += Frame::DeltaTime();
    if (activeTime_ >= duration_)
    {
        Deactivate();
    }
}

Vector3 MakanAttackSkill::GetBeamOrigin()
{
    // 両手を前に突き出すモーションなので、両手ジョイントの中点から発射する。
    // アニメーションに合わせて毎フレーム追従する
    if (pPlayer_)
    {
        auto rightHand = pPlayer_->GetJointWorldPosition(kRightHandJointName);
        auto leftHand = pPlayer_->GetJointWorldPosition(kLeftHandJointName);
        if (rightHand && leftHand)
        {
            return (*rightHand + *leftHand) * 0.5f;
        }
        if (rightHand)
        {
            return *rightHand;
        }
    }

    // ジョイントが取得できない場合は従来どおり胸元の固定オフセットで代用する
    Vector3 origin = pPlayerTransform_->translation_;
    origin.y += kLockOffsetY;
    return origin;
}

void MakanAttackSkill::Activate(WorldTransform *playerTransform)
{
    if (isActive_)
        return;

    isActive_ = true;
    pPlayerTransform_ = playerTransform;
    // 発動の瞬間の向きをスナップショットして固定する（発動後のホーミング防止）
    lockedRotation_ = playerTransform->quaternionRotation_;
    currentLength_ = 0.0f;
    activeTime_ = 0.0f;
    spiralTime_ = 0.0f;
}

void MakanAttackSkill::Deactivate()
{
    isActive_ = false;
    currentLength_ = 0.0f;
    activeTime_ = 0.0f;
    spiralTime_ = 0.0f;
    pPlayerTransform_ = nullptr;
    pMakanCollider_->SetEnabled(false);
    if (pMakanMainEffect_)
    {
        pMakanMainEffect_->SetAuto(false);
    }
    if (pMakanAroundEffect_)
    {
        pMakanAroundEffect_->SetAuto(false);
    }
}

void MakanAttackSkill::Draw(const ViewProjection &viewProjection)
{
}

void MakanAttackSkill::OnCollisionEnter(ColliderBase *pOther)
{
    if (pOther->GetTag() == "Enemy")
    {
        // プレイヤーの敵が存在し、生きている場合
        if (pPlayer_ && pPlayer_->GetEnemy() && pPlayer_->GetEnemy()->GetAlive())
        {
            isAlive_ = false;

            // 必殺技被弾：ビームの進行方向（プレイヤー→敵）へ大きく吹き飛ばし、
            // そのまま地面まで落下する大スタンにする。予約はダメージ適用より先に行う
            Enemy *pEnemy = pPlayer_->GetEnemy();
            Vector3 blowDir = pEnemy->GetWorldPosition() - pPlayer_->GetWorldPosition();
            blowDir.y = 0.0f;
            pEnemy->Status().RequestSkillBlowReaction(blowDir);

            // チャージ度合いに応じたダメージを計算して適用（ガードされた場合は必殺技分のエネルギーを削る）
            float damage = 37.5f;
            pEnemy->SetDamage(damage, false, true);

            // 命中したらビーム演出を終了し、パーティクルの新規発生を止める
            // （既存の粒子は寿命どおり自然に消える）。判定コライダーも即無効化される。
            Deactivate();
        }
    }
}

void MakanAttackSkill::DrawImGui()
{
#ifdef _DEBUG

    if (ImGui::TreeNode("MakanAttackSkill Debug"))
    {
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

        if (pPlayerTransform_)
        {
            ImGui::Separator();
            ImGui::Text("Player Transform");
            ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                        pPlayerTransform_->translation_.x,
                        pPlayerTransform_->translation_.y,
                        pPlayerTransform_->translation_.z);
            ImGui::Text("Rotation: (%.2f, %.2f, %.2f, %.2f)",
                        pPlayerTransform_->quaternionRotation_.x,
                        pPlayerTransform_->quaternionRotation_.y,
                        pPlayerTransform_->quaternionRotation_.z,
                        pPlayerTransform_->quaternionRotation_.w);

            // ローカル座標系の基底ベクトル計算
            Quaternion q = pPlayerTransform_->quaternionRotation_;
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

            if (pMakanAroundEffect_)
            {
                ImGui::Separator();
                ImGui::Text("Spiral Emitter Position: (%.2f, %.2f, %.2f)",
                            pMakanAroundEffect_->GetTranslate().x,
                            pMakanAroundEffect_->GetTranslate().y,
                            pMakanAroundEffect_->GetTranslate().z);
            }
        }

        ImGui::TreePop();
    }
#endif // _DEBUG
}