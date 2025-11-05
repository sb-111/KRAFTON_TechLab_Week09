struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

Texture2D SourceTexture : register(t0);
SamplerState SourceSampler : register(s1);

cbuffer PostProcessChainConstants : register(b0)
{
    float Gamma;
    float VignetteIntensity;
    float VignetteRadius;
    float LetterBoxSize;

    int bEnableGammaCorrection;
    int bEnableVignetting;
    int bEnableLetterbox;
    float Pad;
};

float4 mainPS(VS_OUTPUT Input) : SV_Target
{
    float4 Output = SourceTexture.Sample(SourceSampler, Input.TexCoord);
    
    // Apply Gamma
    if(bEnableGammaCorrection)
    {
        Output.rgb = pow(Output.rgb, 1.0f / Gamma);
    }
    
    // Apply Vignetting
    if(bEnableVignetting)
    {
        float2 Dist = Input.TexCoord - 0.5f;
        float DistFromCenter = length(Dist);

        // VignetteRadius: 밝은 영역의 반경 (0.0 = 전체 어두움, 1.0 = 어두움 없음)
        // VignetteIntensity: 가장 어두운 부분의 강도 (0.0 = 효과 없음, 1.0 = 완전히 어두움)
        float Vignette = smoothstep(0.8, VignetteRadius, DistFromCenter);
        Vignette = pow(Vignette, 0.5); // 부드러운 전환

        // Intensity를 사용해 최종 어둡기 조절
        Output.rgb *= lerp(1.0f - VignetteIntensity, 1.0f, Vignette);
    }
    if(bEnableLetterbox)
    {
        if(Input.TexCoord.y < LetterBoxSize || Input.TexCoord.y > 1.0f - LetterBoxSize)
        {
            Output.rgb = float3(0.0f, 0.0f, 0.0f);
        }
    }
    return Output;
    
}