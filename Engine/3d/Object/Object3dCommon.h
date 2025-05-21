#pragma once
#include"PipeLine/PipeLineManager.h"
class Object3dCommon 
{
  public: // メンバ関数
	/// <summary>
	///  初期化
	/// </summary>
	void  Initialize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawCommonSetting();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void skinningDrawCommonSetting();

	/// <summary>
	/// ブレンドモードの切り替え
	/// </summary>
	void SetBlendMode(BlendMode blendMode);

private:
		PipeLineManager* psoManager_ = nullptr;
};

