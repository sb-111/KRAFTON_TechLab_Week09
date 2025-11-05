struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

cbuffer FColorBufferType : register(b3)
{
    float4 Color;
    uint UUID;
    float3 Padding;
};

float4 mainPS(PS_INPUT Input) : SV_Target0
{
    return Color;
}