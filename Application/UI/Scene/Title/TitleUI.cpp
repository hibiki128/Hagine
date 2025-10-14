#include "TitleUI.h"
#include "Particle/CSParticle/ParticleCSEditor.h"
#include "SpriteManager.h"
#include <Frame.h>
#include <Input.h>
#include <Object/Base/BaseObjectManager.h>
#include <Particle/ParticleEditor.h>

void TitleUI::Initialize() {

    chargeBullet_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chageBullet");
    chargeEffect_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chageEmitter");
    playerAura_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("playerAura");

    chargeBullet_->SetPosition(
        {BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().x,
         BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().y + 6.5f,
         BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().z});
    chargeEffect_->SetPosition(
        {BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().x,
         BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().y + 6.5f,
         BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().z});

    targetPos_ = {-4.5f, -3.1f, 5.7f};

    playerAura_->SetTranslate(BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition());
    playerAura_->SetRotation(BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalRotation());

    chargeScale_ = 0.0f;
    isMaxChargeScale_ = false;

    SpriteManager::GetInstance()->SetSaveFolder("TitleScene");
    SpriteManager::GetInstance()->LoadAllSprites();

    sprites_[kTitleLogo] = SpriteManager::GetInstance()->GetSprite("titleLogo");
    sprites_[kPressStart] = SpriteManager::GetInstance()->GetSprite("startButton");

    // 元の位置を保存
    titleLogoEndPos_ = sprites_[kTitleLogo]->sprite->GetPosition();
    pressStartEndPos_ = sprites_[kPressStart]->sprite->GetPosition();

    // 開始位置のY座標を元の位置に合わせる
    titleLogoStartPos_.y = titleLogoEndPos_.y;
    pressStartStartPos_.y = pressStartEndPos_.y;

    // 初期位置を画面外に設定
    sprites_[kTitleLogo]->sprite->SetPosition(titleLogoStartPos_);
    sprites_[kPressStart]->sprite->SetPosition(pressStartStartPos_);

    isSpriteVisible_ = false;
    spriteEaseTimer_ = 0.0f;
    secondMove_ = false;
    isSpriteExiting_ = false;
    spriteExitTimer_ = 0.0f;
    isFinish_ = false;
}

void TitleUI::Update() {
    time_ += 1.0f / 60.0f;

    // タイマーがkMaxTime以上でスプライト表示開始
    if (time_ >= kMaxTime_ && !isSpriteVisible_) {
        isSpriteVisible_ = true;
        spriteEaseTimer_ = 0.0f;
    }

    // スプライトの移動処理
    if (isSpriteVisible_ && spriteEaseTimer_ < spriteEaseDuration_) {
        spriteEaseTimer_ += 1.0f / 60.0f;

        // イージングで位置を計算
        Vector2 titleLogoPos = ApplyEasing(EasingType::OutCubic, titleLogoStartPos_, titleLogoEndPos_, spriteEaseTimer_, spriteEaseDuration_);
        Vector2 pressStartPos = ApplyEasing(EasingType::OutCubic, pressStartStartPos_, pressStartEndPos_, spriteEaseTimer_, spriteEaseDuration_);

        // 位置を適用
        sprites_[kTitleLogo]->sprite->SetPosition(titleLogoPos);
        sprites_[kPressStart]->sprite->SetPosition(pressStartPos);
    }

    if (time_ >= kMaxTime_) {
        // スケール拡大処理
        if (!isMaxChargeScale_) {
            chargeScale_ += chargeScaleSpeed_ * Frame::DeltaTime() / 1.25f;
            if (chargeScale_ >= maxChargeScale_) {
                chargeScale_ = maxChargeScale_;
                isMaxChargeScale_ = true;
            }
        }
        // パーティクルのスケール設定
        if (!isMaxChargeScale_) {
            // chageAroundのエンドスケール
            float aroundEndScale = (0.8f + chargeScale_) * 1.4f;
            float aroundStartScale = aroundEndScale - 0.3f;
            float bulletScale = aroundEndScale + 1.0f;
            chargeBullet_->SetStartScale("chageAround", {aroundStartScale, aroundStartScale, aroundStartScale});
            chargeBullet_->SetEndScale("chageAround", {aroundEndScale, aroundEndScale, aroundEndScale});
            chargeBullet_->SetStartScale("chageBullet", {bulletScale, bulletScale, bulletScale});
        }
        chargeBullet_->Update();
    }
    if (time_ >= kMaxTime_ - 0.5f) {
        if (!secondMove_) {
            chargeEffect_->Update();
            playerAura_->Update();
        }
    }

    if (time_ >= 2.5f && Input::GetInstance()->TriggerKey(DIK_SPACE) && !secondMove_) {
        secondMove_ = true;
        isSpriteExiting_ = true;
        spriteExitTimer_ = 0.0f;
    }

    if (secondMove_) {
        bulletEaseTimer_ += 1.0f / 60.0f;

        // スプライト横出し処理
        if (isSpriteExiting_ && spriteExitTimer_ < spriteEaseDuration_) {
            spriteExitTimer_ += 1.0f / 60.0f;

            // 逆方向にイージング（開始位置に戻す）
            Vector2 titleLogoPos = ApplyEasing(EasingType::InCubic, titleLogoEndPos_, titleLogoStartPos_, spriteExitTimer_, spriteEaseDuration_);
            Vector2 pressStartPos = ApplyEasing(EasingType::InCubic, pressStartEndPos_, pressStartStartPos_, spriteExitTimer_, spriteEaseDuration_);

            sprites_[kTitleLogo]->sprite->SetPosition(titleLogoPos);
            sprites_[kPressStart]->sprite->SetPosition(pressStartPos);

            if (spriteExitTimer_ >= spriteEaseDuration_) {
                isSpriteExiting_ = false;
            }
        }
    }

    chargeBullet_->SetPosition(ApplyEasing(EasingType::InSine, chargeBullet_->GetPosition(), targetPos_, bulletEaseTimer_, 7.0f));
    if (chargeBullet_->GetPosition() == targetPos_) {
        timer_ += 1.0f / 60.0f;
    }
    if (timer_ >= 0.3f) {
        isFinish_ = true;
    }
}

void TitleUI::Draw(const ViewProjection &vp_) {
    chargeBullet_->Draw(vp_);
    chargeEffect_->Draw(vp_);
    playerAura_->Draw(vp_);
}