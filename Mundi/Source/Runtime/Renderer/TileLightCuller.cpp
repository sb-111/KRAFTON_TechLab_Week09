#include "pch.h"
#include "TileLightCuller.h"

FTileLightCuller::FTileLightCuller()
    : RHI(nullptr)
    , TileSize(16)
    , TileCountX(0)
    , TileCountY(0)
    , TotalTileCount(0)
{
}

FTileLightCuller::~FTileLightCuller()
{
    Release();
}

void FTileLightCuller::Initialize(D3D11RHI* InRHI, UINT InTileSize)
{
    RHI = InRHI;
    TileSize = InTileSize;
    // CS, UAV/SRV는 실제 사용 시점에 lazy-creation
}

void FTileLightCuller::CullLights(
    ID3D11ShaderResourceView* PointLightSRV,
    ID3D11ShaderResourceView* SpotLightSRV,
    uint32 PointLightCount,
    uint32 SpotLightCount,
    const FMatrix& ViewMatrix,
    const FMatrix& ProjMatrix,
    float ZNear,
    float ZFar,
    UINT ViewportWidth,
    UINT ViewportHeight)
{
    if (!RHI) return;

    TileCountX = (ViewportWidth + TileSize - 1) / TileSize;
    TileCountY = (ViewportHeight + TileSize - 1) / TileSize;
    // 클러스터 파라미터 (SceneView와 동일 값 사용)
    ClusterCountZ = 16; // TODO: RenderSettings로 노출
    ClusterNearZ = std::max(ZNear, 0.0001f);
    ClusterFarZ  = std::max(ZFar, ClusterNearZ + 1.0f);
    TotalTileCount = TileCountX * TileCountY * ClusterCountZ;

    const UINT RequiredElements = TotalTileCount * MaxLightsPerTile;
    CreateOrResizeOutputBuffer(RequiredElements);

    CreateCSIfNeeded();
    if (!TileCullingCS)
        return;

    CreateCSConstantBuffersIfNeeded();
    UpdateCSCB_InvViewProj(ViewMatrix, ProjMatrix);
    UpdateCSCB_TileParams(TileSize, TileCountX, TileCountY);
    UpdateCSCB_LightCounts(PointLightCount, SpotLightCount);
    UpdateCSCB_ViewportRect();
    UpdateCSCB_ClusterParams(ClusterNearZ, ClusterFarZ, ClusterCountZ);
    UpdateCSCB_Camera(ViewMatrix);

    DispatchCS(PointLightSRV, SpotLightSRV);
}

ID3D11ShaderResourceView* FTileLightCuller::GetLightIndexBufferSRV()
{
    return LightIndexBufferSRV;
}

void FTileLightCuller::Release()
{
    if (LightIndexBufferUAV) { LightIndexBufferUAV->Release(); LightIndexBufferUAV = nullptr; }
    if (LightIndexBufferSRV) { LightIndexBufferSRV->Release(); LightIndexBufferSRV = nullptr; }
    if (LightIndexBuffer)    { LightIndexBuffer->Release();    LightIndexBuffer = nullptr; }

    if (TileCullingCS) { TileCullingCS->Release(); TileCullingCS = nullptr; }
    if (CS_CB0_InvViewProj) { CS_CB0_InvViewProj->Release(); CS_CB0_InvViewProj = nullptr; }
    if (CS_CB1_TileParams) { CS_CB1_TileParams->Release(); CS_CB1_TileParams = nullptr; }
    if (CS_CB2_LightCounts) { CS_CB2_LightCounts->Release(); CS_CB2_LightCounts = nullptr; }
    if (CS_CB3_Viewport) { CS_CB3_Viewport->Release(); CS_CB3_Viewport = nullptr; }
    if (CS_CB4_Cluster) { CS_CB4_Cluster->Release(); CS_CB4_Cluster = nullptr; }
    if (CS_CB5_Camera) { CS_CB5_Camera->Release(); CS_CB5_Camera = nullptr; }
}

// ================================
// 내부 헬퍼 구현
// ================================

void FTileLightCuller::CreateCSIfNeeded()
{
    if (TileCullingCS) return;

    ID3DBlob* blob = nullptr; ID3DBlob* err = nullptr;
    UINT flags = 0; // D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    HRESULT hr = D3DCompileFromFile(L"Shaders/Utility/LightTilesComputeShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main", "cs_5_0", flags, 0, &blob, &err);
    if (SUCCEEDED(hr))
    {
        RHI->GetDevice()->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &TileCullingCS);
        blob->Release();
    }
    if (err) { err->Release(); }
}

