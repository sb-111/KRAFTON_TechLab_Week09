#pragma once
#include "SceneComponent.h"
#include "Color.h"
#include "ObjectMacros.h"

// 모든 라이트 컴포넌트의 베이스 클래스
class ULightComponentBase : public USceneComponent
{
public:
	DECLARE_CLASS(ULightComponentBase, USceneComponent)
	GENERATED_REFLECTION_BODY()

	ULightComponentBase();
	virtual ~ULightComponentBase() override;

public:
	//// Light Properties
	//void SetEnabled(bool bInEnabled) { bIsEnabled = bInEnabled; }
	//bool IsEnabled() const { return bIsEnabled; }

	void SetIntensity(float InIntensity) { Intensity = InIntensity;  }
	float GetIntensity() const { return Intensity; }
	bool GetCastShadow() const { return bCastShadows; }
	void SetCastShadow(const bool InCastShadows) { bCastShadows = InCastShadows; }

	void SetLightColor(const FLinearColor& InColor) { LightColor = InColor; }
	void SetShadowIndex(int32 InShadowIndex) { ShadowIndex = InShadowIndex; }
	int32 GetShadowIndex() const { return ShadowIndex; }
	const FLinearColor& GetLightColor() const { return LightColor; }

	// Serialization & Duplication
	virtual void OnSerialized() override;
	virtual void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(ULightComponentBase)

protected:
	bool bCastShadows = true;
	float Intensity = 1.0f;
	int32 ShadowIndex = -1;
	FLinearColor LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};
