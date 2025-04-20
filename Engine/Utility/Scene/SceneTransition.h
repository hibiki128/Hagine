#pragma once
#include"Sprite.h"
#include"memory"
#include"vector"
class SceneTransition
{
public:
	SceneTransition();
	~SceneTransition();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	void Debug();

	/// <summary>
	/// セット
	/// </summary>
	/// <param name="start"></param>
	void SetFadeInStart(bool start) { fadeInStart = start; }
	void SetFadeOutStart(bool start) { fadeOutStart = start; }
	void SetFadeInFinish(bool finish) { fadeInFinish = finish; }

	/// <summary>
	/// getter
	/// </summary>
	/// <returns></returns>
	bool IsEnd() { return isEnd; }
	bool FadeInFinish() { return fadeInFinish; }
	bool FadeInStart() { return fadeInStart; }

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

private:

	/// <summary>
	/// フェードアップデート
	/// </summary>
	void FadeUpdate();

	/// <summary>
	/// フェードイン
	/// </summary>
	void FadeIn();

	/// <summary>
	/// フェードアウト
	/// </summary>
	void FadeOut();
	
	/// <summary>
	/// デフォルトフェードイン
	/// </summary>
	void DefaultFadeIn();
	
	/// <summary>
	/// デフォルトフェードアウト
	/// </summary>
	void DefaultFadeOut();

	void ReverseFadeIn();

	void ReverseFadeOut();

private:
	// フェードの持続時間
	float duration_ = 0.0f;
	// 経過時間カウンター
	float counter_ = 0.0f;

	std::unique_ptr<Sprite> sprite_ = nullptr;
	std::vector<std::vector<std::unique_ptr<Sprite>>> transition_;

	Vector2 spPos_ = { 0.0f,0.0f };

	bool fadeInStart = false;
	bool fadeOutStart = false;
	bool fadeInFinish = false;
	bool fadeOutFinish = false;
	bool isEnd = false;

};

