#include "OffScreen.h"
#include"DirectXCommon.h"
#ifdef _DEBUG
#include"imgui.h"
#endif // _DEBUG
#include <fstream>
#include <filesystem>
#include <iostream>
void OffScreen::Initialize()
{
	dxCommon = DirectXCommon::GetInstance();
	psoManager_ = std::make_unique<PipeLineManager>();
	psoManager_->Initialize(dxCommon);
	rootSignature[0] = psoManager_->CreateRenderRootSignature(rootSignature[0], ShaderMode::kNone);
	rootSignature[1] = psoManager_->CreateRenderRootSignature(rootSignature[1], ShaderMode::kVigneet);
	rootSignature[2] = psoManager_->CreateRenderRootSignature(rootSignature[2], ShaderMode::kSmooth);
	rootSignature[3] = psoManager_->CreateRenderRootSignature(rootSignature[3], ShaderMode::kGauss);
	rootSignature[4] = psoManager_->CreateRenderRootSignature(rootSignature[4], ShaderMode::kDepth);
	rootSignature[5] = psoManager_->CreateRenderRootSignature(rootSignature[5], ShaderMode::kBlur);
	rootSignature[6] = psoManager_->CreateRenderRootSignature(rootSignature[6], ShaderMode::kCinematic);
	graphicsPipelineState[0] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[0], rootSignature[0], ShaderMode::kNone);
	graphicsPipelineState[1] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[1], rootSignature[0], ShaderMode::kGray);
	graphicsPipelineState[2] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[2], rootSignature[1], ShaderMode::kVigneet);
	graphicsPipelineState[3] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[3], rootSignature[2], ShaderMode::kSmooth);
	graphicsPipelineState[4] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[4], rootSignature[3], ShaderMode::kGauss);
	graphicsPipelineState[5] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[5], rootSignature[0], ShaderMode::kOutLine);
	graphicsPipelineState[6] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[6], rootSignature[4], ShaderMode::kDepth);
	graphicsPipelineState[7] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[7], rootSignature[5], ShaderMode::kBlur);
	graphicsPipelineState[8] = psoManager_->CreateRenderGraphicsPipeLine(graphicsPipelineState[8], rootSignature[6], ShaderMode::kCinematic);
	srvManager_ = SrvManager::GetInstance();
	CreateSmooth();
	CreateGauss();
	CreateVignette();
	CreateDepth();
	CreateRadial();
	CreateCinematic();
	LoadFromJson();
	LoadFromJson(shaderMode_);
}

