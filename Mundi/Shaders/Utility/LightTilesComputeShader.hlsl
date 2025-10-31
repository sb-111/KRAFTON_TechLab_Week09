//================================================================================================
// Filename:      LightTilesComputeShader.hlsl
// Description:   타일 기반 라이트 컬링용 컴퓨트 셰이더 (Point/Spot)
//                CPU 컬러와 동일한 인덱스 레이아웃으로 결과를 출력합니다.
// Layout (per-tile T):
//   [T * MaxLightsPerTile + 0]     = LightCount
//   [T * MaxLightsPerTile + 1 .. ] = PackedIndex (hi16=type:0=Point,1=Spot, lo16=index)
//================================================================================================

#include "../Common/LightStructures.hlsl"

// ---------------- 스레드 그룹 크기 ----------------
#define LIGHT_TILE_GROUP_SIZE_X 8
#define LIGHT_TILE_GROUP_SIZE_Y 8
#define LIGHT_TILE_GROUP_SIZE_Z 1
// ---------------- 상수 버퍼 (CS) ----------------
// InvViewProj: NDC -> World 역투영에 사용
cbuffer ViewProjCS : register(b0)
{
    row_major float4x4 gInvViewProj;
}

// 타일 파라미터 및 제한치
cbuffer TileParamsCS : register(b1)
{
    uint gTileSize;          // 픽셀 단위 타일 크기
    uint gTileCountX;        // 가로 타일 개수
    uint gTileCountY;        // 세로 타일 개수
    uint gMaxLightsPerTile;  // CPU와 동일해야 함 (256 권장)
}

// 클러스터 Z 슬라이스 파라미터 (로그 분할)
cbuffer ClusterParamsCS : register(b4)
{
    uint  gClusterCountZ;    // Z 슬라이스 개수 (>=1)
    float gClusterNearZ;     // View-space Near (양수)
    float gClusterFarZ;      // View-space Far  (양수)
    float _PadClusterParams; // 정렬 패딩
}

// 카메라 속성 (월드 공간)
cbuffer CameraCS : register(b5)
{
    float3 gCameraPosition;
    float  _PadCamera0;
    float3 gCameraForward;   // 정규화된 카메라 전방 (월드)
    float  _PadCamera1;
}

// 라이트 개수
cbuffer LightCountsCS : register(b2)
{
    uint gPointLightCount;
    uint gSpotLightCount;
    uint2 _padLC;
}

// Viewport rectangle (xy = TopLeft, zw = Size), in pixels
cbuffer ViewportCS : register(b3)
{
    float4 gViewportRect;
}

// ---------------- 리소스 ----------------
StructuredBuffer<FPointLightInfo> g_PointLightListCS : register(t0);
StructuredBuffer<FSpotLightInfo>  g_SpotLightListCS  : register(t1);

// 출력 UAV (타일별 인덱스)
RWStructuredBuffer<uint> g_TileLightIndicesUAV : register(u0);

// ---------------- 유틸/수학 헬퍼 ----------------
struct Plane { float3 Normal; float Distance; }; // 평면: N·X + D = 0
struct FrustumCS
{
    Plane Left;
    Plane Right;
    Plane Top;
    Plane Bottom;
    Plane NearPlane;
    Plane FarPlane;
};

// 전역 상수
static const float Epsilon = 1e-6f;

Plane MakePlane(float3 P0, float3 P1, float3 P2)
{
    float3 Edge1 = P1 - P0;
    float3 Edge2 = P2 - P0;
    float3 Normal = normalize(cross(Edge1, Edge2));
    Plane Out; Out.Normal = Normal; Out.Distance = -dot(Normal, P0);
    return Out;
}

