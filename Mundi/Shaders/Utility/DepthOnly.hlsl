// Shared shadow info
#include "../Common/ShadowBuffers.hlsl"

//#define USE_VSM_MOMENTS 1

// Per-object matrix
cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 world;
}

// b1: ViewProjBuffer (VS) - ViewProjBufferType과 일치
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 Dummy1;
    row_major float4x4 Dummy2;
    row_major float4x4 Dummy3;
    row_major float4x4 Dummy4;
    row_major float4x4 ViewProjectionMatrix; // 이 쉐이더는 이것만 사용한다고 가정. 다른 행렬(아마 카메라 관련 행렬들로 채워져 있음)은 사용하면 안됨
    // 그냥 별개의 상수 버퍼를 쓰거나 다른 상수 버퍼를 재활용해야함. b1 카메라는 모든 셰이더가 정적으로 사용하는 상수 버퍼
};

// Which shadow VP to use and light type
cbuffer ShadowBufferIndex : register(b9)
{
    int ShadowBufferIndex;
    int LightType; // 0=Ambient,1=Directional,2=Point,3=Spot,4=DirectionalCSM
}

struct VS_INPUT
{
    float3 posModel : POSITION;
};

struct PS_INPUT
{
    float4 posProj : SV_POSITION;
    float clipZ : TEXCOORD0; // clip-space z
    float clipW : TEXCOORD1; // clip-space w
    float3 worldPos : TEXCOORD2; // for point VSM radial depth
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    float4 posWorld = mul(float4(input.posModel, 1.0f), world);
    // Use when not CSM
    float4 posClip = mul(posWorld, ViewProjectionMatrix);
    output.posProj = posClip;

    // clip-space z, w 전달 (PS에서 z/w 계산)
    output.clipZ = posClip.z;
    output.clipW = posClip.w;
    output.worldPos = posWorld.xyz;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    #define USE_VSM_MOMENTS
#ifdef USE_VSM_MOMENTS
    // HLSL-side light type tags (must match C++)
    static const int LT_Directional = 1;
    static const int LT_Point       = 2;
    static const int LT_Spot        = 3;

    float near = ShadowInfoList[ShadowBufferIndex].Near;
    float far = ShadowInfoList[ShadowBufferIndex].Far;

    float zNorm;
    if (LightType == LT_Point)
    {
        // Radial distance for point light VSM
        float3 lightPos = ShadowInfoList[ShadowBufferIndex].LightPosition;
        float dist = length(input.worldPos - lightPos);
        zNorm = saturate((dist - near) / max(far - near, 1e-6));
    }
    else
    {
        // Use clip-space w (view-space z) for spot/directional
        float viewZ = input.clipW;
        zNorm = saturate((viewZ - near) / max(far - near, 1e-6));
    }
    
    float2 moments;
    moments.x = zNorm;
    moments.y = zNorm * zNorm;

    return float4(moments, 0, 1);
#else
    return float4(1, 1, 1, 1);
#endif
}
