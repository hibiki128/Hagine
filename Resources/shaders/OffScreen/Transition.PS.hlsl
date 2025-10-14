#include"FullScreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer TransitionParams : register(b0)
{
    float progress; // 0.0 ~ 1.0 (遷移の進行度)
    float splitSpeed; // 切れ込みが入るスピード (0.0 ~ 1.0の範囲での速さ)
    float slideSpeed; // 上下にはけるスピード
    float splitWidth; // 切れ込みの幅（ピクセル単位の正規化値）
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 画面中央のY座標（0.5）
    float centerY = 0.5f;
    
    float splitPhaseEnd = splitSpeed;
    float splitProgress = saturate(progress / splitPhaseEnd);
    float slideProgress = saturate((progress - splitPhaseEnd) / (1.0f - splitPhaseEnd));
    
    // 現在のX座標（0.0 ~ 1.0）
    float currentX = input.texcoord.x;
    
    // 切れ込みの進行位置（左から右へ）
    // splitProgressが1.0のときにX=1.0まで到達するように調整
    float splitEdge = splitProgress * 1.1f; // 1.1倍して確実に右端まで届かせる
    float splitAmount = saturate((splitEdge - currentX) * 50.0f); // 50.0fは切れ込みの鋭さ
    
    // 切れ込みの半分の幅を計算（到達した位置でのみ開く）
    float currentSplitHalfWidth = splitWidth * 0.5f * splitAmount;
    
    // 上半分か下半分かを判定
    bool isUpperHalf = input.texcoord.y < centerY;
    
    // 切れ込み領域の判定
    float distFromCenter = abs(input.texcoord.y - centerY);
    bool isInSplitArea = distFromCenter < currentSplitHalfWidth;
    
    // 切れ込み領域は透明にする
    if (isInSplitArea)
    {
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }
    
    // スライドのオフセット計算（画面座標をずらす）
    float slideOffset = slideProgress * 1.5f; // 1.5は画面外に完全に出るための係数
    
    // 現在のピクセル位置（画面座標）
    float currentY = input.texcoord.y;
    float newY;
    
    if (isUpperHalf)
    {
        // 上半分は上方向（外側）にスライド
        newY = currentY - slideOffset;
        // 画面外に出たら透明
        if (newY < 0.0f)
        {
            output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
            return output;
        }
    }
    else
    {
        // 下半分は下方向（外側）にスライド
        newY = currentY + slideOffset;
        // 画面外に出たら透明
        if (newY > 1.0f)
        {
            output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
            return output;
        }
    }
    
    // 元のテクスチャ座標でサンプリング（スライドは表示位置のみに影響）
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    return output;
}