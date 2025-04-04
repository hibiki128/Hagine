#include "Object3dCommon.h"
#include "Logger.h"

Object3dCommon* Object3dCommon::instance = nullptr;

Object3dCommon* Object3dCommon::GetInstance()
{
	if (instance == nullptr) {
		instance = new Object3dCommon();
	}
	return instance;
}

void Object3dCommon::Finalize()
{
	delete instance;
	instance = nullptr;
}

void Object3dCommon::Initialize()
{
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = DirectXCommon::GetInstance();
	psoManager_ = std::make_unique<PipeLineManager>();
	psoManager_->Initialize(dxCommon_);
	rootSignature = psoManager_->CreateRootSignature(rootSignature);
	graphicsPipelineState[0] = psoManager_->CreateGraphicsPipeLine(graphicsPipelineState[0], rootSignature, BlendMode::kNormal);
	graphicsPipelineState[1] = psoManager_->CreateGraphicsPipeLine(graphicsPipelineState[1], rootSignature, BlendMode::kAdd);
	graphicsPipelineState[2] = psoManager_->CreateGraphicsPipeLine(graphicsPipelineState[2], rootSignature, BlendMode::kSubtract);
	graphicsPipelineState[3] = psoManager_->CreateGraphicsPipeLine(graphicsPipelineState[3], rootSignature, BlendMode::kMultiply);
	graphicsPipelineState[4] = psoManager_->CreateGraphicsPipeLine(graphicsPipelineState[4], rootSignature, BlendMode::kScreen);

	skinningRootSignature = psoManager_->CreateSkinningRootSignature(skinningRootSignature);
	skinningGraphicsPipelineState = psoManager_->CreateSkinningGraphicsPipeLine(skinningGraphicsPipelineState, skinningRootSignature);
}

void Object3dCommon::DrawCommonSetting()
{
	psoManager_->DrawCommonSetting(graphicsPipelineState[0], rootSignature);
}

void Object3dCommon::skinningDrawCommonSetting()
{
	psoManager_->DrawCommonSetting(skinningGraphicsPipelineState, skinningRootSignature);
}

void Object3dCommon::SetBlendMode(BlendMode blendMode)
{
	switch (blendMode) {
	case BlendMode::kNormal:
		psoManager_->DrawCommonSetting(graphicsPipelineState[0], rootSignature);
		break;
	case BlendMode::kAdd:
		psoManager_->DrawCommonSetting(graphicsPipelineState[1], rootSignature);
		break;
	case BlendMode::kSubtract:
		psoManager_->DrawCommonSetting(graphicsPipelineState[2], rootSignature);
		break;
	case BlendMode::kMultiply:
		psoManager_->DrawCommonSetting(graphicsPipelineState[3], rootSignature);
		break;
	case BlendMode::kScreen:
		psoManager_->DrawCommonSetting(graphicsPipelineState[4], rootSignature);
		break;
	default:
		break;
	}
}
