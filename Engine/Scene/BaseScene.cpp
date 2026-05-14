#include "BaseScene.h"

void BaseScene::Initialize() {
    audio_ = Audio::GetInstance();
    input_ = Input::GetInstance();
    lightGroup_ = LightGroup::GetInstance();
    ptEditor_ = ParticleEditor::GetInstance();
    ptCSEditor_ = ParticleCSEditor::GetInstance();
    spriteManager_ = SpriteManager::GetInstance();
    objectManager_ = BaseObjectManager::GetInstance();
}

void BaseScene::Finalize() {
}

void BaseScene::Update() {
}

void BaseScene::Draw() {
}

void BaseScene::AddSceneSetting() {
}

void BaseScene::AddObjectSetting() {
}

void BaseScene::AddParticleSetting() {
}

void BaseScene::AddKeyOperationDebug() {
#ifdef _DEBUG
    ImGui::TextDisabled("このシーンにはキー操作デバッグが登録されていません。");
    ImGui::TextDisabled("GameScene に切り替えると W/A/S/D/R の入力状態と耐久値を確認できます。");
#endif // _DEBUG
}

void BaseScene::DrawForOffScreen() {
}

void BaseScene::DrawParticleEditorUI() {
#ifdef USE_IMGUI
    // CPUとGPUパーティクルをタブで分ける
    ImGui::Begin("CPUパーティクル");
    ptEditor_->ShowImGuiEditor();
    ptEditor_->DebugAll();
    ImGui::End();

    ImGui::Begin("GPUパーティクル");
    ptCSEditor_->ShowImGuiEditor();
    ptCSEditor_->DebugAll();
    ImGui::End();

#endif // USE_IMGUI
}

void BaseScene::DrawAllObjects() {
    spriteManager_->DrawAll();
    objectManager_->Draw(vp_);

    ptEditor_->DrawAll(vp_);
    ptCSEditor_->DrawAll(vp_);
}

