#include "BaseScene.h"

void BaseScene::Initialize()
{
}

void BaseScene::Finalize()
{
    BaseObjectManager::GetInstance()->DeleteObject();
}

void BaseScene::Update()
{
}

void BaseScene::Draw()
{
}

void BaseScene::AddSceneSetting() {
}

void BaseScene::AddObjectSetting() {
}

void BaseScene::AddParticleSetting() {
}

void BaseScene::DrawForOffScreen()
{
}
