#include "SceneManager.h"
#include <SpriteManager.h>
#include <cassert>

SceneManager::~SceneManager() {
    
}

void SceneManager::Initialize() {
    transition_ = SceneTransition::GetInstance();
    transition_->Initialize();
}

void SceneManager::SceneFinalize() {
    if (scene_) {
        scene_->Finalize();
        firstChange = false;
    }
}

void SceneManager::Finalize() {
    // 次シーンが残っていれば先に解放
    nextScene_.reset();
    // 現在のシーンを解放
    scene_.reset();
    // シーンファクトリーを解放
    sceneFactory_.reset();
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
        return;
    }
    transition_->Reset();
    nextScene_ = sceneFactory_->CreateScene(sceneName);
    transition_->SetFadeInStart(true);
#endif // _DEBUG
}

void SceneManager::DrawTransition() {
    if (!transition_->IsEnd()) {
        transition_->Draw();
    }
}

void SceneManager::NextSceneReservation(const std::string &sceneName) {
    if (!transition_->IsEnd() && transition_->FadeInStart()) {
        return; // すでに遷移中なので次の予約はしない
    }
    transition_->Reset();
    assert(sceneFactory_);
    assert(nextScene_ == nullptr);

    currentSceneName_ = sceneName;

    // 次シーンを生成（unique_ptr で受け取る）
    nextScene_ = sceneFactory_->CreateScene(sceneName);
    nextScene_->SetOffScreen(offscreen_);
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
            // delete 不要、reset() で解放
            scene_.reset();
            BaseObjectManager::GetInstance()->RemoveAllObjects();
            SpriteManager::GetInstance()->Clear();
#ifndef _DEBUG
            ParticleCSGroupManager::GetInstance()->ClearIndependentGroups();
#endif // _DEBUG
        }

        // 所有権を移譲（nextScene_ は自動的に nullptr になる）
        scene_ = std::move(nextScene_);

        scene_->SetSceneManager(this);
        scene_->Initialize();
        transition_->SetFadeOutStart(true);
    }
}