void OffScreen::Draw()
{
	// 選択されたShaderModeに基づいて描画設定を実行
	switch (shaderMode_)
	{
	case ShaderMode::kNone:
		psoManager_->DrawCommonSetting(graphicsPipelineState[0], rootSignature[0]);
		break;
	case ShaderMode::kGray:
		psoManager_->DrawCommonSetting(graphicsPipelineState[1], rootSignature[0]);
		break;
	case ShaderMode::kVigneet:
		psoManager_->DrawCommonSetting(graphicsPipelineState[2], rootSignature[1]);
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, vignetteResource->GetGPUVirtualAddress());
		break;
	case ShaderMode::kSmooth:
		psoManager_->DrawCommonSetting(graphicsPipelineState[3], rootSignature[2]);
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, smoothResource->GetGPUVirtualAddress());
		break;
	case ShaderMode::kGauss:
		psoManager_->DrawCommonSetting(graphicsPipelineState[4], rootSignature[3]);
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, gaussianResouce->GetGPUVirtualAddress());
		break;
	case ShaderMode::kOutLine:
		psoManager_->DrawCommonSetting(graphicsPipelineState[5], rootSignature[0]);
		break;
	case ShaderMode::kDepth:
		psoManager_->DrawCommonSetting(graphicsPipelineState[6], rootSignature[4]);
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, depthResouce->GetGPUVirtualAddress());
		dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, dxCommon->GetDepthGPUHandle());
		break;
	case ShaderMode::kBlur:
		psoManager_->DrawCommonSetting(graphicsPipelineState[7], rootSignature[5]);
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, radialResource->GetGPUVirtualAddress());
		break;
	case ShaderMode::kCinematic:
		psoManager_->DrawCommonSetting(graphicsPipelineState[8], rootSignature[6]);
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, cinematicResource->GetGPUVirtualAddress());
	default:
		break;
	}
	dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon->GetOffScreenGPUHandle());
	dxCommon->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void OffScreen::DrawCommonSetting()
{
#ifdef _DEBUG
	ImGui::Begin("Offscreen");

	// ShaderModeを文字列で表現
	const char* shaderModeItems[] = { "None", "Gray", "Vignett", "Smooth", "Gauss", "OutLine","Depth","Blur","Cinematic" };
	int currentShaderMode = static_cast<int>(shaderMode_);

	// Comboを描画してユーザーが選択した場合に値を更新
	if (ImGui::Combo("Shader Mode", &currentShaderMode, shaderModeItems, IM_ARRAYSIZE(shaderModeItems)))
	{
		shaderMode_ = static_cast<ShaderMode>(currentShaderMode);
		LoadFromJson(shaderMode_);
	}

	switch (shaderMode_)
	{
	case ShaderMode::kNone:
		break;
	case ShaderMode::kGray:
		break;
	case ShaderMode::kVigneet:
		ImGui::DragFloat("Exponent", &vignetteData->vignetteExponent, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Radius", &vignetteData->vignetteRadius, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Strength", &vignetteData->vignetteStrength, 0.01f);
		ImGui::DragFloat2("Center", &vignetteData->vignetteCenter.x, 0.01f, -10.0f, 10.0f);
		break;
	case ShaderMode::kSmooth:
		ImGui::DragInt("Kernel Size", &smoothData->kernelSize, 2, 3, 7);
		break;
	case ShaderMode::kGauss:
		ImGui::DragInt("Kernel Size", &gaussianData->kernelSize, 2, 3, 7);
		ImGui::DragFloat("sigma", &gaussianData->sigma, 0.01f, 0.01f, 10.0f);
		break;
	case ShaderMode::kOutLine:
		break;
	case ShaderMode::kDepth:
		depthData->projectionInverse = Inverse(projectionInverse_);
		ImGui::DragInt("Kernel Size", &depthData->kernelSize, 2, 3, 7);
		break;
	case ShaderMode::kBlur:
		ImGui::DragFloat2("中心座標", &radialData->kCenter.x, 0.1f);
		ImGui::DragFloat("幅", &radialData->kBlurWidth, 0.01f);
		break;
	case ShaderMode::kCinematic:
		ImGui::DragFloat("コンストラクト", &cinematicData->contrast, 0.01f);
		ImGui::DragFloat("彩度", &cinematicData->saturation, 0.01f);
		ImGui::DragFloat("輝度", &cinematicData->brightness, 0.01f);
		break;
	default:
		break;
	}
	if (ImGui::Button("セーブ")) {
		SaveToJson();
		std::string message = std::format("OffScreen saved.");
		MessageBoxA(nullptr, message.c_str(), "OffScreen", 0);
	}
	ImGui::End();
#endif // _DEBUG
}

void OffScreen::CreateSmooth()
{
	smoothResource = dxCommon->CreateBufferResource(sizeof(KernelSettings));
	smoothResource->Map(0, nullptr, reinterpret_cast<void**>(&smoothData));
	smoothData->kernelSize = 3;
}

void OffScreen::CreateGauss()
{
	gaussianResouce = dxCommon->CreateBufferResource(sizeof(GaussianParams));
	gaussianResouce->Map(0, nullptr, reinterpret_cast<void**>(&gaussianData));
	gaussianData->kernelSize = 3;
	gaussianData->sigma = 1;
}

