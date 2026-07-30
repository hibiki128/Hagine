#define NOMINMAX
#include "FadeOut.h"
#include "scene/SceneManager.h"
#include <particle/gpu/ParticleCSSpawner.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;
void FadeOut::Initialize()
{
    // パーティクルエミッターの生成と初期設定。
    // Spawn したエミッターの更新・描画はエンジンが自動で回すので、
    // ここでは初期発生と壁サイズだけ与え、配置は Update でカメラ正面へ追従させる。
    pFadeOutEmitter_ = ParticleCSSpawner::GetInstance()->Spawn("FadeOut");
    timer_ = 0.0f;

    if (pFadeOutEmitter_)
    {
        pFadeOutEmitter_->SetAuto(true);
        // 壁のサイズを画面を覆える大きさに設定（配置は Update でカメラ正面に毎フレーム追従）
        pFadeOutEmitter_->SetScale({kWallHalfWidth, kWallHalfHeight, 1.0f});
        // 画面全体を覆うリビール演出なので、ポストエフェクト後・最前面で合成される
        // UI レイヤーに描画する（旧実装も UI 段階で描いていた。既定は "3D" ステージ）。
        pFadeOutEmitter_->SetDrawGroup("UI");
    }

    // シーントランジションの使用を有効化
    SceneManager::GetInstance()->GetSceneTransition()->SetUseTransition(true);
}

void FadeOut::Update(const ViewProjection &vp)
{
    // 経過時間を加算
    timer_ += kDeltaTime;

    if (pFadeOutEmitter_)
    {
        // パーティクル発生中は、エミッター（長方形の壁）をカメラの真正面に追従配置して
        // 画面全体を確実に覆う（ワールド固定配置だとシーンごとのカメラ位置で隙間が出る）。
        // 発生（Emit）は描画より前にエンジンが行うので、姿勢はこの Update で確定させる。
        if (timer_ <= kParticleStopTime)
        {
            Vector3 camPos = {vp.matWorld_.m[3][0], vp.matWorld_.m[3][1], vp.matWorld_.m[3][2]};
            Vector3 forward = {vp.matWorld_.m[2][0], vp.matWorld_.m[2][1], vp.matWorld_.m[2][2]};
            float len = forward.Length();
            if (len > 0.001f)
            {
                forward = forward / len;

                // 壁（XY面・法線+Z）をカメラの向きに合わせる。
                // エミッターの回転はGPU側で「渡したクォータニオンの逆回転」として
                // 発生形状に適用されるため、共役を渡して打ち消す。
                // （ヨーのみ・水平カメラでは逆回転でもズレが見えないが、
                //   スタートカメラのような見下ろしピッチでは上下に隙間が出る）
                float yaw = std::atan2(forward.x, forward.z);
                float pitch = std::asin(std::clamp(-forward.y, -1.0f, 1.0f));
                pFadeOutEmitter_->SetTranslate(camPos + forward * kCameraDistance);
                pFadeOutEmitter_->SetRotation(Quaternion::FromEulerAnglesSafe({pitch, yaw, 0.0f}).Conjugate());
            }
        }

        // 一定時間経過後はパーティクルに重力を掛けて落下させ、画面（シーン）を露出する。
        // （以前はここで全画面の白スプライトを alpha=1 で被せていたが、パーティクル遷移だけで
        //  演出するため撤去。白スプライトはランプ無しで即時表示され、画面が突然白くなって
        //  フェードアウトする違和感の原因になっていた）
        if (timer_ >= kSpriteDrawTime)
        {
            pFadeOutEmitter_->SetEnableGravity(true);
        }

        // パーティクルの自動発生を停止
        if (timer_ >= kParticleStopTime)
        {
            pFadeOutEmitter_->SetAuto(false);
        }
    }

    // フェードアウト完了判定
    if (timer_ >= kFinishTime)
    {
        isFinish_ = true;
    }
}

void FadeOut::Finalize()
{
    // 実体は ParticleCSSpawner が所有し、シーン遷移時にまとめて破棄されるため何もしない。
}

void FadeOut::DrawImGui()
{
    // デバッグ用のImGui描画
    if (pFadeOutEmitter_)
    {
        pFadeOutEmitter_->DrawImGui();
    }
}