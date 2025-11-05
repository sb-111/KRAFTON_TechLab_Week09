#pragma once
#include "SceneComponent.h"

class USpringArmComponent : public USceneComponent
{
public:
	DECLARE_CLASS(USpringArmComponent, USceneComponent)
	GENERATED_REFLECTION_BODY()

	USpringArmComponent() = default;
	
	FTransform GetSocketWorldTransform() const override;
	void OnTransformUpdated() override;

	void EvaluateArm(float DeltaTime);

private:
	// Cache
	FTransform  CachedSocketWorld;
	bool bSocketValid = false;

	// Properties
	float TargetArmLength = 300.0f;              // 원하는 암 길이
	FVector SocketOffset = { 0.0f, 0.0f, 0.0f }; // 암 끝 기준 위치 오프셋
	FVector TargetOffset = { 0.0f, 0.0f, 0.0f }; // "무엇을 볼지" 피벗 보정(부모 좌표계 기준)
	float BackoffEpsilon = 0.01f;                // 히트 시 살짝 당겨서 떨림 방지(cm)

};