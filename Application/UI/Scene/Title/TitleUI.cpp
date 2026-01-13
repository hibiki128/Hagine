#include "TitleUI.h"
#include "Particle/CSParticle/ParticleCSEditor.h"
#include "SpriteManager.h"
#include <Frame.h>
#include <Input.h>
#include <Object/Base/BaseObjectManager.h>
#include <Particle/ParticleEditor.h>

void TitleUI::Initialize() {

    chargeBullet_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chageBullet");
    chargeEffect_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("chargeEmitter");
    playerAura_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("playerAura");

    chargeBullet_->SetPosition(
        {BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().x,
         BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().y + kPlayerPositionOffsetY,
         BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().z});
    chargeEffect_->SetTranslate(
        {BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().x,
         BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().y + kPlayerPositionOffsetY,
         BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition().z});

    targetPos_ = {kTargetPosX, kTargetPosY, kTargetPosZ};

    playerAura_->SetTranslate(BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalPosition());
    playerAura_->SetRotation(BaseObjectManager::GetInstance()->GetObjectByName("cube_2")->GetLocalRotation());

    chargeScale_ = kInitialChargeScale;
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
    cameraMove_ = false;

    gamePad_ = std::make_unique<GamePad>();
    gamePad_->Init(0);
}

void TitleUI::Update() {
    gamePad_->Update();
    time_ += kDeltaTime;

    // タイマーがkMaxTime以上でスプライト表示開始
    if (time_ >= kMaxTime_ && !isSpriteVisible_) {
        isSpriteVisible_ = true;
        spriteEaseTimer_ = 0.0f;
    }

    // スプライトの移動処理
    if (isSpriteVisible_ && spriteEaseTimer_ < spriteEaseDuration_) {
        spriteEaseTimer_ += kDeltaTime;

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
            chargeScale_ += chargeScaleSpeed_ * Frame::DeltaTime() / kParticleScaleSpeedDivisor;
            if (chargeScale_ >= maxChargeScale_) {
                chargeScale_ = maxChargeScale_;
                isMaxChargeScale_ = true;
            }
        }
        // パーティクルのスケール設定
        if (!isMaxChargeScale_) {
            // chageAroundのエンドスケール
            float aroundEndScale = (kParticleScaleBase + chargeScale_) * kParticleScaleMultiplier;
            float aroundStartScale = aroundEndScale - kParticleScaleOffset;
            float bulletScale = aroundEndScale + kBulletScaleOffset;
            chargeBullet_->SetStartScale("chageAround", {aroundStartScale, aroundStartScale, aroundStartScale});
            chargeBullet_->SetEndScale("chageAround", {aroundEndScale, aroundEndScale, aroundEndScale});
            chargeBullet_->SetStartScale("chageBullet", {bulletScale, bulletScale, bulletScale});
        }
        chargeBullet_->Update();
    }
    if (time_ >= kMaxTime_ - kChargeEffectStartTime) {
        if (!secondMove_) {
            chargeEffect_->Update();
            playerAura_->Update();
        }
    }
    if (!gamePad_->IsConnected()) {
        if (time_ >= kMinStartInputTime && Input::GetInstance()->TriggerKey(DIK_SPACE) && !secondMove_ && !cameraMove_) {
            secondMove_ = true;
            isSpriteExiting_ = true;
            spriteExitTimer_ = 0.0f;
        }
    } else {
        if (time_ >= kMinStartInputTime && gamePad_->IsTrigger(XINPUT_GAMEPAD_A) && !secondMove_ && !cameraMove_) {
            secondMove_ = true;
            isSpriteExiting_ = true;
            spriteExitTimer_ = 0.0f;
        }
    }

    if (secondMove_) {
        bulletEaseTimer_ += kDeltaTime;

        // スプライト横出し処理
        if (isSpriteExiting_ && spriteExitTimer_ < spriteEaseDuration_) {
            spriteExitTimer_ += kDeltaTime;

            // 逆方向にイージング(開始位置に戻す)
            Vector2 titleLogoPos = ApplyEasing(EasingType::InCubic, titleLogoEndPos_, titleLogoStartPos_, spriteExitTimer_, spriteEaseDuration_);
            Vector2 pressStartPos = ApplyEasing(EasingType::InCubic, pressStartEndPos_, pressStartStartPos_, spriteExitTimer_, spriteEaseDuration_);

            sprites_[kTitleLogo]->sprite->SetPosition(titleLogoPos);
            sprites_[kPressStart]->sprite->SetPosition(pressStartPos);

            if (spriteExitTimer_ >= spriteEaseDuration_) {
                isSpriteExiting_ = false;
            }
        }
    }

    chargeBullet_->SetPosition(ApplyEasing(EasingType::InSine, chargeBullet_->GetPosition(), targetPos_, bulletEaseTimer_, kBulletEaseDuration));
    if (chargeBullet_->GetPosition() == targetPos_) {
        timer_ += kDeltaTime;
    }
    if (timer_ >= kFinishDelayTime) {
        isFinish_ = true;
    }
}

void TitleUI::Draw(ViewProjection &vp_) {
    chargeBullet_->Draw(vp_);
    chargeEffect_->Draw(vp_);
    playerAura_->Draw(vp_);
    cameraMove_ = vp_.GetIsCameraMove();
}