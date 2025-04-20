#include "ParticleCommon.h"

ParticleCommon* ParticleCommon::instance = nullptr;

ParticleCommon* ParticleCommon::GetInstance()
{
	if (instance == nullptr) {
		instance = new ParticleCommon();
	}
	return instance;
}

void ParticleCommon::Finalize()
{
	delete instance;
	instance = nullptr;
}

void ParticleCommon::Initialize(DirectXCommon* dxCommon)
{
	assert(dxCommon);
	dxCommon_ = dxCommon;
	psoManager_ = std::make_unique<PipeLineManager>();
	psoManager_->Initialize(dxCommon_);
	rootSignature = psoManager_->CreateParticleRootSignature(rootSignature);
	graphicsPipelineState[0] = psoManager_->CreateParticleGraphicsPipeLine(graphicsPipelineState[0], rootSignature, BlendMode::kNormal);
	graphicsPipelineState[1] = psoManager_->CreateParticleGraphicsPipeLine(graphicsPipelineState[1], rootSignature, BlendMode::kAdd);
	graphicsPipelineState[2] = psoManager_->CreateParticleGraphicsPipeLine(graphicsPipelineState[2], rootSignature, BlendMode::kSubtract);
	graphicsPipelineState[3] = psoManager_->CreateParticleGraphicsPipeLine(graphicsPipelineState[3], rootSignature, BlendMode::kMultiply);
	graphicsPipelineState[4] = psoManager_->CreateParticleGraphicsPipeLine(graphicsPipelineState[4], rootSignature, BlendMode::kScreen);
}

void ParticleCommon::DrawCommonSetting()
{
	psoManager_->DrawCommonSetting(graphicsPipelineState[1], rootSignature);
}

void ParticleCommon::SetBlendMode(BlendMode blendMode)
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
