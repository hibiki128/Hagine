#include "PostEffectParameters.h"
#include <Graphics/Srv/SrvManager.h>
#include <Graphics/Texture/TextureManager.h>
#include <Graphics/PipeLine/PipeLineManager.h>
#include <d3d12.h>

void PostEffectParameters::Initialize(DirectXCommon *dxCommon) {
    dxCommon_ = dxCommon;
    TextureManager::GetInstance()->LoadTexture(texPath_);
    CreateAllBuffers();
}

void PostEffectParameters::SetShaderParameters(ShaderMode mode, ID3D12GraphicsCommandList *commandList,
                                               SrvManager *srvManager, DirectXCommon *dxCommon) {
    switch (mode) {
    case ShaderMode::kVigneet:
        commandList->SetGraphicsRootConstantBufferView(1, vignetteResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kSmooth:
        commandList->SetGraphicsRootConstantBufferView(1, smoothResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kGauss:
        commandList->SetGraphicsRootConstantBufferView(1, gaussianResouce->GetGPUVirtualAddress());
        break;
    case ShaderMode::kDepth:
        depthData->projectionInverse = Inverse(projectionInverse_);
        commandList->SetGraphicsRootConstantBufferView(1, depthResouce->GetGPUVirtualAddress());
        commandList->SetGraphicsRootDescriptorTable(2, dxCommon->GetDepthGPUHandle());
        break;
    case ShaderMode::kBlur:
        commandList->SetGraphicsRootConstantBufferView(1, radialResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kCinematic:
        commandList->SetGraphicsRootConstantBufferView(1, cinematicResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kDissolve:
        commandList->SetGraphicsRootConstantBufferView(1, dissolveResource->GetGPUVirtualAddress());
        srvManager->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(texPath_));
        break;
    case ShaderMode::kRandom:
        commandList->SetGraphicsRootConstantBufferView(1, randomResource->GetGPUVirtualAddress());
        break;
    case ShaderMode::kFocusLine:
        commandList->SetGraphicsRootConstantBufferView(1, focusLineResource->GetGPUVirtualAddress());
        break;
    }
}

void PostEffectParameters::UpdateTimeParameters(float deltaTime) {
    if (randomData) {
        randomData->time += deltaTime;
    }
    if (focusLineData) {
        focusLineData->time += deltaTime;
    }
}

void PostEffectParameters::SaveParameters(DataHandler *dataHandler)const {
    // Vignette パラメータ
    if (vignetteData) {
        dataHandler->Save<float>("vignette_exponent", vignetteData->vignetteExponent);
        dataHandler->Save<float>("vignette_radius", vignetteData->vignetteRadius);
        dataHandler->Save<float>("vignette_strength", vignetteData->vignetteStrength);
        dataHandler->Save<Vector2>("vignette_center", vignetteData->vignetteCenter);
    }

    // Smooth パラメータ
    if (smoothData) {
        dataHandler->Save<int>("smooth_kernelSize", smoothData->kernelSize);
    }

    // Gaussian パラメータ
    if (gaussianData) {
        dataHandler->Save<int>("gaussian_kernelSize", gaussianData->kernelSize);
        dataHandler->Save<float>("gaussian_sigma", gaussianData->sigma);
    }

    // Depth パラメータ
    if (depthData) {
        dataHandler->Save<int>("depth_kernelSize", depthData->kernelSize);
    }

    // Radial Blur パラメータ
    if (radialData) {
        dataHandler->Save<Vector2>("radial_center", radialData->kCenter);
        dataHandler->Save<float>("radial_blurWidth", radialData->kBlurWidth);
    }

    // Cinematic パラメータ
    if (cinematicData) {
        dataHandler->Save<Vector2>("cinematic_resolution", cinematicData->iResolution);
        dataHandler->Save<float>("cinematic_contrast", cinematicData->contrast);
        dataHandler->Save<float>("cinematic_saturation", cinematicData->saturation);
        dataHandler->Save<float>("cinematic_brightness", cinematicData->brightness);
    }

    // Dissolve パラメータ
    if (dissolveData) {
        dataHandler->Save<float>("dissolve_threshold", dissolveData->threshold);
        dataHandler->Save<float>("dissolve_edgeWidth", dissolveData->edgeWidth);
        dataHandler->Save<Vector3>("dissolve_edgeColor", dissolveData->edgeColor);
    }

    // Focus Line パラメータ
    if (focusLineData) {
        dataHandler->Save<float>("focusLine_lines", focusLineData->lines);
        dataHandler->Save<float>("focusLine_width", focusLineData->width);
        dataHandler->Save<float>("focusLine_speed", focusLineData->speed);
        dataHandler->Save<float>("focusLine_intensity", focusLineData->intensity);
        dataHandler->Save<float>("focusLine_centerRadius", focusLineData->centerRadius);
        dataHandler->Save<float>("focusLine_maxDistance", focusLineData->maxDistance);
        dataHandler->Save<Vector4>("focusLine_lineColor", focusLineData->lineColor);
    }
}

void PostEffectParameters::LoadParameters(DataHandler *dataHandler) {
    // Vignette パラメータ
    if (vignetteData) {
        vignetteData->vignetteExponent = dataHandler->Load<float>("vignette_exponent", 1.0f);
        vignetteData->vignetteRadius = dataHandler->Load<float>("vignette_radius", 1.0f);
        vignetteData->vignetteStrength = dataHandler->Load<float>("vignette_strength", 1.0f);
        vignetteData->vignetteCenter = dataHandler->Load<Vector2>("vignette_center", {0.5f, 0.5f});
    }

    // Smooth パラメータ
    if (smoothData) {
        smoothData->kernelSize = dataHandler->Load<int>("smooth_kernelSize", 3);
    }

    // Gaussian パラメータ
    if (gaussianData) {
        gaussianData->kernelSize = dataHandler->Load<int>("gaussian_kernelSize", 3);
        gaussianData->sigma = dataHandler->Load<float>("gaussian_sigma", 1.0f);
    }

    // Depth パラメータ
    if (depthData) {
        depthData->kernelSize = dataHandler->Load<int>("depth_kernelSize", 3);
    }

    // Radial Blur パラメータ
    if (radialData) {
        radialData->kCenter = dataHandler->Load<Vector2>("radial_center", {0.5f, 0.5f});
        radialData->kBlurWidth = dataHandler->Load<float>("radial_blurWidth", 0.01f);
    }

    // Cinematic パラメータ
    if (cinematicData) {
        cinematicData->iResolution = dataHandler->Load<Vector2>("cinematic_resolution", {1280.0f, 720.0f});
        cinematicData->contrast = dataHandler->Load<float>("cinematic_contrast", 1.05f);
        cinematicData->saturation = dataHandler->Load<float>("cinematic_saturation", 0.68f);
        cinematicData->brightness = dataHandler->Load<float>("cinematic_brightness", 0.13f);
    }

    // Dissolve パラメータ
    if (dissolveData) {
        dissolveData->threshold = dataHandler->Load<float>("dissolve_threshold", 0.0f);
        dissolveData->edgeWidth = dataHandler->Load<float>("dissolve_edgeWidth", 0.01f);
        dissolveData->edgeColor = dataHandler->Load<Vector3>("dissolve_edgeColor", {1.0f, 0.0f, 0.0f});
    }

    // Focus Line パラメータ
    if (focusLineData) {
        focusLineData->lines = dataHandler->Load<float>("focusLine_lines", 16.0f);
        focusLineData->width = dataHandler->Load<float>("focusLine_width", 0.01f);
        focusLineData->speed = dataHandler->Load<float>("focusLine_speed", 1.0f);
        focusLineData->intensity = dataHandler->Load<float>("focusLine_intensity", 0.3f);
        focusLineData->centerRadius = dataHandler->Load<float>("focusLine_centerRadius", 0.5f);
        focusLineData->maxDistance = dataHandler->Load<float>("focusLine_maxDistance", 1.0f);
        focusLineData->lineColor = dataHandler->Load<Vector4>("focusLine_lineColor", {1.0f, 1.0f, 1.0f, 1.0f});
    }
}

void PostEffectParameters::DrawParameterUI(ShaderMode mode) {
#ifdef _DEBUG
    switch (mode) {
    case ShaderMode::kVigneet:
        if (vignetteData) {
            ImGui::DragFloat("滑らかさ", &vignetteData->vignetteExponent, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("半径", &vignetteData->vignetteRadius, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("強度", &vignetteData->vignetteStrength, 0.01f);
            ImGui::DragFloat2("中心", &vignetteData->vignetteCenter.x, 0.01f, -10.0f, 10.0f);
        }
        break;
    case ShaderMode::kSmooth:
        if (smoothData) {
            ImGui::DragInt("カーネルサイズ", &smoothData->kernelSize, 2, 3, 7);
        }
        break;
    case ShaderMode::kGauss:
        if (gaussianData) {
            ImGui::DragInt("カーネルサイズ", &gaussianData->kernelSize, 2, 3, 7);
            ImGui::DragFloat("シグマ", &gaussianData->sigma, 0.01f, 0.01f, 10.0f);
        }
        break;
    case ShaderMode::kDepth:
        if (depthData) {
            ImGui::DragInt("カーネルサイズ", &depthData->kernelSize, 2, 3, 7);
        }
        break;
    case ShaderMode::kBlur:
        if (radialData) {
            ImGui::DragFloat2("中心座標", &radialData->kCenter.x, 0.1f);
            ImGui::DragFloat("幅", &radialData->kBlurWidth, 0.01f);
        }
        break;
    case ShaderMode::kCinematic:
        if (cinematicData) {
            ImGui::DragFloat("コンストラクト", &cinematicData->contrast, 0.01f);
            ImGui::DragFloat("彩度", &cinematicData->saturation, 0.01f);
            ImGui::DragFloat("輝度", &cinematicData->brightness, 0.01f);
        }
        break;
    case ShaderMode::kDissolve:
        if (dissolveData) {
            ImGui::SliderFloat("Threshold", &dissolveData->threshold, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Edge Width", &dissolveData->edgeWidth, 0.0f, 0.5f, "%.3f");
            ImGui::ColorEdit3("Edge Color", reinterpret_cast<float *>(&dissolveData->edgeColor));
            ImGui::Checkbox("Invert", &dissolveData->invert);
        }
        break;
    case ShaderMode::kRandom:
        // Randomは時間のみなので特にUIなし
        break;
    case ShaderMode::kFocusLine:
        if (focusLineData) {
            ImGui::DragFloat("Time", &focusLineData->time, 0.1f);
            ImGui::DragFloat("Lines", &focusLineData->lines, 0.1f);
            ImGui::DragFloat("Width", &focusLineData->width, 0.01f);
            ImGui::DragFloat("Speed", &focusLineData->speed, 0.1f);
            ImGui::DragFloat("Intensity", &focusLineData->intensity, 0.2f, 1.5f);

            ImGui::Separator();
            ImGui::Text("Area Settings");
            ImGui::DragFloat("Center Radius", &focusLineData->centerRadius, 0.1f);
            ImGui::DragFloat("Max Distance", &focusLineData->maxDistance, 0.1f);

            ImGui::Separator();
            ImGui::Text("Line Color");
            ImGui::ColorEdit3("Color", &focusLineData->lineColor.x);
        }
        break;
    }
#endif // _DEBUG
}

void PostEffectParameters::CreateAllBuffers() {
    CreateSmooth();
    CreateGauss();
    CreateVignette();
    CreateDepth();
    CreateRadial();
    CreateCinematic();
    CreateDissolve();
    CreateRandom();
    CreateFocusLine();
}

void PostEffectParameters::CreateSmooth() {
    smoothResource = dxCommon_->CreateBufferResource(sizeof(KernelSettings));
    smoothResource->Map(0, nullptr, reinterpret_cast<void **>(&smoothData));
    smoothData->kernelSize = 3;
}

void PostEffectParameters::CreateGauss() {
    gaussianResouce = dxCommon_->CreateBufferResource(sizeof(GaussianParams));
    gaussianResouce->Map(0, nullptr, reinterpret_cast<void **>(&gaussianData));
    gaussianData->kernelSize = 3;
    gaussianData->sigma = 1;
}

void PostEffectParameters::CreateVignette() {
    vignetteResource = dxCommon_->CreateBufferResource(sizeof(VignetteParameter));
    vignetteResource->Map(0, nullptr, reinterpret_cast<void **>(&vignetteData));
    vignetteData->vignetteExponent = 1.0f;
    vignetteData->vignetteRadius = 1.0f;
    vignetteData->vignetteStrength = 1.0f;
    vignetteData->vignetteCenter = {0.5f, 0.5f};
}

void PostEffectParameters::CreateDepth() {
    depthResouce = dxCommon_->CreateBufferResource(sizeof(Depth));
    depthResouce->Map(0, nullptr, reinterpret_cast<void **>(&depthData));
    depthData->projectionInverse = MakeIdentity4x4();
    depthData->kernelSize = 3;
}

void PostEffectParameters::CreateRadial() {
    radialResource = dxCommon_->CreateBufferResource(sizeof(RadialBlur));
    radialResource->Map(0, nullptr, reinterpret_cast<void **>(&radialData));
    radialData->kBlurWidth = 0.01f;
    radialData->kCenter = {0.5f, 0.5f};
}

void PostEffectParameters::CreateCinematic() {
    cinematicResource = dxCommon_->CreateBufferResource(sizeof(Cinematic));
    cinematicResource->Map(0, nullptr, reinterpret_cast<void **>(&cinematicData));
    cinematicData->iResolution = {1280.0f, 720.0f};
    cinematicData->contrast = 1.05f;
    cinematicData->saturation = 0.68f;
    cinematicData->brightness = 0.13f;
}

void PostEffectParameters::CreateDissolve() {
    dissolveResource = dxCommon_->CreateBufferResource(sizeof(Dissolve));
    dissolveResource->Map(0, nullptr, reinterpret_cast<void **>(&dissolveData));
    dissolveData->threshold = 0.0f;
    dissolveData->edgeWidth = 0.01f;
    dissolveData->edgeColor = {1.0f, 1.0f, 1.0f}; // 白色
    dissolveData->invert = false;                 // 初期値はfalse
}

void PostEffectParameters::CreateRandom() {
    randomResource = dxCommon_->CreateBufferResource(sizeof(Random));
    randomResource->Map(0, nullptr, reinterpret_cast<void **>(&randomData));
    randomData->time = 0.0f; // 初期値は適宜設定
}

void PostEffectParameters::CreateFocusLine() {
    focusLineResource = dxCommon_->CreateBufferResource(sizeof(FocusLine));
    focusLineResource->Map(0, nullptr, reinterpret_cast<void **>(&focusLineData));
    focusLineData->lines = 16.0f;
    focusLineData->width = 0.01f;
    focusLineData->speed = 1.0f;
    focusLineData->intensity = 0.3f;
}