bool SphereIntersectsFrustum(float3 Center, float Radius, FrustumCS Frustum)
{
    float SignedDistance;
    SignedDistance = dot(Frustum.Left.Normal,      Center) + Frustum.Left.Distance;      if (SignedDistance < -Radius) return false;
    SignedDistance = dot(Frustum.Right.Normal,     Center) + Frustum.Right.Distance;     if (SignedDistance < -Radius) return false;
    SignedDistance = dot(Frustum.Top.Normal,       Center) + Frustum.Top.Distance;       if (SignedDistance < -Radius) return false;
    SignedDistance = dot(Frustum.Bottom.Normal,    Center) + Frustum.Bottom.Distance;    if (SignedDistance < -Radius) return false;
    SignedDistance = dot(Frustum.NearPlane.Normal, Center) + Frustum.NearPlane.Distance; if (SignedDistance < -Radius) return false;
    SignedDistance = dot(Frustum.FarPlane.Normal,  Center) + Frustum.FarPlane.Distance;  if (SignedDistance < -Radius) return false;
    return true;
}

// 슬라이스 경계(view-space z)를 계산 (로그 분할)
float ComputeSliceZ(uint SliceIndex, uint SliceCount, float NearZ, float FarZ)
{
    SliceCount = max(SliceCount, 1u);
    float T = saturate((float)SliceIndex / (float)SliceCount);
    // log2 공간에서 선형 보간 후 exp2로 역변환 (NearZ, FarZ는 >0 가정)
    float LogNear = log2(max(NearZ, Epsilon));
    float LogFar  = log2(max(FarZ, NearZ + Epsilon));
    float LogZ = lerp(LogNear, LogFar, T);
    return exp2(LogZ);
}

FrustumCS BuildTileFrustum(uint TileX, uint TileY, uint TileZ)
{
    // 1) Screen-space tile bounds (absolute pixels, compensate viewport offset)
    float AbsMinX = gViewportRect.x + float(TileX * gTileSize);
    float AbsMaxX = gViewportRect.x + float((TileX + 1) * gTileSize);
    float AbsMinY = gViewportRect.y + float(TileY * gTileSize);
    float AbsMaxY = gViewportRect.y + float((TileY + 1) * gTileSize);

    float ViewportWidth  = gViewportRect.z;
    float ViewportHeight = gViewportRect.w;

    // 2) Screen -> NDC (DirectX: +Y up), normalize within the viewport
    float NdcMinX = ((AbsMinX - gViewportRect.x) / ViewportWidth)  * 2.0f - 1.0f;
    float NdcMaxX = ((AbsMaxX - gViewportRect.x) / ViewportWidth)  * 2.0f - 1.0f;
    float NdcMinY = 1.0f - ((AbsMaxY - gViewportRect.y) / ViewportHeight) * 2.0f; // flip Y
    float NdcMaxY = 1.0f - ((AbsMinY - gViewportRect.y) / ViewportHeight) * 2.0f;

    // 3) NDC frustum corners
    float4 NdcCorners[8];
    // Near
    NdcCorners[0] = float4(NdcMinX, NdcMinY, 0.0f, 1.0f);
    NdcCorners[1] = float4(NdcMaxX, NdcMinY, 0.0f, 1.0f);
    NdcCorners[2] = float4(NdcMaxX, NdcMaxY, 0.0f, 1.0f);
    NdcCorners[3] = float4(NdcMinX, NdcMaxY, 0.0f, 1.0f);
    // Far
    NdcCorners[4] = float4(NdcMinX, NdcMinY, 1.0f, 1.0f);
    NdcCorners[5] = float4(NdcMaxX, NdcMinY, 1.0f, 1.0f);
    NdcCorners[6] = float4(NdcMaxX, NdcMaxY, 1.0f, 1.0f);
    NdcCorners[7] = float4(NdcMinX, NdcMaxY, 1.0f, 1.0f);

    // 4) Transform to world space
    float3 WorldCorners[8];
    [unroll]
    for (int Index = 0; Index < 8; ++Index)
    {
        float4 World = mul(NdcCorners[Index], gInvViewProj);
        WorldCorners[Index] = World.xyz / max(World.w, Epsilon);
    }

    // 5) Build planes (outward normals)
    FrustumCS Result;
    // XY 측면은 슬라이스와 무관 (타일의 좌우/상하 경계)
    Result.Left      = MakePlane(WorldCorners[0], WorldCorners[3], WorldCorners[7]); // Left  (0,3,7)
    Result.Right     = MakePlane(WorldCorners[1], WorldCorners[5], WorldCorners[6]); // Right (1,5,6)
    Result.Bottom    = MakePlane(WorldCorners[0], WorldCorners[1], WorldCorners[5]); // Bottom(0,1,5)
    Result.Top       = MakePlane(WorldCorners[2], WorldCorners[3], WorldCorners[7]); // Top   (2,3,7)

    // Z 슬라이스 경계를 view-space 깊이로 계산
    uint SliceCount = max(gClusterCountZ, 1u);
    float SliceNearZ = ComputeSliceZ(min(TileZ, SliceCount - 1u), SliceCount, gClusterNearZ, gClusterFarZ);
    float SliceFarZ  = ComputeSliceZ(min(TileZ + 1u, SliceCount),   SliceCount, gClusterNearZ, gClusterFarZ);

    // 월드 공간에서 Near/Far 평면 구성 (카메라 전방에 수직)
    // 평면 식: N·X + d = 0, N = gCameraForward, 점 P = C + N * Z
    float3 N = normalize(gCameraForward);
    float3 Pn = gCameraPosition + N * SliceNearZ;
    float3 Pf = gCameraPosition + N * SliceFarZ;
    Plane NearP; NearP.Normal =  N; NearP.Distance = -dot(N, Pn);
    Plane FarP;  FarP.Normal = -N; FarP.Distance  = -dot(-N, Pf); // 바깥쪽을 향하도록 반전
    Result.NearPlane = NearP;
    Result.FarPlane  = FarP;

    return Result;
}