void OffScreen::CreateVignette()
{
	vignetteResource = dxCommon->CreateBufferResource(sizeof(VignetteParameter));
	vignetteResource->Map(0, nullptr, reinterpret_cast<void**>(&vignetteData));
	vignetteData->vignetteExponent = 1.0f;
	vignetteData->vignetteRadius = 1.0f;
	vignetteData->vignetteStrength = 1.0f;
	vignetteData->vignetteCenter = { 0.5f,0.5f };
}

void OffScreen::CreateDepth()
{
	depthResouce = dxCommon->CreateBufferResource(sizeof(Depth));
	depthResouce->Map(0, nullptr, reinterpret_cast<void**>(&depthData));
	depthData->projectionInverse = MakeIdentity4x4();
	depthData->kernelSize = 3;
}

void OffScreen::CreateRadial()
{
	radialResource = dxCommon->CreateBufferResource(sizeof(RadialBlur));
	radialResource->Map(0, nullptr, reinterpret_cast<void**>(&radialData));
	radialData->kBlurWidth = 0.01f;   // 発光のしきい値（低いほど多くの部分が発光）
	radialData->kCenter = { 0.5f,0.5f };
}

void OffScreen::CreateCinematic()
{
	cinematicResource = dxCommon->CreateBufferResource(sizeof(Cinematic));
	cinematicResource->Map(0, nullptr, reinterpret_cast<void**>(&cinematicData));
	cinematicData->iResolution = { 1280.0f,720.0f };
	cinematicData->contrast = 1.05f;
	cinematicData->saturation = 0.68f;
	cinematicData->brightness = 0.13f;
}

void OffScreen::SaveToJson()
{
	json j;

	// ShaderMode を文字列として保存
	std::map<ShaderMode, std::string> shaderModeStrings = {
		{ShaderMode::kNone, "kNone"},
		{ShaderMode::kGray, "kGray"},
		{ShaderMode::kVigneet, "kVigneet"},
		{ShaderMode::kSmooth, "kSmooth"},
		{ShaderMode::kGauss, "kGauss"},
		{ShaderMode::kOutLine, "kOutLine"},
		{ShaderMode::kDepth, "kDepth"},
		{ShaderMode::kBlur, "kBlur"},
		{ShaderMode::kCinematic, "kCinematic"}
	};

	j["shaderMode"] = shaderModeStrings[shaderMode_];

	// 各項目を直接設定
	j["vignette_Exponent"] = vignetteData->vignetteExponent;
	j["vignette_Radius"] = vignetteData->vignetteRadius;
	j["vignette_Strength"] = vignetteData->vignetteStrength;
	j["vignette_Center"] = { vignetteData->vignetteCenter.x, vignetteData->vignetteCenter.y };

	j["smooth_kernelSize"] = smoothData->kernelSize;

	j["gaussian_kernelSize"] = gaussianData->kernelSize;
	j["gaussian_sigma"] = gaussianData->sigma;

	j["depth_kernelSize"] = depthData->kernelSize;

	j["radial_BlurWidth"] = radialData->kBlurWidth;
	j["radial_Center"] = { radialData->kCenter.x, radialData->kCenter.y };

	j["cinematic_contrast"] = cinematicData->contrast;
	j["cinematic_saturation"] = cinematicData->saturation;
	j["cinematic_brightness"] = cinematicData->brightness;

	// ディレクトリを作成し、JSONファイルを保存
	std::filesystem::create_directories("resources/jsons/Offscreen/");
	std::ofstream outFile("resources/jsons/OffScreen/offscreen.json");
	outFile << j.dump(4);  // インデントをつけて保存
}

