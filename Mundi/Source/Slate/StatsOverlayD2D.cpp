#include "pch.h"

#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_2.h>

#include "StatsOverlayD2D.h"
#include "UIManager.h"
#include "RenderManager.h"
#include "Renderer.h"
#include "ShadowSystem.h"
#include "CSM.h"
#include "MemoryManager.h"
#include "Picking.h"
#include "PlatformTime.h"
#include "DecalStatManager.h"
#include "TileCullingStats.h"
#include "World.h"
#include "WorldPhysics.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

static inline void SafeRelease(IUnknown* p) { if (p) p->Release(); }

UStatsOverlayD2D& UStatsOverlayD2D::Get()
{
	static UStatsOverlayD2D Instance;
	return Instance;
}

void UStatsOverlayD2D::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InContext, IDXGISwapChain* InSwapChain)
{
	D3DDevice = InDevice;
	D3DContext = InContext;
	SwapChain = InSwapChain;
	bInitialized = (D3DDevice && D3DContext && SwapChain);
}

void UStatsOverlayD2D::Shutdown()
{
	D3DDevice = nullptr;
	D3DContext = nullptr;
	SwapChain = nullptr;
	bInitialized = false;
}

void UStatsOverlayD2D::EnsureInitialized()
{
}

void UStatsOverlayD2D::ReleaseD2DTarget()
{
}

static void DrawTextBlock(
	ID2D1DeviceContext* InD2dCtx,
	IDWriteFactory* InDwrite,
	const wchar_t* InText,
	const D2D1_RECT_F& InRect,
	float InFontSize,
	D2D1::ColorF InBgColor,
	D2D1::ColorF InTextColor)
{
	if (!InD2dCtx || !InDwrite || !InText) return;

	ID2D1SolidColorBrush* BrushFill = nullptr;
	InD2dCtx->CreateSolidColorBrush(InBgColor, &BrushFill);

	ID2D1SolidColorBrush* BrushText = nullptr;
	InD2dCtx->CreateSolidColorBrush(InTextColor, &BrushText);

	InD2dCtx->FillRectangle(InRect, BrushFill);

	IDWriteTextFormat* Format = nullptr;
	InDwrite->CreateTextFormat(
		L"Segoe UI",
		nullptr,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		InFontSize,
		L"en-us",
		&Format);

	if (Format)
	{
		Format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		Format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		Format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
		InD2dCtx->DrawTextW(
			InText,
			static_cast<UINT32>(wcslen(InText)),
			Format,
			InRect,
			BrushText);
		Format->Release();
	}

	SafeRelease(BrushText);
	SafeRelease(BrushFill);
}

static float CalcPanelHeightForText(IDWriteFactory* Dwrite, const wchar_t* InText, float FontSize, float MaxWidth, float MinHeight)
{
    if (!Dwrite || !InText)
        return MinHeight;

    IDWriteTextFormat* Format = nullptr;
    if (FAILED(Dwrite->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        FontSize, L"en-us", &Format)))
    {
        return MinHeight;
    }
    Format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    Format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		Format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    Format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

    IDWriteTextLayout* Layout = nullptr;
    // Large max height to allow wrapping measurement
    const float MaxHeight = 4096.0f;
    HRESULT hr = Dwrite->CreateTextLayout(
        InText,
        static_cast<UINT32>(wcslen(InText)),
        Format,
        MaxWidth,
        MaxHeight,
        &Layout);

    float Result = MinHeight;
    if (SUCCEEDED(hr) && Layout)
    {
        DWRITE_TEXT_METRICS M{};
        if (SUCCEEDED(Layout->GetMetrics(&M)))
        {
            const float padding = 12.0f; // top+bottom
            const float h = padding + M.height;
            Result = (h < MinHeight) ? MinHeight : h;
        }
    }

    if (Layout) Layout->Release();
    if (Format) Format->Release();
    return Result;
}

