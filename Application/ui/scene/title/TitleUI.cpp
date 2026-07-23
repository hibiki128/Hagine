#include "TitleUI.h"
#include "particle/gpu/ParticleCSSpawner.h"
#include "SpriteManager.h"
#include <Frame.h>
#include <Input.h>
#include <object/base/BaseObjectManager.h>
#include <particle/ParticleEditor.h>
#ifdef _DEBUG
#include "imgui.h"
#endif

using namespace Hagine;
void TitleUI::Initialize()
{

    // chargeBullet_ は別システムの CPU パーティクル（従来どおり手動で駆動する）。
    // chargeEffect_ / playerAura_ は GPU パーティクル（Spawn した実体の更新・描画はエンジンが自動で回す）。
    chargeBullet_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("chargeBullet");
    chargeEffect_ = ParticleCSSpawner::GetInstance()->Spawn("chargeEmitter");
    playerAura_ = ParticleCSSpawner::GetInstance()->Spawn("playerAura");

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

    // GPUパーティクルはエンジンが毎フレーム自動で駆動する。演出開始まで発生させたくないので、
    // ここでは発生を止めておき、開始タイミングで SetAuto(true) に切り替える
    // （旧実装は Update() を呼ぶ/呼ばないで発生を制御していたが、自動駆動では効かないため）。
    chargeEffect_->SetAuto(false);
    playerAura_->SetAuto(false);

    // 弾を振り下ろすキャラ（cube_2）。ベースモデルの Charge はループ再生されるので、
    // 振り下ろし用の ChargeShot をループ無しで追加登録しておく（発火は RequestStartCinematic）。
    pChargeChara_ = BaseObjectManager::GetInstance()->GetObjectByName("cube_2");
    if (pChargeChara_)
    {
        pChargeChara_->SetAnimationBlendDuration(0.1f);
        pChargeChara_->AddAnimation(kChargeShotAnim, false);
    }

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

void TitleUI::Update()
{
    gamePad_->Update();
    time_ += kDeltaTime;

    // タイマーがkMaxTime以上でスプライト表示開始
    if (time_ >= kMaxTime_ && !isSpriteVisible_)
    {
        isSpriteVisible_ = true;
        spriteEaseTimer_ = 0.0f;
    }

    // スプライトの移動処理
    if (isSpriteVisible_ && spriteEaseTimer_ < spriteEaseDuration_)
    {
        spriteEaseTimer_ += kDeltaTime;

        // イージングで位置を計算
        Vector2 titleLogoPos = ApplyEasing(EasingType::OutCubic, titleLogoStartPos_, titleLogoEndPos_, spriteEaseTimer_, spriteEaseDuration_);
        Vector2 pressStartPos = ApplyEasing(EasingType::OutCubic, pressStartStartPos_, pressStartEndPos_, spriteEaseTimer_, spriteEaseDuration_);

        // 位置を適用
        sprites_[kTitleLogo]->sprite->SetPosition(titleLogoPos);
        sprites_[kPressStart]->sprite->SetPosition(pressStartPos);
    }

    if (time_ >= kMaxTime_)
    {
        // チャージスケールの更新
        if (!isMaxChargeScale_)
        {
            chargeScale_ += chargeScaleSpeed_ * kDeltaTime / kParticleScaleSpeedDivisor;
            if (chargeScale_ >= maxChargeScale_)
            {
                chargeScale_ = maxChargeScale_;
                isMaxChargeScale_ = true;
            }
        }

        // パーティクルのスケール設定
        {
            float aroundEndScale = (kParticleScaleBase + chargeScale_) * kParticleScaleMultiplier;
            float aroundStartScale = aroundEndScale - kParticleScaleOffset;
            float bulletScale = aroundEndScale + kBulletScaleOffset;
            chargeBullet_->SetStartScale("chargeAround", {aroundStartScale, aroundStartScale, aroundStartScale});
            chargeBullet_->SetEndScale("chargeAround", {aroundEndScale, aroundEndScale, aroundEndScale});
            chargeBullet_->SetStartScale("chargeBullet", {bulletScale, bulletScale, bulletScale});
        }

        chargeBullet_->Update();
    }

    if (time_ >= kMaxTime_ - kChargeEffectStartTime)
    {
        if (!secondMove_)
        {
            // 演出開始タイミングで発生を有効化する（更新・描画はエンジンが自動で回す）
            chargeEffect_->SetAuto(true);
            playerAura_->SetAuto(true);
        }
    }

    // 開始演出の発火はシーン側（メニューでチュートリアル選択時）から RequestStartCinematic() で行う

    if (secondMove_)
    {
        bulletEaseTimer_ += kDeltaTime;

        // スプライト横出し処理
        if (isSpriteExiting_ && spriteExitTimer_ < spriteEaseDuration_)
        {
            spriteExitTimer_ += kDeltaTime;

            // 逆方向にイージング(開始位置に戻す)
            Vector2 titleLogoPos = ApplyEasing(EasingType::InCubic, titleLogoEndPos_, titleLogoStartPos_, spriteExitTimer_, spriteEaseDuration_);
            Vector2 pressStartPos = ApplyEasing(EasingType::InCubic, pressStartEndPos_, pressStartStartPos_, spriteExitTimer_, spriteEaseDuration_);

            sprites_[kTitleLogo]->sprite->SetPosition(titleLogoPos);
            sprites_[kPressStart]->sprite->SetPosition(pressStartPos);

            if (spriteExitTimer_ >= spriteEaseDuration_)
            {
                isSpriteExiting_ = false;
            }
        }
    }

    chargeBullet_->SetPosition(ApplyEasing(EasingType::InSine, chargeBullet_->GetPosition(), targetPos_, bulletEaseTimer_, kBulletEaseDuration));
    if (chargeBullet_->GetPosition() == targetPos_)
    {
        timer_ += kDeltaTime;
    }
    if (timer_ >= kFinishDelayTime)
    {
        isFinish_ = true;
    }
}

void TitleUI::RequestStartCinematic()
{
    if (secondMove_)
    {
        return;
    }
    secondMove_ = true;
    isSpriteExiting_ = true;
    spriteExitTimer_ = 0.0f;

    // 弾が動き出す瞬間に、振り下ろしモーション（ChargeShot）を一度だけ再生する
    if (pChargeChara_)
    {
        pChargeChara_->SetAnima(kChargeShotAnim);
    }
}

void TitleUI::HidePressStart()
{
    if (sprites_[kPressStart] && sprites_[kPressStart]->sprite)
    {
        sprites_[kPressStart]->sprite->SetAlpha(0.0f);
    }
}

void TitleUI::Draw(ViewProjection &vp_)
{
    // chargeEffect_ / playerAura_（GPUパーティクル）はエンジンが自動で描画する。
    // ここでは別システムの CPU パーティクル（chargeBullet_）だけ描画する。
    chargeBullet_->Draw(vp_);
    cameraMove_ = vp_.GetIsCameraMove();
}

#ifdef _DEBUG
void TitleUI::DrawImGui()
{
    ImGui::Begin("TitleUI Debug");

    ImGui::SeparatorText("Timer");
    ImGui::Text("time_           : %.3f / %.3f (kMaxTime)", time_, kMaxTime_);
    ImGui::Text("timer_          : %.3f / %.3f (kFinishDelayTime)", timer_, kFinishDelayTime);
    ImGui::Text("bulletEaseTimer_: %.3f / %.3f (kBulletEaseDuration)", bulletEaseTimer_, kBulletEaseDuration);

    ImGui::SeparatorText("ChargeScale");
    // chargeScale_ のプログレスバー表示
    float progress = (maxChargeScale_ > 0.0f) ? (chargeScale_ / maxChargeScale_) : 0.0f;
    char overlay[64];
    snprintf(overlay, sizeof(overlay), "%.3f / %.3f", chargeScale_, maxChargeScale_);
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), overlay);
    ImGui::Text("isMaxChargeScale_: %s", isMaxChargeScale_ ? "TRUE" : "false");

    ImGui::SeparatorText("Particle Scale (computed)");
    float aroundEndScale = (kParticleScaleBase + chargeScale_) * kParticleScaleMultiplier;
    float aroundStartScale = aroundEndScale - kParticleScaleOffset;
    float bulletScale = aroundEndScale + kBulletScaleOffset;
    ImGui::Text("chargeAround StartScale : %.3f", aroundStartScale);
    ImGui::Text("chargeAround EndScale   : %.3f", aroundEndScale);
    ImGui::Text("chargeBullet StartScale : %.3f", bulletScale);

    ImGui::SeparatorText("Sprite");
    ImGui::Text("isSpriteVisible_  : %s", isSpriteVisible_ ? "true" : "false");
    ImGui::Text("spriteEaseTimer_  : %.3f", spriteEaseTimer_);
    ImGui::Text("isSpriteExiting_  : %s", isSpriteExiting_ ? "true" : "false");
    ImGui::Text("spriteExitTimer_  : %.3f", spriteExitTimer_);

    ImGui::SeparatorText("State");
    ImGui::Text("secondMove_  : %s", secondMove_ ? "true" : "false");
    ImGui::Text("cameraMove_  : %s", cameraMove_ ? "true" : "false");
    ImGui::Text("isFinish_    : %s", isFinish_ ? "true" : "false");

    Vector3 pos = chargeBullet_->GetPosition();
    ImGui::SeparatorText("chargeBullet Position");
    ImGui::Text("pos : (%.3f, %.3f, %.3f)", pos.x, pos.y, pos.z);
    ImGui::Text("target : (%.3f, %.3f, %.3f)", targetPos_.x, targetPos_.y, targetPos_.z);

    ImGui::End();
}
#endif