#include "ModelCommon.h"

void ModelCommon::Finalize() {
}

void ModelCommon::Initialize()
{
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = DirectXCommon::GetInstance();
}
