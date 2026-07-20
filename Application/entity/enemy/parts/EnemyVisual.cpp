#define NOMINMAX
#include "EnemyVisual.h"
#include "Application/entity/enemy/Enemy.h"
#include "Application/entity/player/Player.h"
#include "EnemyCombat.h"
#include "EnemyMovement.h"
#include "EnemyStatus.h"
#include <Frame.h>
#include <cmath>

using namespace Hagine;

void EnemyVisual::Init(Enemy *owner)
{
    pOwner_ = owner;

    // ───────────────────────────────────────────
    // アニメーションコントローラへクリップを登録（プレイヤーと同一構成）
    // ───────────────────────────────────────────
    animationController_.Initialize(pOwner_->GetObject3d());
    animationController_.RegisterClip("Idle", "animation/Player/Idle_Ground.gltf", true);
    animationController_.RegisterClip("FlyIdle", "animation/Player/Idle_Flying.gltf", true);
    animationController_.RegisterClip("Run", "animation/Player/Running.gltf", true);
    animationController_.RegisterClip("RunBack", "animation/Player/Running_Back.gltf", true);
    animationController_.RegisterClip("RunLeft", "animation/Player/Running_Left.gltf", true);
    animationController_.RegisterClip("RunRight", "animation/Player/Running_Right.gltf", true);
    animationController_.RegisterClip("FlyMove", "animation/Player/Running_Fly.gltf", true);
    animationController_.RegisterClip("Guard", "animation/Player/Block_Idle.gltf", true);
    animationController_.RegisterClip("Jump", "animation/Player/Running_Jump.gltf", false);
    animationController_.RegisterClip("Die", "animation/Player/die.gltf", false, 1.0f, 0.15f);
    animationController_.RegisterClip("MakanSkill", "animation/Player/MakanSkill.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("GuardHit", "animation/Player/Block_Hit.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_1", "animation/Player/Punch_1.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_2", "animation/Player/Punch_2.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_3", "animation/Player/Punch_3.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Punch_4", "animation/Player/Punch_4.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_1", "animation/Player/Kick_1.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_2", "animation/Player/Kick_2.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Kick_3", "animation/Player/Kick_3.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Smash", "animation/Player/Smash.gltf", false, 1.0f, 0.1f);
    animationController_.RegisterClip("Shot", "animation/Player/Shot.gltf", false, 1.0f, 0.1f);
    // プレイヤー側で調整済みのクリップ設定（速度・補間）を流用する
    animationController_.LoadClips("AnimationController", "PlayerClips");
}

void EnemyVisual::UpdateAnimation()
{
    // ──────────────────────────────────────────
    // ビーム必殺技中：発動前演出〜ビーム終了まで一続きの専用モーションを再生
    // （プレイヤーの必殺技と同じく、構え→フレーム30付近で発射→フレーム80で撃ち終わり）
    // ──────────────────────────────────────────
    if (pOwner_->Combat().IsBeamStaging() || pOwner_->Combat().IsBeamActive())
    {
        animationController_.Play("MakanSkill");
        return;
    }

    ComboSystem &punchCombo = pOwner_->Combat().GetPunchCombo();
    const std::vector<std::string> &comboAnimations = pOwner_->Combat().GetComboAnimations();

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

    // ──────────────────────────────────────────
    // ガード中：Shot（射撃）モーションの再生中でも防御姿勢を優先する。
    // Shot 判定より後に置くと、射撃直後にガードを始めたとき Shot が
    // 再生し終わるまでガードアニメが適用されない
    // ──────────────────────────────────────────
    if (pOwner_->Status().IsGuarding())
    {
        animationController_.Play("Guard");
        return;
    }

    // ──────────────────────────────────────────
    // 通常弾の発射モーション中：再生し終わるまでステートによる切り替えで上書きしない
    // （発射のたびに PlayShotAnimation() で先頭から再生し直される）
    // ──────────────────────────────────────────
    if (animationController_.GetCurrentClipName() == "Shot" && !animationController_.IsFinished())
    {
        return;
    }

    // ──────────────────────────────────────────
    // 移動状態でクリップを選択する。
    // 敵はBT駆動で moveDir_ を持たないため、velocity と向きから進行方向を判定する
    // ──────────────────────────────────────────
    EnemyMovement &mv = pOwner_->Movement();
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
        Player *target = pOwner_->GetTarget();
        if (target && hSpeed > kMinRotationDistance)
        {
            Vector3 toPlayer = target->GetWorldPosition() - pOwner_->GetWorldPosition();
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

void EnemyVisual::PlayDeathAnimation()
{
    animationController_.Play("Die");
}

void EnemyVisual::PlayShotAnimation()
{
    // 同一クリップ再生中の Play() は無視されるため、既に Shot 中なら先頭へ巻き戻して再生し直す
    if (animationController_.GetCurrentClipName() == "Shot")
    {
        animationController_.SetTime(0.0f);
        animationController_.SetPaused(false); // 再生終了で停止していた場合に再開する
    }
    else
    {
        animationController_.Play("Shot");
    }
}

bool EnemyVisual::IsDeathAnimationFinished() const
{
    // 死亡クリップ再生中に最後（非ループのため終端で停止）まで到達したか
    return animationController_.GetCurrentClipName() == "Die" &&
           animationController_.IsFinished();
}

void EnemyVisual::UpdateGuardBlink()
{
    // ガード中のエフェクト（点滅）
    if (pOwner_->Status().IsGuarding())
    {
        const float blinkInterval = kBlinkInterval;
        int blinkCount = static_cast<int>(Frame::Time() / blinkInterval);
        if (blinkCount % kBlinkModulo == kEvenBlink)
        {
            pOwner_->SetColor(Vector4(kColorOpaque, kColorZero, kColorZero, kColorOpaque));
        }
        else
        {
            pOwner_->SetColor(Vector4(kColorOpaque, kColorOpaque, kColorOpaque, kColorOpaque));
        }
    }
    else
    {
        pOwner_->SetColor(Vector4(kColorOpaque, kColorZero, kColorZero, kColorOpaque));
    }
}
