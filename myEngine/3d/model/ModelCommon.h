#pragma once
#include"DirectXCommon.h"
class ModelCommon
{
private:
	DirectXCommon* dxCommon_;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon"></param>
	void Initialize();

	/// <summary>
	/// getter
	/// </summary>
	/// <returns></returns>
	DirectXCommon* GetDxCommon()const { return dxCommon_; }
};

