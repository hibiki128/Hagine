#define NOMINMAX
#include "FadeOut.h"
#include "scene/SceneManager.h"
#include "SpriteManager.h"
#include <particle/gpu/ParticleCSEditor.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;
void FadeOut::Initialize()
{
    // スプライトマネージャーの設定と読み込み
    SpriteManager::GetInstance()->SetSaveFolder("Transition");
    SpriteManager::GetInstance()->LoadAllSprites();

    // パーティクルエミッターの生成と初期設定
    fadeOut_ = ParticleCSEditor::GetInstance()->CreateEmitterFromTemplate("FadeOut");
    fadeOut_->SetAuto(true);
    timer_ = 0.0f;

    // 壁のサイズを画面を覆える大きさに設定（配置は Draw でカメラ正面に毎フレーム追従）
    fadeOut_->SetScale({kWallHalfWidth, kWallHalfHeight, 1.0f});

    // シーントランジションの使用を有効化
    SceneManager::GetInstance()->GetSceneTransition()->SetUseTransition(true);
}

void FadeOut::Update()
{
    // パーティクルの更新
    fadeOut_->Update();
    // 経過時間を加算
    timer_ += kDeltaTime;
}

void FadeOut::Draw(const ViewProjection &vp)
{
    // パーティクル発生中は、エミッター（長方形の壁）をカメラの真正面に追従配置して
    // 画面全体を確実に覆う（ワールド固定配置だとシーンごとのカメラ位置で隙間が出る）
    if (timer_ <= kParticleStopTime)
    {
        Vector3 camPos = {vp.matWorld_.m[3][0], vp.matWorld_.m[3][1], vp.matWorld_.m[3][2]};
        Vector3 forward = {vp.matWorld_.m[2][0], vp.matWorld_.m[2][1], vp.matWorld_.m[2][2]};
        float len = forward.Length();
        if (len > 0.001f)
        {
            forward = forward / len;

            // 壁（XY面・法線+Z）をカメラの向きに合わせる（ヨー→ピッチの順で合成）
            float yaw = std::atan2(forward.x, forward.z);
            float pitch = std::asin(std::clamp(-forward.y, -1.0f, 1.0f));
            fadeOut_->SetTranslate(camPos + forward * kCameraDistance);
            fadeOut_->SetRotation(Quaternion::FromEulerAnglesSafe({pitch, yaw, 0.0f}));
        }
    }

    if (SpriteManager::GetInstance()->GetSprite("transition"))
    {
        // 一定時間内はスプライトを表示
        if (timer_ <= kSpriteDrawTime)
        {
            SpriteManager::GetInstance()->GetSprite("transition")->sprite->SetAlpha(1.0f);
        }
        else
        {
            // 時間経過後はスプライトを非表示にし、パーティクルの重力を有効化
            SpriteManager::GetInstance()->GetSprite("transition")->sprite->SetAlpha(0.0f);
            fadeOut_->SetEnableGravity(true);
        }
    }

    // パーティクルの自動発生を停止
    if (timer_ >= kParticleStopTime)
    {
        fadeOut_->SetAuto(false);
    }

    // パーティクルの描画
    fadeOut_->Draw(vp);

    // フェードアウト完了判定
    if (timer_ >= kFinishTime)
    {
        isFinish_ = true;
    }
}

void FadeOut::Finalize()
{
    // エミッター名のカウンターをクリア
    ParticleCSEmitter::ClearNameCounter("FadeOut");
}

void FadeOut::ImGui()
{
    // デバッグ用のImGui描画
    fadeOut_->DrawImGui();
}