#pragma once
#include "SceneComponent.h"

class USpringArmComponent : public USceneComponent
{
public:
	DECLARE_CLASS(USpringArmComponent, USceneComponent)
	GENERATED_REFLECTION_BODY()

	USpringArmComponent();

	void OnRegister(UWorld* InWorld) override;
	void BeginPlay() override;

	FTransform GetSocketWorldTransform() const override;
	void TickComponent(float DeltaTime) override;
	
	void RenderDebugVolume(class URenderer* Renderer) const override;

	void EvaluateArm(float DeltaTime);

	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(USpringArmComponent)

private:
	// Cache
	mutable FTransform CachedSocketWorld;
	mutable bool bSocketValid = false;

	// Settings (Property)
	float TargetArmLength = 10.0f;              // 원하는 암 길이
	FVector SocketOffset = { 0.0f, 0.0f, 0.0f }; // 암 끝 기준 위치 오프셋
	FVector TargetOffset = { 0.0f, 0.0f, 0.0f }; // "무엇을 볼지" 피벗 보정(부모 좌표계 기준)
	float BackoffEpsilon = 0.1f;                // 히트 시 살짝 당겨서 떨림 방지

	FQuat InitialRotation;
	bool bUseControllerRotation = true;		// 컨트롤러의 Rotation 사용
};
