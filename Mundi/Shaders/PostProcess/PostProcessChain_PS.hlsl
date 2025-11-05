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
        // Calculate vignette effect based on distance, radius, and intensity
        float Vignette = 1.0f - smoothstep(0.0, VignetteRadius, length(Dist)) * VignetteIntensity;
        Output.rgb *= Vignette;
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