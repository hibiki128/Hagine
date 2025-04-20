#include "SceneManager.h"
#include "ImGui/ImGuiManager.h"
#include <cassert>

SceneManager *SceneManager::instance = nullptr;

SceneManager *SceneManager::GetInstance() {
    if (instance == nullptr) {
        instance = new SceneManager;
    }
    return instance;
}

void SceneManager::Initialize() {
    transition_ = std::make_unique<SceneTransition>();
    transition_->Initialize();
}

void SceneManager::Finalize() {
    if (scene_) {
        firstChange = false;
        scene_->Finalize();
    }
    delete scene_;
    delete instance;
    instance = nullptr;
}

void SceneManager::Update() {

    // 次のシーンの予約があるなら
    if (nextScene_) {
        if (!firstChange) {
            transition_->SetFadeInFinish(true);
            firstChange = true;
        }
        SceneChange();
    }
    if (!transition_->IsEnd()) {
        transitionEnd = false;
        transition_->Update();
    } else {
        transitionEnd = true;
    }
    if (scene_) {
        // 実行中シーンを更新する
        scene_->Update();
    }
}

void SceneManager::Draw() {
    if (scene_) {
        scene_->Draw();
    }
}

void SceneManager::DrawForOffScreen() {
    if (scene_) {
        scene_->DrawForOffScreen();
    }
}

void SceneManager::SceneSelection(const std::string &sceneName) {
#ifdef _DEBUG
    if (!transition_->IsEnd() && transition_->FadeInStart()) {
        return; // すでに遷移中なので、次の遷移予約はしない
    }
    transition_->Reset();
    nextScene_ = sceneFactory_->CreateScene(sceneName);
    transition_->SetFadeInStart(true);
#endif // _DEBUG
}

void SceneManager::DrawTransition() {
    if (!transition_->IsEnd()) {
        transition_->Draw(); // トランジションの描画
    }
}

void SceneManager::NextSceneReservation(const std::string &sceneName) {
    // トランジション中なら処理をスキップ
    if (!transition_->IsEnd() && transition_->FadeInStart()) {
        return; // すでに遷移中なので、次の遷移予約はしない
    }
    transition_->Reset();
    assert(sceneFactory_);
    assert(nextScene_ == nullptr);

    // 次シーンを生成
    nextScene_ = sceneFactory_->CreateScene(sceneName);
    if (!firstChange) {
        transition_->SetFadeOutStart(true);
    } else {
        transition_->SetFadeInStart(true);
    }
}

void SceneManager::SceneChange() {
    if (transition_->FadeInFinish()) {
        // 旧シーンの終了
        if (scene_) {
            scene_->Finalize();
            delete scene_;
        }
        // シーンの切り替え
        scene_ = nextScene_;
        nextScene_ = nullptr;

        // シーンマネージャをセット
        scene_->SetSceneManager(this);

        // 次のシーンを初期化する
        scene_->Initialize();
        transition_->SetFadeOutStart(true);
    }
}
