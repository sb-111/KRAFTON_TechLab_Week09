#pragma once
#include "LightManager.h"
#include "TileCullingStats.h"
#include "D3D11RHI.h"
#include "Frustum.h"

// 타일 기반 라이트 컬링을 Compute Shader로 수행하는 클래스
// 결과를 Structured Buffer(SRV)로 노출하여 픽셀 셰이더(UberLit)에서 사용
class FTileLightCuller
{
public:
	FTileLightCuller();
	~FTileLightCuller();

	// 초기화 (셰이더/상수버퍼 등 생성)
	void Initialize(D3D11RHI* InRHI, UINT InTileSize = 16);

	// 타일 컬링 수행 (매 프레임 호출) - Compute Shader 경로
    void CullLights(
        ID3D11ShaderResourceView* PointLightSRV,
        ID3D11ShaderResourceView* SpotLightSRV,
        uint32 PointLightCount,
        uint32 SpotLightCount,
        const FMatrix& ViewMatrix,
        const FMatrix& ProjMatrix,
        float ZNear,
        float ZFar,
        UINT ViewportWidth,
        UINT ViewportHeight);

	// 컬링 결과를 Structured Buffer에 업데이트하고 SRV 반환
	ID3D11ShaderResourceView* GetLightIndexBufferSRV();

	// 통계 정보 반환
	const FTileCullingStats& GetStats() const { return Stats; }

	// 리소스 해제
	void Release();

private:
    D3D11RHI* RHI;

	// 타일 설정
    UINT TileSize;          // 타일 크기 (픽셀, 기본값 16)
    UINT TileCountX;        // 가로 타일 개수
    UINT TileCountY;        // 세로 타일 개수
    UINT TotalTileCount;    // 전체 타일 개수

	// 타일당 최대 라이트 개수 (CPU/GPU 동일)
    static constexpr UINT MaxLightsPerTile = 256;

	// GPU 리소스 (타일 인덱스 버퍼)
    ID3D11Buffer* LightIndexBuffer = nullptr;
    ID3D11ShaderResourceView* LightIndexBufferSRV = nullptr;
    ID3D11UnorderedAccessView* LightIndexBufferUAV = nullptr;
    UINT AllocatedElements = 0; // NumTiles * MaxLightsPerTile

	// CS 리소스
    ID3D11ComputeShader* TileCullingCS = nullptr;
    ID3D11Buffer* CS_CB0_InvViewProj = nullptr; // row_major float4x4
    ID3D11Buffer* CS_CB1_TileParams = nullptr;  // uint4(TileSize, CountX, CountY, MaxLightsPerTile)
    ID3D11Buffer* CS_CB2_LightCounts = nullptr; // uint4(PointCount, SpotCount, 0, 0)
    ID3D11Buffer* CS_CB3_Viewport = nullptr;    // float4(TopLeftX, TopLeftY, Width, Height)
    ID3D11Buffer* CS_CB4_Cluster = nullptr;     // uint, float, float, pad
    ID3D11Buffer* CS_CB5_Camera = nullptr;      // float3 Pos + pad, float3 Fwd + pad

    // 통계
    FTileCullingStats Stats;

    // 클러스터링 파라미터
    UINT ClusterCountZ = 1;     // 기본 1 (2D 타일)
    float ClusterNearZ = 0.1f;  // View-space Near
    float ClusterFarZ  = 1000.0f; // View-space Far

private:
    // 내부 헬퍼들 (가독성 향상)
    void CreateCSIfNeeded();
    void CreateOrResizeOutputBuffer(UINT RequiredElements);
    void CreateCSConstantBuffersIfNeeded();
    void UpdateCSCB_InvViewProj(const FMatrix& ViewMatrix, const FMatrix& ProjMatrix);
    void UpdateCSCB_TileParams(uint32 InTileSize, uint32 InTileCountX, uint32 InTileCountY);
    void UpdateCSCB_LightCounts(uint32 PointLightCount, uint32 SpotLightCount);
    void UpdateCSCB_ViewportRect();
    void UpdateCSCB_ClusterParams(float NearZ, float FarZ, uint32 InClusterCountZ);
    void UpdateCSCB_Camera(const FMatrix& ViewMatrix);
    void DispatchCS(ID3D11ShaderResourceView* PointLightSRV, ID3D11ShaderResourceView* SpotLightSRV);
};
