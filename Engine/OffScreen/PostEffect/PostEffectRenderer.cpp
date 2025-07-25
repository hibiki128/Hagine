#include "PostEffectRenderer.h"

void PostEffectRenderer::Initialize(DirectXCommon *dxCommon, SrvManager *srvManager, PipeLineManager *psoManager) {
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;
    psoManager_ = psoManager;
    // レンダーバッファの初期化
    renderBuffer_.Initialize(dxCommon_, srvManager_);
    // DSVハンドルの取得
    dsvHandle_ = dxCommon_->GetDSVCPUDescriptorHandle(0);
    finalResultRtvHandle_ = renderBuffer_.GetFinalResultRtvHandle();
}

void PostEffectRenderer::Draw(const PostEffectChain &effectChain, PostEffectParameters &parameters) {

    if (effectChain.IsEmpty()) {
        DrawToFinalResult();
        CopyFinalResultToBackBuffer();
        return;
    }

    bool isFirstInput = true;
    int currentPingPongBuffer = 0;
    int outputBuffer = 0;

    std::vector<int> enabledEffects;
    enabledEffects = effectChain.GetEnabledEffectIndices();

    if (enabledEffects.empty()) {
        // 有効なエフェクトがない場合
        DrawToFinalResult();
        CopyFinalResultToBackBuffer();
        return;
    }

    // 有効なエフェクトチェーンを順番に適用
    for (size_t i = 0; i < enabledEffects.size(); ++i) {
        int effectIndex = enabledEffects[i];
        bool isLastEffect = (i == enabledEffects.size() - 1);

        if (isLastEffect) {
            // 最後のエフェクトは最終結果テクスチャに描画
            DrawSingleEffect(effectChain.GetEffects()[i].shaderMode, isFirstInput, currentPingPongBuffer, -2, parameters); // -2は最終結果を示す特別な値
        } else {
            // 中間エフェクトはピンポンバッファに描画
            DrawSingleEffect(effectChain.GetEffects()[i].shaderMode, isFirstInput, currentPingPongBuffer, outputBuffer,parameters);
            currentPingPongBuffer = outputBuffer;
            outputBuffer = 1 - outputBuffer;
            isFirstInput = false;
        }
    }

    // 最終結果をバックバッファにコピー
    CopyFinalResultToBackBuffer();
}

void PostEffectRenderer::DrawToFinalResult() {
    // 最終結果テクスチャに直接描画（エフェクトなし）
    dxCommon_->GetCommandList()->OMSetRenderTargets(1, &finalResultRtvHandle_, false, &dsvHandle_);

    // バリア遷移
    dxCommon_->BarrierTransition(renderBuffer_.GetFinalResultResource().Get(),
                                 D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // レンダーターゲットをクリア
    D3D12_CLEAR_VALUE clearValue = dxCommon_->GetClearColorValue();
    const float clearColor[4] = {
        clearValue.Color[0],
        clearValue.Color[1],
        clearValue.Color[2],
        clearValue.Color[3]};

    dxCommon_->GetCommandList()->ClearRenderTargetView(renderBuffer_.GetFinalResultRtvHandle(), clearColor, 0, nullptr);

    // パイプライン設定
    psoManager_->DrawCommonSetting(PipelineType::kRender, BlendMode::kNormal, ShaderMode::kNone);

    // 入力テクスチャを設定
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon_->GetOffScreenGPUHandle());

    // 描画
    dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);

    // バリア遷移
    dxCommon_->BarrierTransition(renderBuffer_.GetFinalResultResource().Get(),
                                 D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void PostEffectRenderer::CopyFinalResultToBackBuffer() {
    // バックバッファに最終結果をコピー
    UINT backBufferIndex = dxCommon_->GetSwapChain()->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->GetRTVCPUDescriptorHandle(backBufferIndex);
    dxCommon_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle_);

    // パイプライン設定
    psoManager_->DrawCommonSetting(PipelineType::kRender, BlendMode::kNormal, ShaderMode::kNone);

    // 最終結果テクスチャを入力として設定
    dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, renderBuffer_.GetFinalResultSrvHandleGPU());

    // 描画
    dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void PostEffectRenderer::DrawSingleEffect(ShaderMode mode, bool isFirstInput, int inputPingPongIndex, int outputRtvIndex,
                          PostEffectParameters &parameters) {
    // DirectXCommonからクリアカラーを取得
    D3D12_CLEAR_VALUE clearValue = dxCommon_->GetClearColorValue();
    const float clearColor[4] = {
        clearValue.Color[0],
        clearValue.Color[1],
        clearValue.Color[2],
        clearValue.Color[3]};

   // 出力先を設定
    if (outputRtvIndex == -2) {
        // 最終結果テクスチャに描画
        dxCommon_->GetCommandList()->OMSetRenderTargets(1, &finalResultRtvHandle_, false, &dsvHandle_);
        dxCommon_->BarrierTransition(renderBuffer_.GetFinalResultResource().Get(),
                                     D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
        dxCommon_->GetCommandList()->ClearRenderTargetView(renderBuffer_.GetFinalResultRtvHandle(), clearColor, 0, nullptr);
    } else if (outputRtvIndex != -1) {
        // ピンポンバッファに描画
        D3D12_CPU_DESCRIPTOR_HANDLE pingPongRtvHandle = renderBuffer_.GetPingPongRtvHandle(outputRtvIndex);
        dxCommon_->GetCommandList()->OMSetRenderTargets(1, &pingPongRtvHandle, false, &dsvHandle_);
        dxCommon_->BarrierTransition(renderBuffer_.GetPingPongResource(outputRtvIndex).Get(),
                                     D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

        D3D12_CPU_DESCRIPTOR_HANDLE pingPongRtvHandleForClear = renderBuffer_.GetPingPongRtvHandle(outputRtvIndex);
        dxCommon_->GetCommandList()->ClearRenderTargetView(pingPongRtvHandleForClear, clearColor, 0, nullptr);
    } else {
        // バックバッファに描画（この分岐は使われなくなる）
        UINT backBufferIndex = dxCommon_->GetSwapChain()->GetCurrentBackBufferIndex();
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->GetRTVCPUDescriptorHandle(backBufferIndex);
        dxCommon_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle_);
    }

    // パイプライン設定とシェーダーパラメータ設定（既存のコードと同じ）
    psoManager_->DrawCommonSetting(PipelineType::kRender, BlendMode::kNormal, mode);

    parameters.SetShaderParameters(mode, dxCommon_->GetCommandList().Get(), srvManager_, dxCommon_);

    // 入力テクスチャを設定
    if (isFirstInput) {
        // 最初の入力はオフスクリーンバッファ
        dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon_->GetOffScreenGPUHandle());
    } else {
        // 2回目以降はピンポンバッファ
        if (inputPingPongIndex >= 0 && inputPingPongIndex < renderBuffer_.GetPingPongBufferCount()) {
            dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, renderBuffer_.GetPingPongSrvHandleGPU(inputPingPongIndex));
        } else {
            // エラーハンドリング - フォールバックとしてオフスクリーンバッファを使用
            dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon_->GetOffScreenGPUHandle());
        }
    }

    // 描画
    dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);

    // バリア遷移
    if (outputRtvIndex == -2) {
        // 最終結果テクスチャの場合
        dxCommon_->BarrierTransition(renderBuffer_.GetFinalResultResource().Get(),
                                     D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    } else if (outputRtvIndex != -1) {
        // ピンポンバッファの場合
        dxCommon_->BarrierTransition(renderBuffer_.GetPingPongResource(outputRtvIndex).Get(),
                                     D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    }
}
