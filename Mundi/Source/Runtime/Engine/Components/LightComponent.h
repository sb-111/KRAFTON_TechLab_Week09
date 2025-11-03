#pragma once
#include "LightComponentBase.h"

// 실제 조명 효과를 가진 라이트들의 공통 베이스
class ULightComponent : public ULightComponentBase
{
public:
	DECLARE_CLASS(ULightComponent, ULightComponentBase)
	GENERATED_REFLECTION_BODY()

    ULightComponent();
    virtual ~ULightComponent() override;

public:
	// 그림자 관련 설정
    void SetShadowSharpen(float InSharpen) { ShadowSharpen = InSharpen; }
    float GetShadowSharpen() const { return ShadowSharpen; }
    
	void SetShadowBias(float InBias) { ShadowBias = InBias; }
	float GetShadowBias() const { return ShadowBias; }

	void SetShadowSlopeBias(float InSlopeBias) { ShadowSlopeBias = InSlopeBias; }
	float GetShadowSlopeBias() const { return ShadowSlopeBias; }

	void SetMaxSlopeDepthBias(float InMaxDepthBias) { MaxSlopeDepthBias = InMaxDepthBias; }
	float GetMaxSlopeDepthBias() const { return MaxSlopeDepthBias; }

	void SetShadowValuesToDefault();

	// Temperature
	void SetTemperature(float InTemperature) { Temperature = InTemperature; }
	float GetTemperature() const { return Temperature; }

	// 색상과 강도를 합쳐서 반환
	virtual FLinearColor GetLightColorWithIntensity() const;
	void OnRegister(UWorld* InWorld) override;

	// Serialization & Duplication
	virtual void OnSerialized() override;
	virtual void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(ULightComponent)

protected:
    float ShadowSharpen;
	float ShadowBias = 0.0001f;;
	float ShadowSlopeBias;
	float MaxSlopeDepthBias;

    float Temperature = 6500.0f; // 색온도 (K)
};
