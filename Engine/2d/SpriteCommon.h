#pragma once
#include"DirectXCommon.h"
#include "PipeLine/PipeLineManager.h"
class SpriteCommon
{

private:
	static SpriteCommon* instance;

	SpriteCommon() = default;
	~SpriteCommon() = default;
	SpriteCommon(SpriteCommon&) = delete;
	SpriteCommon& operator=(SpriteCommon&) = delete;

private:
	DirectXCommon* dxCommon_;
	PipeLineManager* psoManager_ = nullptr;
public: // メンバ関数

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static SpriteCommon* GetInstance();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	///  初期化
	/// </summary>
	void  Initialize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawCommonSetting();

	/// <summary>
	///  getter
	/// </summary>
	/// <returns></returns>
	DirectXCommon* GetDxCommon()const { return dxCommon_; }

	/// <summary>
	/// ブレンドモードの切り替え
	/// </summary>
	void SetBlendMode(BlendMode blendMode);
};

