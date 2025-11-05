#pragma once
// b0 in VS    
#include "Color.h"
#include "LightManager.h"
//#include "ShadowSystem.h"

struct FShadowInfo
{
    // PSM 말고 VP로 일단 SpotLight만 처리
    FMatrix ViewProjectionMatrix;

    FVector LightPosition;
    // Depth normalization params for VSM (linear view-space z)
    float Near;

    float Far;
    float Padding[3]; // 16-byte alignment for array element size

    float ShadowBias = 0.0001f;
    float ShadowSlopeBias;
    float MaxSlopeDepthBias;
	float Padding2; // 16-byte alignment
};

struct ModelBufferType // b0
{
    FMatrix Model;
    FMatrix ModelInverseTranspose;  // For correct normal transformation with non-uniform scale
};

struct ViewProjBufferType // b1 고유번호 고정
{
    FMatrix View;
    FMatrix Proj;
    FMatrix InvView;
    FMatrix InvProj;
	FMatrix ViewProjection; // View * Proj
};

struct DecalBufferType
{
    FMatrix DecalMatrix;
    float Opacity;
};

struct PostProcessBufferType // b0
{
    float Near;
    float Far;
    int IsOrthographic; // 0 = Perspective, 1 = Orthographic
    float Padding; // 16바이트 정렬을 위한 패딩
};

struct FogBufferType // b2
{
    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;

    FVector4 FogInscatteringColor; // 16 bytes alignment 위해 중간에 넣음

    float FogMaxOpacity;
    float FogHeight; // fog base height
    float Padding[2]; // 16바이트 정렬을 위한 패딩
};

struct FXAABufferType // b2
{
    FVector2D ScreenSize; // 화면 해상도 (e.g., float2(1920.0f, 1080.0f))
    FVector2D InvScreenSize; // 1.0f / ScreenSize (픽셀 하나의 크기)

    float EdgeThresholdMin; // 엣지 감지 최소 휘도 차이 (0.0833f 권장)
    float EdgeThresholdMax; // 엣지 감지 최대 휘도 차이 (0.166f 권장)
    float QualitySubPix; // 서브픽셀 품질 (낮을수록 부드러움, 0.75 권장)
    int32_t QualityIterations; // 엣지 탐색 반복 횟수 (12 권장)
};

// b0 in PS
struct FMaterialInPs
{
    FVector DiffuseColor; // Kd
    float OpticalDensity; // Ni

    FVector AmbientColor; // Ka
    float Transparency; // Tr Or d

    FVector SpecularColor; // Ks
    float SpecularExponent; // Ns

    FVector EmissiveColor; // Ke
    uint32 IlluminationModel; // illum. Default illumination model to Phong for non-Pbr materials

    FVector TransmissionFilter; // Tf
    float dummy; // 4 bytes padding
    FMaterialInPs() = default;
    FMaterialInPs(const FMaterialInfo& MaterialInfo)
        :DiffuseColor(MaterialInfo.DiffuseColor),
        OpticalDensity(MaterialInfo.OpticalDensity),
        AmbientColor(MaterialInfo.AmbientColor),
        Transparency(MaterialInfo.Transparency),
        SpecularColor(MaterialInfo.SpecularColor),
        SpecularExponent(MaterialInfo.SpecularExponent),
        EmissiveColor(MaterialInfo.EmissiveColor),
        IlluminationModel(MaterialInfo.IlluminationModel),
        TransmissionFilter(MaterialInfo.TransmissionFilter),
        dummy(0)
    { 

    }
};


struct FPixelConstBufferType
{
    FMaterialInPs Material;
    uint32 bHasMaterial;
    uint32 bHasDiffuseTexture;
    uint32 bHasNormalTexture;
	float Padding; // 16바이트 정렬을 위한 패딩
};

struct FColorBufferType // b3
{
    FLinearColor Color;
    uint32 UUID;
    FVector Padding;
};

struct FLightBufferType
{
    FAmbientLightInfo AmbientLight;
    FDirectionalLightInfo DirectionalLight;

    uint32 PointLightCount;
    uint32 SpotLightCount;
    FVector2D Padding;
};


struct FShadowBufferType
{
    // 그림자 최대 100개로 일단 하드코딩함
    FShadowInfo ShadowInfoList[100];
    int32 NumSpotShadow;
	int32 NumPointShadow;
	float Padding[2]; // 16바이트 정렬을 위한 패딩
};

struct FShadowBufferIndexType
{
    int32 ShadowIndex;   // FShadowBufferType.ShadowInfoList 배열의 인덱스
    int32 LightType;     // ELightType (0=Ambient,1=Directional,2=Point,3=Spot,4=DirectionalCSM)
    int32 Padding[2];    // 16바이트 정렬을 위한 패딩

    FShadowBufferIndexType(int32 InShadowIndex, int32 InLightType = 0)
        : ShadowIndex(InShadowIndex)
        , LightType(InLightType)
    {
    }
};

// b10 고유번호 고정
struct FViewportConstants
{
    // x = Viewport TopLeftX
    // y = Viewport TopLeftY
    // z = Viewport Width
    // w = Viewport Height
    FVector4 ViewportRect;

