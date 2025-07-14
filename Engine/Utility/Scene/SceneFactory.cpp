#include "SceneFactory.h"
#include"Application/Scene/TitleScene.h"
#include"Application/Scene/GameScene.h"
#include"Application/Scene/SelectScene.h"
#include"Application/Scene/ClearScene.h"
#include"Application/Scene/DemoScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
   // 次のシーンを生成
	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = new TitleScene();
	}
	else if (sceneName == "SELECT") {
		newScene = new SelectScene();
	}
	else if (sceneName == "GAME") {
		newScene = new GameScene();
	}
	else if (sceneName == "CLEAR") {
		newScene = new ClearScene();
	}
	else if (sceneName == "DEMO") {
		newScene = new DemoScene();
	}
	return newScene;
}