void UStatsOverlayD2D::Draw()
{
	if (!bInitialized
		|| (!bShowFPS && !bShowMemory && !bShowPicking && !bShowDecal && !bShowTileCulling && !bShowShadowInfo && !bShowPhysics)
		|| !SwapChain)
		return;

	ID2D1Factory1* D2dFactory = nullptr;
	D2D1_FACTORY_OPTIONS Opts{};
#ifdef _DEBUG
	Opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &Opts, (void**)&D2dFactory)))
		return;

	IDXGISurface* Surface = nullptr;
	if (FAILED(SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&Surface)))
	{
		SafeRelease(D2dFactory);
		return;
	}

	IDXGIDevice* DxgiDevice = nullptr;
	if (FAILED(D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DxgiDevice)))
	{
		SafeRelease(Surface);
		SafeRelease(D2dFactory);
		return;
	}

	ID2D1Device* D2dDevice = nullptr;
	if (FAILED(D2dFactory->CreateDevice(DxgiDevice, &D2dDevice)))
	{
		SafeRelease(DxgiDevice);
		SafeRelease(Surface);
		SafeRelease(D2dFactory);
		return;
	}

	ID2D1DeviceContext* D2dCtx = nullptr;
	if (FAILED(D2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2dCtx)))
	{
		SafeRelease(D2dDevice);
		SafeRelease(DxgiDevice);
		SafeRelease(Surface);
		SafeRelease(D2dFactory);
		return;
	}

	IDWriteFactory* Dwrite = nullptr;
	if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&Dwrite)))
	{
		SafeRelease(D2dCtx);
		SafeRelease(D2dDevice);
		SafeRelease(DxgiDevice);
		SafeRelease(Surface);
		SafeRelease(D2dFactory);
		return;
	}

	D2D1_BITMAP_PROPERTIES1 BmpProps = {};
	BmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	BmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	BmpProps.dpiX = 96.0f;
	BmpProps.dpiY = 96.0f;
	BmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

	ID2D1Bitmap1* TargetBmp = nullptr;
	if (FAILED(D2dCtx->CreateBitmapFromDxgiSurface(Surface, &BmpProps, &TargetBmp)))
	{
		SafeRelease(Dwrite);
		SafeRelease(D2dCtx);
		SafeRelease(D2dDevice);
		SafeRelease(DxgiDevice);
		SafeRelease(Surface);
		SafeRelease(D2dFactory);
		return;
	}

	D2dCtx->SetTarget(TargetBmp);

	D2dCtx->BeginDraw();
	const float Margin = 12.0f;
	const float Space = 8.0f;   // 패널간의 간격
	const float PanelWidth = 250.0f;
	const float PanelHeight = 48.0f;
	float NextY = 70.0f;

	if (bShowFPS)
	{
		float Dt = UUIManager::GetInstance().GetDeltaTime();
		float Fps = Dt > 0.0f ? (1.0f / Dt) : 0.0f;
		float Ms = Dt * 1000.0f;

		wchar_t Buf[128];
		swprintf_s(Buf, L"FPS: %.1f\nFrame time: %.2f ms", Fps, Ms);

		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PanelHeight);
		DrawTextBlock(
			D2dCtx, Dwrite, Buf, rc, 16.0f,
			D2D1::ColorF(0, 0, 0, 0.6f),
			D2D1::ColorF(D2D1::ColorF::Yellow));

		NextY += PanelHeight + Space;
	}

	if (bShowPicking)
	{
		// Build the entire block in one Buffer to avoid overwriting previous lines
		wchar_t Buf[256];
		double LastMs = FWindowsPlatformTime::ToMilliseconds(CPickingSystem::GetLastPickTime());
		double TotalMs = FWindowsPlatformTime::ToMilliseconds(CPickingSystem::GetTotalPickTime());
		uint32 Count = CPickingSystem::GetPickCount();
		double AvgMs = (Count > 0) ? (TotalMs / (double)Count) : 0.0;
		swprintf_s(Buf, L"Pick Count: %u\nLast: %.3f ms\nAvg: %.3f ms\nTotal: %.3f ms", Count, LastMs, AvgMs, TotalMs);

		// Increase panel height to fit multiple lines
		const float PickPanelHeight = 96.0f;
		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PickPanelHeight);
		DrawTextBlock(
			D2dCtx, Dwrite, Buf, rc, 16.0f,
			D2D1::ColorF(0, 0, 0, 0.6f),
			D2D1::ColorF(D2D1::ColorF::SkyBlue));

		NextY += PickPanelHeight + Space;
	}

	if (bShowMemory)
	{
		double Mb = static_cast<double>(CMemoryManager::TotalAllocationBytes) / (1024.0 * 1024.0);

		wchar_t Buf[128];
		swprintf_s(Buf, L"Memory: %.1f MB\nAllocs: %u", Mb, CMemoryManager::TotalAllocationCount);

		D2D1_RECT_F Rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PanelHeight);
		DrawTextBlock(
			D2dCtx, Dwrite, Buf, Rc, 16.0f,
			D2D1::ColorF(0, 0, 0, 0.6f),
			D2D1::ColorF(D2D1::ColorF::LightGreen));

		NextY += PanelHeight + Space;
	}

	if (bShowPhysics)
	{
		int32 ShapeCount = 0;
		int32 NodeCount = 0;
		if (GWorld)
		{
			if (UWorldPhysics* Physics = GWorld->GetWorldPhysics())
			{
				ShapeCount = Physics->GetCollisionShapeCount();
				NodeCount = Physics->GetCollisionNodeCount();
			}
		}

		wchar_t Buf[128];
		swprintf_s(Buf, L"[Physics]\nShape Components: %d\nBVH Nodes: %d", ShapeCount, NodeCount);

		const float PhysicsPanelHeight = 96.0f;
		D2D1_RECT_F Rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + PhysicsPanelHeight);
		DrawTextBlock(
			D2dCtx, Dwrite, Buf, Rc, 16.0f,
			D2D1::ColorF(0, 0, 0, 0.6f),
			D2D1::ColorF(D2D1::ColorF::LightSteelBlue));

		NextY += PhysicsPanelHeight + Space;
	}

	if (bShowDecal)
	{
		// 1. FDecalStatManager로부터 통계 데이터를 가져옵니다.
		uint32_t TotalCount = FDecalStatManager::GetInstance().GetTotalDecalCount();
		//uint32_t VisibleDecalCount = FDecalStatManager::GetInstance().GetVisibleDecalCount();
		uint32_t AffectedMeshCount = FDecalStatManager::GetInstance().GetAffectedMeshCount();
		double TotalTime = FDecalStatManager::GetInstance().GetDecalPassTimeMS();
		double AverageTimePerDecal = FDecalStatManager::GetInstance().GetAverageTimePerDecalMS();
		double AverageTimePerDraw = FDecalStatManager::GetInstance().GetAverageTimePerDrawMS();

		// 2. 출력할 문자열 버퍼를 만듭니다.
		wchar_t Buf[256];
		swprintf_s(Buf, L"[Decal Stats]\nTotal: %u\nAffectedMesh: %u\n전체 소요 시간: %.3f ms\nAvg/Decal: %.3f ms\nAvg/Mesh: %.3f ms",
			TotalCount,
			AffectedMeshCount,
			TotalTime,
			AverageTimePerDecal,
			AverageTimePerDraw);

		// 3. 텍스트를 여러 줄 표시해야 하므로 패널 높이를 늘립니다.
		const float decalPanelHeight = 140.0f;
		D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + decalPanelHeight);

		// 4. DrawTextBlock 함수를 호출하여 화면에 그립니다. 색상은 구분을 위해 주황색(Orange)으로 설정합니다.
		DrawTextBlock(
			D2dCtx, Dwrite, Buf, rc, 16.0f,
			D2D1::ColorF(0, 0, 0, 0.6f),
			D2D1::ColorF(D2D1::ColorF::Orange));

		NextY += decalPanelHeight + Space;
	}

    if (bShowTileCulling)
    {
        // LIGHT: 섀도우 텍스처 기준 메모리/개수 표시
        FLightManager* LightManager = GWorld->GetLightManager();
        URenderer* Renderer = URenderManager::GetInstance().GetRenderer();
        FShadowSystem* ShadowSys = Renderer ? Renderer->GetShadowSystem() : nullptr;
        FCSM* CSM = Renderer ? Renderer->GetCSMSystem() : nullptr;

        const uint32 PointLightCount = LightManager ? LightManager->GetPointLightCount() : 0;
        const uint32 SpotLightCount  = LightManager ? LightManager->GetSpotLightCount()  : 0;
        const uint32 DirectionalCount = (LightManager && LightManager->GetDirectionalLight()) ? 1u : 0u;

        // 현재 프레임 실제 섀도우 사용 개수(후보)
        const uint32 UsedSpotShadow  = ShadowSys ? ShadowSys->GetNumSpotShadow()  : 0u;
        const uint32 UsedPointShadow = ShadowSys ? ShadowSys->GetNumPointShadow() : 0u;
        const uint32 UsedDirShadow   = DirectionalCount; // CSM 사용 여부는 Directional 존재로 간주

        // 해상도
        const uint32 SpotRes  = ShadowSys ? ShadowSys->GetSpotShadowTextureResolution() : 0u;
        const uint32 PointRes = ShadowSys ? ShadowSys->GetPointShadowTextureResolution() : 0u;
        const uint32 DirRes   = CSM ? CSM->GetShadowSize() : 0u;

        // 포맷별 BPP
        const uint32 BppDepth32 = 4;           // D32 / R32
        const uint32 BppR16G16F = 4;           // 32-bit total
        const uint32 BppD24S8   = 4;           // R24G8 typeless as 32-bit total

        // 메모리 계산 (바이트)
        auto Bytes2D = [](uint32 w, uint32 h, uint32 bpp) -> size_t { return static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(bpp); };

        // Spot: depth + VSM (array slices = UsedSpotShadow)
        size_t SpotDepthBytes = Bytes2D(SpotRes, SpotRes, BppDepth32) * UsedSpotShadow;
        size_t SpotVSMBytes   = Bytes2D(SpotRes, SpotRes, BppR16G16F) * UsedSpotShadow;

        // Point: depth cube + VSM cube (6 faces per light)
        size_t PointDepthBytes = Bytes2D(PointRes, PointRes, BppDepth32) * (UsedPointShadow * 6ull);
        size_t PointVSMBytes   = Bytes2D(PointRes, PointRes, BppR16G16F) * (UsedPointShadow * 6ull);

        // Directional: CSM cascades
        const uint32 Cascades =  CSM->GetCascadeNum();
        size_t DirCSMBytes = UsedDirShadow ? Bytes2D(DirRes, DirRes, BppD24S8) * Cascades : 0ull;

        size_t TotalBytes = SpotDepthBytes + SpotVSMBytes + PointDepthBytes + PointVSMBytes + DirCSMBytes;

        auto ToKB = [](size_t bytes) -> double { return static_cast<double>(bytes) / 1024.0; };
        auto ToMB = [](size_t bytes) -> double { return static_cast<double>(bytes) / (1024.0 * 1024.0); };

        wchar_t Buf[768];
        swprintf_s(
            Buf,
            L"[LIGHT]\n"
            L"Counts\n"
            L"  Point:       %3u\n"
            L"  Spot:        %3u\n"
            L"  Directional: %3u\n"
            L"\n"
            L"Resolution\n"
            L"  Dir CSM:  %4ux%4u, casc:%u\n"
            L"  Spot:     %4ux%4u, slices:%u\n"
            L"  Point:    %4ux%4u, cubes:%u \n"
            L"\n"
            L"Memory (MB)\n"
            L"  Dir CSM:      %6.2f\n"
            L"  Spot Depth:   %6.2f\n"
            L"  Spot VSM:     %6.2f\n"
            L"  Point Depth:  %6.2f\n"
            L"  Point VSM:    %6.2f\n"
            L"  Total:        %6.2f",
            PointLightCount, SpotLightCount, DirectionalCount,
            DirRes, DirRes, Cascades,
            SpotRes, SpotRes, UsedSpotShadow,
            PointRes, PointRes, UsedPointShadow,
            ToMB(DirCSMBytes),
            ToMB(SpotDepthBytes),
            ToMB(SpotVSMBytes),
            ToMB(PointDepthBytes),
            ToMB(PointVSMBytes),
            ToMB(TotalBytes));

        const float fontSize = 16.0f;
        const float minHeight = 160.0f; // ensure decent base height
        // Use layout-based measurement to include wrapped lines within PanelWidth
        const float panelHeight = CalcPanelHeightForText(Dwrite, Buf, fontSize, PanelWidth, minHeight);
        D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + panelHeight);

        DrawTextBlock(
            D2dCtx, Dwrite, Buf, rc, fontSize,
            D2D1::ColorF(0, 0, 0, 0.6f),
            D2D1::ColorF(D2D1::ColorF::Cyan));

        NextY += panelHeight + Space;
    }

  //  if (bShowShadowInfo)
  //  {
		//// TODO: Shadow 관련 사용된 메모리량, 라이트 종류 별 개수 등 출력

		//
		//wchar_t Buf[256];

		//// PointLight, SpotLight, DirectionalLight 개수
		//FLightManager* LightManager = GWorld->GetLightManager();

		//const uint32 PointLightCount = LightManager->GetPointLightCount();
		//const uint32 SpotLightCount = LightManager->GetSpotLightCount();

		//swprintf_s(Buf, L"[Shadow Info]\nPoint Lights: %u\nSpot Lights: %u\nDirectional Lights: %u\n\nPoint Lights Tex Count: %u\nSpot Lights Tex Count: %u\nDirectional Lights Tex Count: %u",
		//	PointLightCount,
		//	SpotLightCount,
		//	1,
		//	PointLightCount * 6,
		//	SpotLightCount,
		//	1
		//);

		//// 3. 텍스트를 여러 줄 표시해야 하므로 패널 높이를 늘립니다.
  //      const float fontSize = 16.0f;
  //      const float minHeight = 140.0f;
  //      const float ShadowPanelHeight = CalcPanelHeightForText(Dwrite, Buf, fontSize, PanelWidth, minHeight);
  //      D2D1_RECT_F rc = D2D1::RectF(Margin, NextY, Margin + PanelWidth, NextY + ShadowPanelHeight);

		//// 4. DrawTextBlock 함수를 호출하여 화면에 그립니다. 색상은 구분을 위해 주황색(Orange)으로 설정합니다.
		//DrawTextBlock(
  //          D2dCtx, Dwrite, Buf, rc, fontSize,
  //          D2D1::ColorF(0, 0, 0, 0.6f),
  //          D2D1::ColorF(D2D1::ColorF::Orange));

  //      NextY += ShadowPanelHeight + Space;
  //  }

	D2dCtx->EndDraw();
	D2dCtx->SetTarget(nullptr);

	SafeRelease(TargetBmp);
	SafeRelease(Dwrite);
	SafeRelease(D2dCtx);
	SafeRelease(D2dDevice);
	SafeRelease(DxgiDevice);
	SafeRelease(Surface);
	SafeRelease(D2dFactory);
}

