#include"object3d.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLights> gPointLights : register(b3);
ConstantBuffer<SpotLights> gSpotLights : register(b4);
SamplerState gSampler : register(s0);
Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEngironmentTexture : register(t1);

PixelShaderOutput main(VertexShaderOutput input)
{
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    PixelShaderOutput output;
    
    if (gMaterial.enableLighting != 0)
    {
        output.color.rgb = float3(0.0f, 0.0f, 0.0f);
        
        // 指向性ライトの計算
        if (gDirectionalLight.active != 0)
        {
            if (gDirectionalLight.HalfLambert != 0)
            {
                float NdotL = dot(normalize(input.normal), normalize(-gDirectionalLight.direction));
                float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
                output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
            }
            else if (gDirectionalLight.BlinnPhong != 0)
            {
                // 指向性ライトの計算
                float NdotLDirectional = dot(normalize(input.normal), normalize(-gDirectionalLight.direction));
                float cosDirectional = pow(NdotLDirectional * 0.5f + 0.5f, 2.0f);
                float3 toEyeDirectional = normalize(gCamera.worldPosition - input.worldPosition);
                float3 halfVectorDirectional = normalize(-gDirectionalLight.direction + toEyeDirectional);
                float NDotHDirectional = dot(normalize(input.normal), halfVectorDirectional);
                float specularPowDirectional = pow(saturate(NDotHDirectional), gMaterial.shininess);

                // 拡散反射
                float3 diffuseDirectional = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cosDirectional * gDirectionalLight.intensity;
                // 鏡面反射
                float3 specularDirectional = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPowDirectional * float3(1.0f, 1.0f, 1.0f);
                // 拡散反射 + 鏡面反射
                output.color.rgb += diffuseDirectional + specularDirectional;
            }
        }
        
        // 複数のポイントライト計算
        for (int i = 0; i < min(gPointLights.count, MAX_POINT_LIGHTS); i++)
        {
            PointLight pointLight = gPointLights.lights[i];
            
            if (pointLight.active != 0)
            {
                if (pointLight.HalfLambert != 0)
                {
                    // ライトからピクセルへの方向ベクトルを計算
                    float3 lightDir = pointLight.position - input.worldPosition; // ライト位置からのベクトル
                    float distance = length(lightDir); // ライトまでの距離
                    lightDir = normalize(lightDir); // ライト方向を正規化

                    // 法線ベクトルとライト方向ベクトルの内積を計算
                    float NdotL = dot(normalize(input.normal), lightDir);

                    // HalfLambert反射の補正（NdotL * 0.5f + 0.5f による補正）
                    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);

                    // 距離減衰の適用
                    float factor = pow(saturate(-distance / pointLight.radius + 1.0f), pointLight.decay);

                    // 出力色を計算（拡散反射部分にライトの影響を加える）
                    output.color.rgb += gMaterial.color.rgb * textureColor.rgb * pointLight.color.rgb * cos * pointLight.intensity * factor;
                }
                else if (pointLight.BlinnPhong != 0)
                {
                    // ポイントライトの計算
                    float3 lightDir = pointLight.position - input.worldPosition; // ライトからの方向
                    float distance = length(lightDir); // ライトまでの距離
                    lightDir = normalize(lightDir); // 正規化
     
                    // 拡散反射
                    float NdotLPoint = dot(normalize(input.normal), lightDir);
                    float cosPoint = max(NdotLPoint, 0.0f); // コサイン値
                    float3 diffusePoint = gMaterial.color.rgb * textureColor.rgb * pointLight.color.rgb * cosPoint * pointLight.intensity;

                    // 鏡面反射
                    float3 toEyePoint = normalize(gCamera.worldPosition - input.worldPosition);
                    float3 halfVectorPoint = normalize(lightDir + toEyePoint);
                    float NDotHPoint = dot(normalize(input.normal), halfVectorPoint);
                    float specularPowPoint = pow(saturate(NDotHPoint), gMaterial.shininess);
                    float3 specularPoint = pointLight.color.rgb * pointLight.intensity * specularPowPoint * float3(1.0f, 1.0f, 1.0f);

                    float factor = pow(saturate(-distance / pointLight.radius + 1.0f), pointLight.decay);
                
                    // 修正：単体版と同じ計算方式に統一
                    output.color.rgb += (diffusePoint + specularPoint) * factor;
                }
            }
        }
        
        // 複数のスポットライト計算
        for (int j = 0; j < min(gSpotLights.count, MAX_SPOT_LIGHTS); j++)
        {
            SpotLight spotLight = gSpotLights.lights[j];
            
            if (spotLight.active != 0)
            {
                // HalfLambertの場合
                if (spotLight.HalfLambert != 0)
                {
                    float3 spotLightDirectionOnSurface = normalize(input.worldPosition - spotLight.position);
                    float cosAngle = dot(spotLightDirectionOnSurface, spotLight.direction);

                    float falloffFactor = saturate((cosAngle - spotLight.cosAngle) / (1.0f - spotLight.cosAngle));

                    float distance = length(spotLight.position - input.worldPosition);
                    float attenuationFactor = pow(saturate(-distance / spotLight.distance + 1.0f), spotLight.decay);

                    float NdotL = max(dot(normalize(input.normal), -spotLightDirectionOnSurface), 0.0f);
                    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f); // HalfLambert

                    // 修正：材質色とテクスチャ色を追加
                    output.color.rgb += gMaterial.color.rgb * textureColor.rgb * spotLight.color.rgb * spotLight.intensity * attenuationFactor * falloffFactor * cos;
                }

                // BlinnPhongの場合
                if (spotLight.BlinnPhong != 0)
                {
                    float3 spotLightDirectionOnSurface = normalize(input.worldPosition - spotLight.position);
                    float cosAngle = dot(spotLightDirectionOnSurface, spotLight.direction);

                    float falloffFactor = saturate((cosAngle - spotLight.cosAngle) / (1.0f - spotLight.cosAngle));

                    float distance = length(spotLight.position - input.worldPosition);
                    float attenuationFactor = pow(saturate(-distance / spotLight.distance + 1.0f), spotLight.decay);

                    // 拡散反射計算
                    float NdotLSpot = max(dot(normalize(input.normal), -spotLightDirectionOnSurface), 0.0f);
                    float3 diffuseSpot = gMaterial.color.rgb * textureColor.rgb * spotLight.color.rgb * NdotLSpot * spotLight.intensity;

                    // 鏡面反射計算
                    float3 toEyeSpot = normalize(gCamera.worldPosition - input.worldPosition);
                    float3 halfVectorSpot = normalize(-spotLightDirectionOnSurface + toEyeSpot);
                    float NDotHSpot = dot(normalize(input.normal), halfVectorSpot);
                    float specularFactor = pow(saturate(NDotHSpot), gMaterial.shininess);
                    float3 specularSpot = spotLight.color.rgb * spotLight.intensity * specularFactor * float3(1.0f, 1.0f, 1.0f);

                    // 拡散反射 + 鏡面反射
                    float3 spotLightContribution = (diffuseSpot + specularSpot) * attenuationFactor * falloffFactor;

                    output.color.rgb += spotLightContribution;
                }
            }
        }
        
        // 環境マッピング
        float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
        float4 environmentColor = gEngironmentTexture.Sample(gSampler, reflectedVector);
        
        // 環境マップの色を加算
        output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;
        
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    if (textureColor.a == 0.0f)
    {
        discard;
    }
    if (output.color.a == 0.0f)
    {
        discard;
    }
    
    return output;
}