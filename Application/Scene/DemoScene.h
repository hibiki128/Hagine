#pragma once
#include "BaseScene.h"

class DemoScene : public Hagine::Scene::BaseScene {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize() override;

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize() override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw() override;

    /// <summary>
    /// オフスクリーン描画処理
    /// </summary>
    void DrawForOffScreen() override;

    /// <summary>
    /// シーン設定を追加
    /// </summary>
    void AddSceneSetting() override;

    /// <summary>
    /// オブジェクト設定を追加
    /// </summary>
    void AddObjectSetting() override;

    /// <summary>
    /// パーティクル設定を追加
    /// </summary>
    void AddParticleSetting() override;

    /// <summary>
    /// ビュープロジェクションを取得
    /// </summary>
    /// <returns>ViewProjection*: ビュープロジェクションのポインタ</returns>
    Hagine::Camera::ViewProjection *GetViewProjection() override { return &vp_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// カメラを更新
    /// </summary>
    void CameraUpdate();

    /// <summary>
    /// シーン遷移を実行
    /// </summary>
    void ChangeScene();

  private:
    /// ====================================
    /// private variaus
    /// ====================================

    Hagine::Audio::Audio *audio_;
    Hagine::Input *input_;
    Hagine::Graphics::SpriteCommon *spCommon_;
    Hagine::Graphics::ParticleCommon *ptCommon_;
    Hagine::Graphics::ParticleEditor *ptEditor_;
    Hagine::Graphics::ParticleCSEditor *ptCSEditor_;

    Hagine::Camera::ViewProjection vp_;
    std::unique_ptr<Hagine::Camera::DebugCamera> debugCamera_;
};