void OffScreen::LoadFromJson(ShaderMode shaderMode)
{
	// JSONファイルのパス
	std::ifstream inFile("resources/jsons/OffScreen/offscreen.json");

	// JSONファイルが存在しない場合、ロードしない
	if (!inFile)
	{
		std::cerr << "Error: offscreen.json not found!" << std::endl;
		return;
	}

	// JSONをパースする
	json j;
	inFile >> j;

	switch (shaderMode)
	{
	case ShaderMode::kNone:
		break;
	case ShaderMode::kGray:
		break;
	case ShaderMode::kVigneet:
		if (j.contains("vignette_Exponent"))
			vignetteData->vignetteExponent = j["vignette_Exponent"].get<float>();

		if (j.contains("vignette_Radius"))
			vignetteData->vignetteRadius = j["vignette_Radius"].get<float>();

		if (j.contains("vignette_Strength"))
			vignetteData->vignetteStrength = j["vignette_Strength"].get<float>();

		if (j.contains("vignette_Center"))
		{
			vignetteData->vignetteCenter.x = j["vignette_Center"][0].get<float>();
			vignetteData->vignetteCenter.y = j["vignette_Center"][1].get<float>();
		}

		break;
	case ShaderMode::kSmooth:
		// JSONから各項目をロード
		if (j.contains("somooth_kernelSize"))
			smoothData->kernelSize = j["somooth_kernelSize"].get<int>();

		break;
	case ShaderMode::kGauss:
		if (j.contains("gaussian_kernelSize"))
			gaussianData->kernelSize = j["gaussian_kernelSize"].get<int>();

		if (j.contains("gaussian_sigma"))
			gaussianData->sigma = j["gaussian_sigma"].get<float>();

		break;
	case ShaderMode::kOutLine:
		break;
	case ShaderMode::kDepth:
		if (j.contains("depth_kernelSize"))
			depthData->kernelSize = j["depth_kernelSize"].get<int>();

		break;
	case ShaderMode::kBlur:
		if (j.contains("radial_BlurWidth"))
			radialData->kBlurWidth = j["radial_BlurWidth"].get<float>();

		if (j.contains("radial_Center"))
		{
			radialData->kCenter.x = j["radial_Center"][0].get<float>();
			radialData->kCenter.y = j["radial_Center"][1].get<float>();
		}

		break;
	case ShaderMode::kCinematic:
		if (j.contains("cinematic_contrast"))
			cinematicData->contrast = j["cinematic_contrast"].get<float>();

		if (j.contains("cinematic_saturation"))
			cinematicData->saturation = j["cinematic_saturation"].get<float>();

		if (j.contains("cinematic_brightness"))
			cinematicData->brightness = j["cinematic_brightness"].get<float>();
		break;
	default:
		break;
	}
}

void OffScreen::LoadFromJson()
{
	// JSONファイルのパス
	std::ifstream inFile("resources/jsons/OffScreen/offscreen.json");

	// JSONファイルが存在しない場合、ロードしない
	if (!inFile)
	{
		std::cerr << "Error: offscreen.json not found!" << std::endl;
		return;
	}

	// JSONをパースする
	json j;
	inFile >> j;

	// ShaderMode を文字列から enum に変換するマップ
	std::map<std::string, ShaderMode> shaderModeStrings = {
		{"kNone", ShaderMode::kNone},
		{"kGray", ShaderMode::kGray},
		{"kVigneet", ShaderMode::kVigneet},
		{"kSmooth", ShaderMode::kSmooth},
		{"kGauss", ShaderMode::kGauss},
		{"kOutLine", ShaderMode::kOutLine},
		{"kDepth", ShaderMode::kDepth},
		{"kBlur", ShaderMode::kBlur},
		{"kCinematic", ShaderMode::kCinematic}
	};

	// "shaderMode" を文字列として読み込んで、対応する enum に変換
	if (j.contains("shaderMode") && j["shaderMode"].is_string())
	{
		std::string shaderModeStr = j["shaderMode"];
		if (shaderModeStrings.find(shaderModeStr) != shaderModeStrings.end())
		{
			shaderMode_ = shaderModeStrings[shaderModeStr];
		}
		else
		{
			// "shaderMode" が不正な場合はデフォルトに設定
			shaderMode_ = ShaderMode::kNone;
		}
	}
	else
	{
		// "shaderMode" が存在しない場合はデフォルトに設定
		shaderMode_ = ShaderMode::kNone;
	}
}
