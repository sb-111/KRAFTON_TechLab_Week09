#pragma once
#include "pch.h"

// Per-world render settings (view mode + show flags)
enum class EShadowFilterMode : uint32 
{ 
    PCF = 0,
    VSM = 1, 
    NONE = 2 
};

enum class EDirectionalShadowMode : uint32
{
    GENERAL = 0,
    CSM = 1
};

class URenderSettings {
public:
    URenderSettings() = default;
    ~URenderSettings() = default;

    // View mode
    void SetViewModeIndex(EViewModeIndex In) { ViewModeIndex = In; }
    EViewModeIndex GetViewModeIndex() const { return ViewModeIndex; }

    // Show flags
    EEngineShowFlags GetShowFlags() const { return ShowFlags; }
    void SetShowFlags(EEngineShowFlags In) { ShowFlags = In; }
    void EnableShowFlag(EEngineShowFlags Flag) { ShowFlags |= Flag; }
    void DisableShowFlag(EEngineShowFlags Flag) { ShowFlags &= ~Flag; }
    void ToggleShowFlag(EEngineShowFlags Flag) { ShowFlags = HasShowFlag(ShowFlags, Flag) ? (ShowFlags & ~Flag) : (ShowFlags | Flag); }
    bool IsShowFlagEnabled(EEngineShowFlags Flag) const { return HasShowFlag(ShowFlags, Flag); }

    // FXAA parameters
    void SetFXAAEdgeThresholdMin(float Value) { FXAAEdgeThresholdMin = Value; }
    float GetFXAAEdgeThresholdMin() const { return FXAAEdgeThresholdMin; }

    void SetFXAAEdgeThresholdMax(float Value) { FXAAEdgeThresholdMax = Value; }
    float GetFXAAEdgeThresholdMax() const { return FXAAEdgeThresholdMax; }

    void SetFXAAQualitySubPix(float Value) { FXAAQualitySubPix = Value; }
    float GetFXAAQualitySubPix() const { return FXAAQualitySubPix; }

    void SetFXAAQualityIterations(int32 Value) { FXAAQualityIterations = Value; }
    int32 GetFXAAQualityIterations() const { return FXAAQualityIterations; }

    // Tile-based light culling
    void SetTileSize(uint32 Value) { TileSize = Value; }
    uint32 GetTileSize() const { return TileSize; }

    void SetShadowFilterMode(EShadowFilterMode In) { ShadowFilterMode = In; }
    void SetDirectionaliShadowMode(EDirectionalShadowMode In) { DirectionalShadowMode = In; }
    EShadowFilterMode GetShadowFilterMode() const { return ShadowFilterMode; }
    EDirectionalShadowMode GetDirectionaliShadowMode() const { return DirectionalShadowMode; }

    // Shadow resolution (Spot/Point)
    void SetSpotShadowResolution(uint32 Value) { SpotShadowResolution = Value; }
    uint32 GetSpotShadowResolution() const { return SpotShadowResolution; }
    void SetPointShadowResolution(uint32 Value) { PointShadowResolution = Value; }
    uint32 GetPointShadowResolution() const { return PointShadowResolution; }
    // Directional shadow resolution
    void SetDirectionalShadowResolution(uint32 Value) { DirectionalShadowResolution = Value; }
    uint32 GetDirectionalShadowResolution() const { return DirectionalShadowResolution; }
    
private:
    EEngineShowFlags ShowFlags = EEngineShowFlags::SF_DefaultEnabled;
    EViewModeIndex ViewModeIndex = EViewModeIndex::VMI_Lit_Phong;

    // FXAA parameters
    float FXAAEdgeThresholdMin = 0.0833f;   // 엣지 감지 최소 휘도 차이 (권장: 0.0833)
    float FXAAEdgeThresholdMax = 0.166f;    // 엣지 감지 최대 휘도 차이 (권장: 0.166)
    float FXAAQualitySubPix = 0.75f;        // 서브픽셀 품질 (낮을수록 부드러움, 권장: 0.75)
    int32 FXAAQualityIterations = 12;       // 엣지 탐색 반복 횟수 (권장: 12)

    // Tile-based light culling
    uint32 TileSize = 16;                   // 타일 크기 (픽셀, 기본값: 16)

    // Shadow filtering
    EShadowFilterMode ShadowFilterMode = EShadowFilterMode::NONE;
    EDirectionalShadowMode DirectionalShadowMode = EDirectionalShadowMode::GENERAL;

    // Shadow resolution (used for Spot/Point atlas textures)
    uint32 SpotShadowResolution = 1024;
    uint32 PointShadowResolution = 1024;
    uint32 DirectionalShadowResolution = 4096;
};
