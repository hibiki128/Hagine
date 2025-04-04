#pragma once
#include"ViewProjection.h"
class SceneManager;
class BaseScene
{
public:

	virtual ~BaseScene() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize();

	/// <summary>
	/// 終了
	/// </summary>
	virtual void Finalize();

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void DrawForOffScreen();

	virtual void SetSceneManager(SceneManager* sceneManager) { sceneManager_ = sceneManager; }

	virtual ViewProjection* GetViewProjection() = 0; 

protected:
	// シーンマネージャ
	SceneManager* sceneManager_ = nullptr;
};

