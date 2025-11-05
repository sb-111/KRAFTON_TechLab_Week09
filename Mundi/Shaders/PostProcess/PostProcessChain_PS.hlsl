struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0; // 스크린 UV (뷰포트 로컬 [0,1]이 아님!)
};

Texture2D SourceTexture : register(t0);
SamplerState SourceSampler : register(s1);

// ViewportConstants: VS와 동일한 구조체 (b10, VS+PS 모두 바인딩됨)
cbuffer ViewportConstants : register(b10)
{
    // x: TopLeftX, y: TopLeftY, z: Width, w: Height (픽셀 단위)
    float4 ViewportRect;
    // x: Screen Width, y: Screen Height, z: 1/ScreenW, w: 1/ScreenH
    float4 ScreenSize;
};

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
    // 스크린 UV를 뷰포트 로컬 UV [0,1]로 변환
    // VS에서: TexCoord = ViewportStartUV + (LocalUV * ViewportUVSpan)
    // 역변환: LocalUV = (TexCoord - ViewportStartUV) / ViewportUVSpan
    float2 ViewportStartUV = ViewportRect.xy * ScreenSize.zw;
    float2 ViewportUVSpan = ViewportRect.zw * ScreenSize.zw;
    float2 LocalUV = (Input.TexCoord - ViewportStartUV) / ViewportUVSpan;

    // 텍스처 샘플링은 반드시 원본 스크린 UV 사용!
    float4 Output = SourceTexture.Sample(SourceSampler, Input.TexCoord);

    // Apply Gamma
    if(bEnableGammaCorrection)
    {
        Output.rgb = pow(Output.rgb, 1.0f / Gamma);
    }

    // Apply Vignetting
    if(bEnableVignetting)
    {
        // 뷰포트 로컬 UV를 중심 기준 -1 ~ 1 범위로 변환
        float2 UV = LocalUV * 2.0f - 1.0f;

        // 뷰포트 종횡비 보정으로 완벽한 원형 비네팅 만들기
        // 16:9 화면의 경우 AspectRatio = 9/16 = 0.5625
        // Y축을 확장하여 가로로 긴 화면에서도 원형 유지
        float AspectRatio = ViewportRect.w / ViewportRect.z; // Height / Width
        UV.y /= AspectRatio;

        float DistFromCenter = length(UV);

        // VignetteRadius: 밝은 영역의 반경 (0.0 ~ 1.4, 작을수록 좁은 영역)
        // VignetteIntensity: 어두운 정도 (0.0 = 효과 없음, 1.0 = 완전히 검게)
        float Vignette = 1.0f - smoothstep(VignetteRadius, VignetteRadius + 0.5f, DistFromCenter);

        // Intensity를 적용하여 최종 밝기 계산
        Output.rgb *= lerp(1.0f, Vignette, VignetteIntensity);
    }

    // Apply Letterbox
    if(bEnableLetterbox)
    {
        // 뷰포트 로컬 UV 사용
        if(LocalUV.y < LetterBoxSize || LocalUV.y > 1.0f - LetterBoxSize)
        {
            Output.rgb = float3(0.0f, 0.0f, 0.0f);
        }
    }
    return Output;
}