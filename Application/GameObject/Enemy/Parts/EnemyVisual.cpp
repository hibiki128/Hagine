#define NOMINMAX
#include "EnemyVisual.h"
#include "Application/GameObject/Enemy/Enemy.h"
#include "Application/GameObject/Player/Player.h"
#include "EnemyCombat.h"
#include "EnemyMovement.h"
#include "EnemyStatus.h"
#include <Frame.h>
#include <cmath>

using namespace Hagine;

void EnemyVisual::Init(Enemy *owner)
{
    owner_ = owner;

    // ───────────────────────────────────────────
    // アニメーションコントローラへクリップを登録（プレイヤーと同一構成）
    // ───────────────────────────────────────────
    animationController_.Initialize(owner_->GetObject3d());
    animationController_.RegisterClip("Idle", "animation/Player/Idle_Ground.gltf", true);
    animationController_.RegisterClip("FlyIdle", "animation/Player/Idle_Flying.gltf", true);
    animationController_.RegisterClip("Run", "animation/Player/Running.gltf", true);
    animationController_.RegisterClip("RunBack", "animation/Player/Running_Back.gltf", true);
    animationController_.RegisterClip("RunLeft", "animation/Player/Running_Left.gltf", true);
    animationController_.RegisterClip("RunRight", "animation/Player/Running_Right.gltf", true);
    animationController_.RegisterClip("FlyMove", "animation/Player/Running_Fly.gltf", true);
    animationController_.RegisterClip("Guard", "animation/Player/Block_Idle.gltf", true);
    animationController_.RegisterClip("Jump", "animation/Player/Running_Jamp.gltf", false);
    animationController_.RegisterClip("GuardHit", "animation/Player/Block_Hit.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_1", "animation/Player/Punch_1.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_2", "animation/Player/Punch_2.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_3", "animation/Player/Punch_3.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_4", "animation/Player/Punch_4.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_1", "animation/Player/Kick_1.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_2", "animation/Player/Kick_2.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_3", "animation/Player/Kick_3.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Smash", "animation/Player/Smash.gltf", false, 1.0f, 0.1f);
    // プレイヤー側で調整済みのクリップ設定（速度・補間）を流用する
    animationController_.LoadClips("AnimationController", "PlayerClips");
}

void EnemyVisual::UpdateAnimation()
{
    ComboSystem &punchCombo = owner_->Combat().GetPunchCombo();
    const std::vector<std::string> &comboAnimations = owner_->Combat().GetComboAnimations();

    // ──────────────────────────────────────────
    // コンボ攻撃中：段数に対応したアニメーションを再生（プレイヤーと同じロジック）
    // GetCurrentComboIndex() は「次に実行する」インデックスなので、
    // 現在再生中の段 = nextIdx - 1（0 のときは最終段の後）
    // ──────────────────────────────────────────
    if (punchCombo.IsComboActive())
    {
        int nextIdx = punchCombo.GetCurrentComboIndex();
        int comboLen = punchCombo.GetComboLength();
        int animIdx = (nextIdx == 0) ? (comboLen - 1) : (nextIdx - 1);

        if (animIdx >= 0 && animIdx < static_cast<int>(comboAnimations.size()))
        {
            const std::string &path = comboAnimations[animIdx];
            if (!path.empty())
            {
                animationController_.PlayFile(path, false, 1.0f, 0.1f);
            }
        }
        return; // 攻撃中は以降の判定をスキップ
    }

    // ガード中
    if (owner_->Status().IsGuarding())
    {
        animationController_.Play("Guard");
        return;
    }

    // ──────────────────────────────────────────
    // 移動状態でクリップを選択する。
    // 敵はBT駆動で moveDir_ を持たないため、velocity と向きから進行方向を判定する
    // ──────────────────────────────────────────
    EnemyMovement &mv = owner_->Movement();
    const Vector3 &velocity = mv.GetVelocity();
    Vector3 horizontalVel = {velocity.x, 0.0f, velocity.z};
    float hSpeed = horizontalVel.Length();
    bool verticalMove = std::abs(velocity.y) > kFlyVerticalAnimThreshold;

    if (mv.GetIsFlying() || !mv.GetIsGrounded())
    {
        // 飛行中 ― プレイヤーと同じ規則：
        // 後退・上昇・下降は Idle_Flying、前進・左右移動は Running_Fly
        if (hSpeed < kMoveAnimMinSpeed && !verticalMove)
        {
            animationController_.Play("FlyIdle");
            return;
        }

        // 「前進／後退」はプレイヤーの位置を基準に判定する。
        // 敵は常にプレイヤーへ向くため、プレイヤーへ近づく成分が＋＝前進、離れる成分が－＝後退。
        // 向き(GetForward)はクォータニオン規約の影響でプレイヤーと逆を指すことがあるため、
        // 実ワールド位置から求めたベクトルで判定する方が確実
        bool movingBackward = false;
        Player *target = owner_->GetTarget();
        if (target && hSpeed > kMinRotationDistance)
        {
            Vector3 toPlayer = target->GetWorldPosition() - owner_->GetWorldPosition();
            toPlayer.y = 0.0f;
            float toLen = toPlayer.Length();
            if (toLen > kMinRotationDistance)
            {
                float f = (horizontalVel / hSpeed).Dot(toPlayer / toLen); // +接近(前進) / -後退
                movingBackward = (f < -kBackwardDotThreshold);
            }
        }

        if (verticalMove || movingBackward)
        {
            animationController_.Play("FlyIdle");
        }
        else
        {
            animationController_.Play("FlyMove");
        }
        return;
    }

    // 地上 ― 待機 / 移動
    if (hSpeed < kMoveAnimMinSpeed)
    {
        animationController_.Play("Idle");
    }
    else
    {
        animationController_.Play("Run");
    }
}

void EnemyVisual::UpdateGuardBlink()
{
    // ガード中のエフェクト（点滅）
    if (owner_->Status().IsGuarding())
    {
        const float blinkInterval = kBlinkInterval;
        int blinkCount = static_cast<int>(Frame::Time() / blinkInterval);
        if (blinkCount % kBlinkModulo == kEvenBlink)
        {
            owner_->SetColor(Vector4(kColorOpaque, kColorZero, kColorZero, kColorOpaque));
        }
        else
        {
            owner_->SetColor(Vector4(kColorOpaque, kColorOpaque, kColorOpaque, kColorOpaque));
        }
    }
    else
    {
        owner_->SetColor(Vector4(kColorOpaque, kColorZero, kColorZero, kColorOpaque));
    }
}
