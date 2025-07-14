#include "BaseScene.h"

void BaseScene::Initialize()
{
}

void BaseScene::Finalize()
{
    BaseObjectManager::GetInstance()->RemoveAllObjects();
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
