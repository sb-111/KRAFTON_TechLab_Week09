struct FShadowInfo
{
    row_major float4x4 ViewProjectionMatrix;
    
    float3 LightPosition;
    // Depth normalization parameters per light (linear view-space z)
    // x = Near, y = Far
    float Near;
    
    float Far;
    float3 Padding; // 16-byte alignment for array elements
   // float4 AtlasUv; // 0~1로 정규화된 뷰포트 좌표 (x, y, width, height)
    
    float ShadowBias;
    float ShadowSlopeBias;
    float MaxSlopeDepthBias;
    float Padding2; // 16-byte alignment for array elements
};

// b12: 그림자 정보 버퍼 (PS only)
cbuffer ShadowBuffer : register(b12)
{
    // 그림자 최대 100개로 일단 하드코딩함
    FShadowInfo ShadowInfoList[100];
    int NumSpotShadow;
    int NumPointShadow;
    float2 Padding; // 16바이트 정렬을 위한 패딩
};

Texture2DArray g_SpotShadowTextureArray : register(t2);
Texture2DArray g_CSMShadowTextureArray : register(t5); 
TextureCubeArray g_PointShadowTextureArray : register(t6);
Texture2D g_DirectionalShadowTexture : register(t7);

SamplerComparisonState ShadowSampler : register(s2);
SamplerState ShadowMomentsSampler : register(s3); // VSM sampling (LinearClamp)