void FTileLightCuller::CreateOrResizeOutputBuffer(UINT RequiredElements)
{
    if (AllocatedElements == RequiredElements && LightIndexBuffer && LightIndexBufferSRV && LightIndexBufferUAV)
        return;

    if (LightIndexBufferUAV) { LightIndexBufferUAV->Release(); LightIndexBufferUAV = nullptr; }
    if (LightIndexBufferSRV) { LightIndexBufferSRV->Release(); LightIndexBufferSRV = nullptr; }
    if (LightIndexBuffer)    { LightIndexBuffer->Release();    LightIndexBuffer = nullptr; }

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = RequiredElements * sizeof(uint32);
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(uint32);
    HRESULT hr = RHI->GetDevice()->CreateBuffer(&desc, nullptr, &LightIndexBuffer);
    if (FAILED(hr)) return;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = DXGI_FORMAT_UNKNOWN;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvd.Buffer.FirstElement = 0;
    srvd.Buffer.NumElements = RequiredElements;
    RHI->GetDevice()->CreateShaderResourceView(LightIndexBuffer, &srvd, &LightIndexBufferSRV);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavd = {};
    uavd.Format = DXGI_FORMAT_UNKNOWN;
    uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavd.Buffer.FirstElement = 0;
    uavd.Buffer.NumElements = RequiredElements;
    RHI->GetDevice()->CreateUnorderedAccessView(LightIndexBuffer, &uavd, &LightIndexBufferUAV);

    AllocatedElements = RequiredElements;
    Stats.LightIndexBufferSizeBytes = RequiredElements * sizeof(uint32);
}

void FTileLightCuller::CreateCSConstantBuffersIfNeeded()
{
    if (!CS_CB0_InvViewProj)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(FMatrix);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        RHI->GetDevice()->CreateBuffer(&bd, nullptr, &CS_CB0_InvViewProj);
    }
    if (!CS_CB1_TileParams)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(uint32) * 4;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        RHI->GetDevice()->CreateBuffer(&bd, nullptr, &CS_CB1_TileParams);
    }
    if (!CS_CB2_LightCounts)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(uint32) * 4;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        RHI->GetDevice()->CreateBuffer(&bd, nullptr, &CS_CB2_LightCounts);
    }
    if (!CS_CB3_Viewport)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(float) * 4;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        RHI->GetDevice()->CreateBuffer(&bd, nullptr, &CS_CB3_Viewport);
    }
    if (!CS_CB4_Cluster)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(float) * 4; // uint, uint/float align
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        RHI->GetDevice()->CreateBuffer(&bd, nullptr, &CS_CB4_Cluster);
    }
    if (!CS_CB5_Camera)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = sizeof(float) * 8; // pos(3)+pad, fwd(3)+pad
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        RHI->GetDevice()->CreateBuffer(&bd, nullptr, &CS_CB5_Camera);
    }
}

