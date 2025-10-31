#pragma once

#define NUM_LIGHT_MAX 800
#define NUM_POINT_LIGHT_MAX 256
#define NUM_SPOT_LIGHT_MAX 256

class UAmbientLightComponent;
class UDirectionalLightComponent;
class UPointLightComponent;
class USpotLightComponent;
class ULightComponent;
class D3D11RHI;

enum class ELightType
{
    AmbientLight,
    DirectionalLight,
    PointLight,
    SpotLight,
};
struct FAmbientLightInfo
{
    FLinearColor Color;     // 16 bytes - Color already includes Intensity and Temperature
    // Total: 16 bytes
};

struct FDirectionalLightInfo
{
    FLinearColor Color;     // 16 bytes - Color already includes Intensity and Temperature
    FVector Direction;      // 12 bytes
    int32 ShadowIndex;      // 4 bytes
    float ShadowSharpen;
    FVector Padding;
    // Total: 32 bytes
};

struct FPointLightInfo
{
    FLinearColor Color;         // 16 bytes - Color already includes Intensity and Temperature
    FVector Position;           // 12 bytes
    float AttenuationRadius;    // 4 bytes (moved up to fill slot)
    float FalloffExponent;      // 4 bytes - Falloff exponent for artistic control
    uint32 bUseInverseSquareFalloff; // 4 bytes - true = physically accurate, false = exponent-based
    int32 ShadowIndex;            // 4 bytes
    float ShadowSharpen;        // 4 bytes - per-light shadow sharpness (0=soft, 1=sharp)
};

struct FSpotLightInfo
{
    FLinearColor Color;         // 16 bytes - Color already includes Intensity and Temperature
    FVector Position;           // 12 bytes
    float InnerConeAngle;       // 4 bytes
    FVector Direction;          // 12 bytes
    float OuterConeAngle;       // 4 bytes
    float AttenuationRadius;    // 4 bytes
    float FalloffExponent;      // 4 bytes - Falloff exponent for artistic control
    uint32 bUseInverseSquareFalloff; // 4 bytes - true = physically accurate, false = exponent-based
    float ShadowSharpen;        // 4 bytes - per-light shadow sharpness (0=soft, 1=sharp)
    int32 ShadowIndex;          // 4 bytes
    // Total: 64 bytes
};

class FLightManager
{

public:
    FLightManager() = default;
    ~FLightManager();

    void Initialize(D3D11RHI* RHIDevice);
    void Release();

    void UpdateLightBuffer(D3D11RHI* RHIDevice);
    
	void SetAllLightShadowInfoToDefault();

    UDirectionalLightComponent* GetDirectionalLight();
    //리스트를 클리어하고 새로운 LightInfoList를 리턴
    const TArray<UPointLightComponent*>& GetPointLightList() {return PointLightList;}
    const TArray<USpotLightComponent*>& GetSpotLightList() {  return SpotLightList;}

    // SRV getters for compute shader usage
    ID3D11ShaderResourceView* GetPointLightSRV() const { return PointLightBufferSRV; }
    ID3D11ShaderResourceView* GetSpotLightSRV() const { return SpotLightBufferSRV; }

    uint32 GetPointLightCount() const { return PointLightNum; }
    uint32 GetSpotLightCount() const { return SpotLightNum; }

    template<typename T>
    void RegisterLight(T* LightComponent);
    template<typename T>
    void DeRegisterLight(T* LightComponent);

    void ClearAllLightList();
private:

    //structured buffer
    ID3D11Buffer* PointLightBuffer = nullptr;
    ID3D11Buffer* SpotLightBuffer = nullptr;
    ID3D11ShaderResourceView* PointLightBufferSRV = nullptr;
    ID3D11ShaderResourceView* SpotLightBufferSRV = nullptr;

    TArray<UAmbientLightComponent*> AmbientLightList;
    TArray<UDirectionalLightComponent*> DirectionalLightList;
    TArray<UPointLightComponent*> PointLightList;
    TArray<USpotLightComponent*> SpotLightList;

    //FDirectionalLightInfo DirectionalLightInfo;
    //TArray<FPointLightInfo> PointLightInfoList;
    //TArray<FSpotLightInfo> SpotLightInfoList;

    //이미 레지스터된 라이트인지 확인하는 용도
    TSet<ULightComponent*> LightComponentList;
    uint32 PointLightNum = 0;
    uint32 SpotLightNum = 0;
};

template<> void FLightManager::RegisterLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent);
template<> void FLightManager::RegisterLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent);
template<> void FLightManager::RegisterLight<UPointLightComponent>(UPointLightComponent* LightComponent);
template<> void FLightManager::RegisterLight<USpotLightComponent>(USpotLightComponent* LightComponent);

template<> void FLightManager::DeRegisterLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent);
template<> void FLightManager::DeRegisterLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent);
template<> void FLightManager::DeRegisterLight<UPointLightComponent>(UPointLightComponent* LightComponent);
template<> void FLightManager::DeRegisterLight<USpotLightComponent>(USpotLightComponent* LightComponent);
