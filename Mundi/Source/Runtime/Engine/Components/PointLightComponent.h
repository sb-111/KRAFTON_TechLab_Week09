#pragma once
#include "LocalLightComponent.h"
#include "LightManager.h"
#include "DelegateBinding.h"

// 점광원 (모든 방향으로 균등하게 빛 방출)
class UPointLightComponent : public ULocalLightComponent
{
public:
	DECLARE_CLASS(UPointLightComponent, ULocalLightComponent)
	GENERATED_REFLECTION_BODY()

	UPointLightComponent();
	virtual ~UPointLightComponent() override;

public:
	// Source Radius
	void SetSourceRadius(float InRadius) { SourceRadius = InRadius; }
	float GetSourceRadius() const { return SourceRadius; }

	// Light Info
	FPointLightInfo GetLightInfo() const;

	// Virtual Interface
	void OnRegister(UWorld* InWorld) override;
	void OnUnregister()	override;

	// Debug Rendering
	virtual void RenderDebugVolume(class URenderer* Renderer) const override;

	// Serialization & Duplication
	virtual void OnSerialized() override;
	virtual void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(UPointLightComponent)

	// 델리게이트 테스트
	//void BeginPlay() override;
	//void EndPlay(EEndPlayReason Reason) override;
	//void MoveUp(float Delta);
	

protected:
	float SourceRadius = 0.0f; // 광원 반경
	// 델리게이트 테스트
	//TArray<UStaticMeshComponent*> BroadCasters;
	//TArray<FBindingHandle> BindingHandles;
};
