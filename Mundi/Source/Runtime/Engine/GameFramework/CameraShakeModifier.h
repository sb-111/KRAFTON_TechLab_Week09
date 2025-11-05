#pragma once
#include "CameraModifier.h"
#include "Vector.h"

enum class EShakePattern : uint8
{
	SineWave,      // 부드럽고 예측 가능한 사인파 쉐이크
	PerlinNoise    // 자연스럽고 불규칙한 펄린 노이즈 쉐이크
};

class UCameraShakeModifier : public UCameraModifier
{
public:
	DECLARE_CLASS(UCameraShakeModifier, UCameraModifier)

	void ModifyCamera(float DeltaTime, FMinimalViewInfo& ViewInfo) override;
	void StartShake(float InDuration, FVector InLocationAmplitude, FVector InRotationAmplitude, float InFovAmplitude, float InFrequency = 10.0f, EShakePattern InPattern = EShakePattern::SineWave);
private:
	FVector LocationAmplitude; // 위치 흔들림 강도
	FVector RotationAmplitude; // 회전 흔들림 강도
	float FOVAmplitude = 0.0f; // FOV 흔들림 강도

	float ShakeFrequency = 10.0f;
	float ShakeDuration = 1.0f;
	float ShakeTime = 0.0f;
	EShakePattern ShakePattern = EShakePattern::SineWave;

	bool bBlendIn = true;
	bool bBlendOut = true;
	float BlendInTime = 0.2f;
	float BlendOutTime = 0.2f;

	// Perlin Noise용 시드 (랜덤성)
	float NoiseSeedX = 0.0f;
	float NoiseSeedY = 0.0f;
	float NoiseSeedZ = 0.0f;



};