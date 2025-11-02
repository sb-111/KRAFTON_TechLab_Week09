#pragma once

#include <d3d11.h>
#include <dxgi.h>

class UStatsOverlayD2D
{
public:
    static UStatsOverlayD2D& Get();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain);
	void Shutdown();
    void Draw();

    void SetShowFPS(bool b);
    void SetShowMemory(bool b);
    void SetShowPicking(bool b);
    void SetShowDecal(bool b);
    void SetShowTileCulling(bool b);
	void SetShowShadowInfo(bool b);
    void SetShowPhysics(bool b);
	void ToggleFPS();
    void ToggleMemory();
    void TogglePicking();
    void ToggleDecal();
    void ToggleTileCulling();
	void ToggleShadowInfo();
    void TogglePhysics();
    bool IsFPSVisible() const { return bShowFPS; }
    bool IsMemoryVisible() const { return bShowMemory; }
    bool IsPickingVisible() const { return bShowPicking; }
    bool IsDecalVisible() const { return bShowDecal; }
    bool IsTileCullingVisible() const { return bShowTileCulling; }
	bool IsShadowInfoVisible() const { return bShowShadowInfo; }
    bool IsPhysicsVisible() const { return bShowPhysics; }

private:
    UStatsOverlayD2D() = default;
    ~UStatsOverlayD2D() = default;
    UStatsOverlayD2D(const UStatsOverlayD2D&) = delete;
    UStatsOverlayD2D& operator=(const UStatsOverlayD2D&) = delete;

    void EnsureInitialized();
    void ReleaseD2DTarget();

private:
    bool bInitialized = false;
    bool bShowFPS = true;
    bool bShowMemory = true;
    bool bShowPicking = false;
    bool bShowDecal = false;
    bool bShowTileCulling = false;
	bool bShowShadowInfo = true;
    bool bShowPhysics = false;

    ID3D11Device* D3DDevice = nullptr;
    ID3D11DeviceContext* D3DContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
};