uint PackLightIndex(uint type01, uint index16)
{
    return (type01 << 16) | (index16 & 0xFFFFu);
}

// ---------------- Main (8x8x1) ----------------
[numthreads(LIGHT_TILE_GROUP_SIZE_X, LIGHT_TILE_GROUP_SIZE_Y, LIGHT_TILE_GROUP_SIZE_Z)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint TileX = DispatchThreadID.x;
    const uint TileY = DispatchThreadID.y;
    const uint TileZ = DispatchThreadID.z;
    const uint ClusterCountZ = max(gClusterCountZ, 1u);
    if (TileX >= gTileCountX || TileY >= gTileCountY || TileZ >= ClusterCountZ) return;

    const uint ClusterIndex = TileZ * (gTileCountX * gTileCountY) + (TileY * gTileCountX + TileX);
    const uint BaseOffset = ClusterIndex * gMaxLightsPerTile;

    // 초기화
    g_TileLightIndicesUAV[BaseOffset] = 0u;
    uint LightCount = 0u;

    // 타일 + Z슬라이스 프러스텀 구성
    FrustumCS fr = BuildTileFrustum(TileX, TileY, TileZ);

    // 전역 라이트(Ambient/Directional)는 타일 컬링 대상에서 제외 (Point/Spot만 기록)

    // Point (type=0)
    [loop]
    for (uint LightIndex = 0; LightIndex < gPointLightCount && LightCount < (gMaxLightsPerTile - 1u); ++LightIndex)
    {
        FPointLightInfo Light = g_PointLightListCS[LightIndex];
        if (SphereIntersectsFrustum(Light.Position, Light.AttenuationRadius, fr))
        {
            g_TileLightIndicesUAV[BaseOffset + 1u + LightCount] = PackLightIndex(0u, LightIndex);
            ++LightCount;
        }
    }

    // Spot (type=1)
    [loop]
    for (uint LightIndex = 0; LightIndex < gSpotLightCount && LightCount < (gMaxLightsPerTile - 1u); ++LightIndex)
    {
        FSpotLightInfo Light = g_SpotLightListCS[LightIndex];
        if (SphereIntersectsFrustum(Light.Position, Light.AttenuationRadius, fr))
        {
            g_TileLightIndicesUAV[BaseOffset + 1u + LightCount] = PackLightIndex(1u, LightIndex);
            ++LightCount;
        }
    }

    // 최종 개수 기록
    g_TileLightIndicesUAV[BaseOffset] = LightCount;
}
