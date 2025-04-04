#include "SpriteCommon.h"
#include "Logger.h"

SpriteCommon* SpriteCommon::instance = nullptr;

SpriteCommon* SpriteCommon::GetInstance()
{
	if (instance == nullptr) {
		instance = new SpriteCommon();
	}
	return instance;
}

void SpriteCommon::Finalize()
{
	delete instance;
	instance = nullptr;
}

void SpriteCommon::Initialize()
{
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = DirectXCommon::GetInstance();
	psoManager_ = std::make_unique<PipeLineManager>();
	psoManager_->Initialize(dxCommon_);
	rootSignature = psoManager_->CreateSpriteRootSignature(rootSignature);
	graphicsPipelineState[0] = psoManager_->CreateSpriteGraphicsPipeLine(graphicsPipelineState[0], rootSignature, BlendMode::kNormal);
	graphicsPipelineState[1] = psoManager_->CreateSpriteGraphicsPipeLine(graphicsPipelineState[1], rootSignature, BlendMode::kAdd);
	graphicsPipelineState[2] = psoManager_->CreateSpriteGraphicsPipeLine(graphicsPipelineState[2], rootSignature, BlendMode::kSubtract);
	graphicsPipelineState[3] = psoManager_->CreateSpriteGraphicsPipeLine(graphicsPipelineState[3], rootSignature, BlendMode::kMultiply);
	graphicsPipelineState[4] = psoManager_->CreateSpriteGraphicsPipeLine(graphicsPipelineState[4], rootSignature, BlendMode::kScreen);
}

void SpriteCommon::DrawCommonSetting()
{
	psoManager_->DrawCommonSetting(graphicsPipelineState[0], rootSignature);
}

void SpriteCommon::SetBlendMode(BlendMode blendMode)
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