void FTileLightCuller::UpdateCSCB_InvViewProj(const FMatrix& ViewMatrix, const FMatrix& ProjMatrix)
{
    FMatrix InvView = ViewMatrix.InverseAffine();
    FMatrix InvProj = ProjMatrix.InversePerspectiveProjection();
    FMatrix InvViewProj = InvProj * InvView;

    D3D11_MAPPED_SUBRESOURCE msr = {};
    if (SUCCEEDED(RHI->GetDeviceContext()->Map(CS_CB0_InvViewProj, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
    {
        memcpy(msr.pData, &InvViewProj, sizeof(FMatrix));
        RHI->GetDeviceContext()->Unmap(CS_CB0_InvViewProj, 0);
    }
}

void FTileLightCuller::UpdateCSCB_TileParams(uint32 InTileSize, uint32 InTileCountX, uint32 InTileCountY)
{
    struct { uint32 a,b,c,d; } TileParams = { InTileSize, InTileCountX, InTileCountY, MaxLightsPerTile };

    D3D11_MAPPED_SUBRESOURCE msr = {};
    if (SUCCEEDED(RHI->GetDeviceContext()->Map(CS_CB1_TileParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
    {
        memcpy(msr.pData, &TileParams, sizeof(TileParams));
        RHI->GetDeviceContext()->Unmap(CS_CB1_TileParams, 0);
    }
}

void FTileLightCuller::UpdateCSCB_LightCounts(uint32 PointLightCount, uint32 SpotLightCount)
{
    struct { uint32 p,s; uint32 pad[2]; } LightCounts = { PointLightCount, SpotLightCount, {0,0} };

    D3D11_MAPPED_SUBRESOURCE msr = {};
    if (SUCCEEDED(RHI->GetDeviceContext()->Map(CS_CB2_LightCounts, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
    {
        memcpy(msr.pData, &LightCounts, sizeof(LightCounts));
        RHI->GetDeviceContext()->Unmap(CS_CB2_LightCounts, 0);
    }
}

void FTileLightCuller::UpdateCSCB_ViewportRect()
{
    D3D11_VIEWPORT vp = {};
    UINT num = 1;
    RHI->GetDeviceContext()->RSGetViewports(&num, &vp);
    struct { float x,y,w,h; } VP = { vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height };

    D3D11_MAPPED_SUBRESOURCE msr = {};
    if (SUCCEEDED(RHI->GetDeviceContext()->Map(CS_CB3_Viewport, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
    {
        memcpy(msr.pData, &VP, sizeof(VP));
        RHI->GetDeviceContext()->Unmap(CS_CB3_Viewport, 0);
    }
}

void FTileLightCuller::UpdateCSCB_ClusterParams(float NearZ, float FarZ, uint32 InClusterCountZ)
{
    struct { uint32 CountZ; float NearZ; float FarZ; float Pad; } Params = { InClusterCountZ, NearZ, FarZ, 0.0f };
    D3D11_MAPPED_SUBRESOURCE msr = {};
    if (SUCCEEDED(RHI->GetDeviceContext()->Map(CS_CB4_Cluster, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
    {
        memcpy(msr.pData, &Params, sizeof(Params));
        RHI->GetDeviceContext()->Unmap(CS_CB4_Cluster, 0);
    }
}

void FTileLightCuller::UpdateCSCB_Camera(const FMatrix& ViewMatrix)
{
    // World-space camera position and forward
    const FMatrix InvView = ViewMatrix.InverseAffine();
    const FVector CamPos = (FVector4::FromPoint(FVector(0,0,0)) * InvView).ToVector3();
    const FVector CamFwd = (FVector4::FromDirection(FVector(0,0,1)) * InvView).ToVector3().GetSafeNormal();
    struct { FVector Position; float _pad0; FVector Forward; float _pad1; } Cam = { CamPos, 0.0f, CamFwd, 0.0f };
    D3D11_MAPPED_SUBRESOURCE msr = {};
    if (SUCCEEDED(RHI->GetDeviceContext()->Map(CS_CB5_Camera, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
    {
        memcpy(msr.pData, &Cam, sizeof(Cam));
        RHI->GetDeviceContext()->Unmap(CS_CB5_Camera, 0);
    }
}

void FTileLightCuller::DispatchCS(ID3D11ShaderResourceView* PointLightSRV, ID3D11ShaderResourceView* SpotLightSRV)
{
    if (!TileCullingCS || !LightIndexBufferUAV || !PointLightSRV || !SpotLightSRV)
        return;

    ID3D11DeviceContext* ctx = RHI->GetDeviceContext();
    // Pre-clear possible SRV bindings of the same resource in PS (t8/t2) to avoid UAV hazard
    ID3D11ShaderResourceView* nullPS = nullptr;
    ctx->PSSetShaderResources(8, 1, &nullPS);
    ctx->PSSetShaderResources(2, 1, &nullPS);
    ctx->CSSetShader(TileCullingCS, nullptr, 0);
    ID3D11Buffer* CBs[6] = { CS_CB0_InvViewProj, CS_CB1_TileParams, CS_CB2_LightCounts, CS_CB3_Viewport, CS_CB4_Cluster, CS_CB5_Camera };
    ctx->CSSetConstantBuffers(0, 6, CBs);
    ID3D11ShaderResourceView* SRVs[2] = { PointLightSRV, SpotLightSRV };
    ctx->CSSetShaderResources(0, 2, SRVs);
    ctx->CSSetUnorderedAccessViews(0, 1, &LightIndexBufferUAV, nullptr);

    const UINT groupX = (TileCountX + 7) / 8;
    const UINT groupY = (TileCountY + 7) / 8;
    ctx->Dispatch(groupX, groupY, ClusterCountZ);

    // 언바인드
    ID3D11UnorderedAccessView* nullUAV = nullptr;
    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
    ctx->CSSetShaderResources(0, 2, nullSRVs);
}
