//================================================================================================
// Filename:      TileDebugVisualization_PS.hlsl
// Description:   타일 기반 라이트 컬링 디버그 시각화 픽셀 셰이더
//                각 타일의 라이트 개수를 히트맵으로 표시
//================================================================================================

// b11: 타일 컬링 설정 상수 버퍼 (Uber와 동일)
cbuffer TileCullingBuffer : register(b11)
{
    uint TileSize;          // 타일 크기 (픽셀, 기본 16)
    uint TileCountX;        // 가로 타일 개수
    uint TileCountY;        // 세로 타일 개수
    uint bUseTileCulling;   // 타일 컬링 활성화 여부 (0=비활성화, 1=활성화)
};

// b10: Viewport constants (compensate UI/overlay offset)
cbuffer ViewportConstants : register(b10)
{
    float4 ViewportRect;   // xy = TopLeft, zw = Size
    float4 ScreenSize;     // xy = RT Size, zw = 1/Size
};

// t0: 원본 씬 텍스처
Texture2D g_SceneTexture : register(t0);
SamplerState g_SamplerLinear : register(s0);

// t2: 타일별 라이트 인덱스 Structured Buffer
// 구조: [TileIndex * MaxLightsPerTile] = LightCount
//       [TileIndex * MaxLightsPerTile + 1 ~ ...] = LightIndices
StructuredBuffer<uint> g_TileLightIndices : register(t2);

// b13: 클러스터(Z-슬라이스) 상수 버퍼 (Uber와 동일)
cbuffer ClusterCullingBuffer : register(b13)
{
    uint  ClusterCountZ;   // Z 슬라이스 개수
    float ClusterNearZ;    // View-space Near (양수)
    float ClusterFarZ;     // View-space Far  (양수)
    float _PadCluster;     // 정렬 패딩
};

// 타일별 라이트 인덱스 데이터의 시작 오프셋 계산
uint GetTileDataOffset(uint tileIndex)
{
    const uint MaxLightsPerTile = 256;
    return tileIndex * MaxLightsPerTile;
}

// 라이트 개수를 색상으로 변환 (히트맵)
// 0 = 파란색(차가운), 많을수록 빨간색(뜨거운)
float3 LightCountToHeatmap(uint lightCount)
{
    // 정규화 (0~16개를 0.0~1.0으로)
    float normalized = saturate(float(lightCount) / 16.0f);

    // 히트맵 색상 계산 (Blue -> Cyan -> Green -> Yellow -> Red)
    float3 color;
    if (normalized < 0.25f)
    {
        // Blue -> Cyan
        float t = normalized / 0.25f;
        color = lerp(float3(0.0f, 0.0f, 1.0f), float3(0.0f, 1.0f, 1.0f), t);
    }
    else if (normalized < 0.5f)
    {
        // Cyan -> Green
        float t = (normalized - 0.25f) / 0.25f;
        color = lerp(float3(0.0f, 1.0f, 1.0f), float3(0.0f, 1.0f, 0.0f), t);
    }
    else if (normalized < 0.75f)
    {
        // Green -> Yellow
        float t = (normalized - 0.5f) / 0.25f;
        color = lerp(float3(0.0f, 1.0f, 0.0f), float3(1.0f, 1.0f, 0.0f), t);
    }
    else
    {
        // Yellow -> Red
        float t = (normalized - 0.75f) / 0.25f;
        color = lerp(float3(1.0f, 1.0f, 0.0f), float3(1.0f, 0.0f, 0.0f), t);
    }

    return color;
}

//================================================================================================
// Pixel Shader Entry Point
//================================================================================================
float4 mainPS(float4 Pos : SV_Position, float2 TexCoord : TEXCOORD0) : SV_Target
{
    // 원본 씬 색상
    float3 sceneColor = g_SceneTexture.Sample(g_SamplerLinear, TexCoord).rgb;

    if (!bUseTileCulling)
    {
        // 타일 컬링이 비활성화되어 있으면 원본 그대로 출력
        return float4(sceneColor, 1.0f);
    }

    // 현재 픽셀이 속한 타일(XY) 계산
    float2 local = Pos.xy - ViewportRect.xy;
    local = max(local, 0.0f);
    uint tileX = (uint)local.x / TileSize;
    uint tileY = (uint)local.y / TileSize;

    // 모든 Z-슬라이스를 순회하여 최대 라이트 개수 계산 (하이라이트용)
    uint maxLightCount = 0u;
    uint tilesPerSlice = TileCountX * TileCountY;
    uint xyIndex = tileY * TileCountX + tileX;
    [loop]
    for (uint z = 0u; z < max(ClusterCountZ, 1u); ++z)
    {
        uint clusterIndex = z * tilesPerSlice + xyIndex;
        uint tileDataOffset = GetTileDataOffset(clusterIndex);
        uint lightCount = g_TileLightIndices[tileDataOffset];
        maxLightCount = max(maxLightCount, lightCount);
    }

    // 히트맵 색상 계산
    float3 heatmapColor = LightCountToHeatmap(maxLightCount);

    // 타일 경계선 그리기 (선택적)
    float2 tileLocalPos = fmod(Pos.xy, float(TileSize));
    bool isBorder = (tileLocalPos.x < 1.0f || tileLocalPos.y < 1.0f);

    // 원본 씬과 히트맵을 블렌딩 (50% 투명도)
    float3 finalColor = lerp(sceneColor, heatmapColor, 0.25f);

    // 경계선은 흰색으로 표시
    if (isBorder)
    {
        finalColor = lerp(finalColor, float3(1.0f, 1.0f, 1.0f), 0.3f);
    }

    return float4(finalColor, 1.0f);
}