void UStatsOverlayD2D::SetShowFPS(bool b)
{
	bShowFPS = b;
}

void UStatsOverlayD2D::SetShowMemory(bool b)
{
	bShowMemory = b;
}

void UStatsOverlayD2D::SetShowPicking(bool b)
{
	bShowPicking = b;
}

void UStatsOverlayD2D::SetShowDecal(bool b)
{
	bShowDecal = b;
}

void UStatsOverlayD2D::ToggleFPS()
{
	bShowFPS = !bShowFPS;
}

void UStatsOverlayD2D::ToggleMemory()
{
	bShowMemory = !bShowMemory;
}

void UStatsOverlayD2D::TogglePicking()
{
	bShowPicking = !bShowPicking;
}

void UStatsOverlayD2D::ToggleDecal()
{
	bShowDecal = !bShowDecal;
}

void UStatsOverlayD2D::SetShowTileCulling(bool b)
{
	bShowTileCulling = b;
}

void UStatsOverlayD2D::SetShowShadowInfo(bool b)
{
	bShowShadowInfo = b;
}

void UStatsOverlayD2D::SetShowPhysics(bool b)
{
	bShowPhysics = b;
}

void UStatsOverlayD2D::ToggleTileCulling()
{
	bShowTileCulling = !bShowTileCulling;
}

void UStatsOverlayD2D::ToggleShadowInfo()
{
	bShowShadowInfo = !bShowShadowInfo;
}

void UStatsOverlayD2D::TogglePhysics()
{
	bShowPhysics = !bShowPhysics;
}