    // x = Screen Width (전체 렌더 타겟 너비)
    // y = Screen Height (전체 렌더 타겟 높이)
    // z = 1.0f / Screen Width
    // w = 1.0f / Screen Height
    FVector4 ScreenSize;
};

struct CameraBufferType // b7
{
    FVector CameraPosition;
    float Padding;
};

// b11: 타일 기반 라이트 컬링 상수 버퍼
struct FTileCullingBufferType
{
    uint32 TileSize;          // 타일 크기 (픽셀, 기본 16)
    uint32 TileCountX;        // 가로 타일 개수
    uint32 TileCountY;        // 세로 타일 개수
    uint32 bUseTileCulling;   // 타일 컬링 활성화 여부 (0=비활성화, 1=활성화)
};

// b13: 클러스터(Z-슬라이스) 컬링 상수 버퍼 (PS only)
struct FClusterCullingBufferType
{
    uint32 ClusterCountZ;   // Z 슬라이스 개수
    float  ClusterNearZ;    // View-space Near (>0)
    float  ClusterFarZ;     // View-space Far  (>Near)
    float  Padding;         // 16바이트 정렬 패딩
};

// b5 in VS, PS
#define MAX_CASCADE 8 /* Also In UberLit.hlsl CSM ConstBuffer*/
__declspec(align(16))
struct FCSMConstants
{
    UINT NumCascades;
    UINT UseBlending;
    UINT UseDebugging;
    UINT UseShadow;
    float Padding0[2]; // align to 16 bytes

    FMatrix ShadowMatrix[MAX_CASCADE];
    
    FVector4 CascadeFar[MAX_CASCADE];
    FVector4 CascadeNear[MAX_CASCADE];
    FVector4 CascadeBlend[MAX_CASCADE];
};

#define CONSTANT_BUFFER_INFO(TYPE, SLOT, VS, PS) \
constexpr uint32 TYPE##Slot = SLOT;\
constexpr bool TYPE##IsVS = VS;\
constexpr bool TYPE##IsPS = PS;

//매크로를 인자로 받고 그 매크로 함수에 버퍼 전달
#define CONSTANT_BUFFER_LIST(MACRO) \
MACRO(ModelBufferType)              \
MACRO(DecalBufferType)              \
MACRO(PostProcessBufferType)        \
MACRO(FogBufferType)                \
MACRO(FXAABufferType)               \
MACRO(FPixelConstBufferType)        \
MACRO(ViewProjBufferType)           \
MACRO(FColorBufferType)              \
MACRO(CameraBufferType)             \
MACRO(FLightBufferType)             \
MACRO(FShadowBufferType)            \
MACRO(FViewportConstants)           \
MACRO(FTileCullingBufferType)       \
MACRO(FClusterCullingBufferType)    \
MACRO(FShadowBufferIndexType)       \
MACRO(FCSMConstants)                

// 16 바이트 패딩 어썰트
#define STATIC_ASSERT_CBUFFER_ALIGNMENT(Type) \
    static_assert(sizeof(Type) % 16 == 0, "[ " #Type " ] Bad Size. Needs 16-Byte Padding.");
CONSTANT_BUFFER_LIST(STATIC_ASSERT_CBUFFER_ALIGNMENT)

//VS, PS 세팅은 함수 파라미터로 결정하게 하는게 훨씬 나을듯 나중에 수정 필요
//그리고 UV Scroll 상수버퍼도 처리해줘야함
CONSTANT_BUFFER_INFO(ModelBufferType, 0, true, false)
CONSTANT_BUFFER_INFO(PostProcessBufferType, 0, false, true)
CONSTANT_BUFFER_INFO(ViewProjBufferType, 1, true, true) // b1 카메라 행렬 고정
CONSTANT_BUFFER_INFO(FogBufferType, 2, false, true)
CONSTANT_BUFFER_INFO(FXAABufferType, 2, false, true)
CONSTANT_BUFFER_INFO(FColorBufferType, 3, true, true)   // b3 color
CONSTANT_BUFFER_INFO(FPixelConstBufferType, 4, true, true) // GOURAUD에도 사용되므로 VS도 true
CONSTANT_BUFFER_INFO(FCSMConstants, 5, true, true) // b5, CSM용으로 Uberlit, Shadow Shader에서 씀
CONSTANT_BUFFER_INFO(DecalBufferType, 6, true, true)
CONSTANT_BUFFER_INFO(CameraBufferType, 7, true, true)  // b7, VS+PS (UberLit.hlsl과 일치) -> b1로 변경고려
CONSTANT_BUFFER_INFO(FLightBufferType, 8, true, true)
CONSTANT_BUFFER_INFO(FViewportConstants, 10, true, true)   // VS+PS에서 모두 사용 (디버그/타일 인덱싱 보정)
CONSTANT_BUFFER_INFO(FTileCullingBufferType, 11, false, true)  // b11, PS only (UberLit.hlsl과 일치)
CONSTANT_BUFFER_INFO(FClusterCullingBufferType, 13, false, true) // b13, PS only (LightingBuffers.hlsl과 일치)
CONSTANT_BUFFER_INFO(FShadowBufferType, 12, true, true)
CONSTANT_BUFFER_INFO(FShadowBufferIndexType, 9, true, true) // b9 used by DepthOnly VS+PS
// (Removed global shadow filter constant buffer; per-light sharpen is used.)
