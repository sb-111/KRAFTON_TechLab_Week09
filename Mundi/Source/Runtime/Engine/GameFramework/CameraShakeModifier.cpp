#include "pch.h"
#include "CameraShakeModifier.h"
#include <random>

IMPLEMENT_CLASS(UCameraShakeModifier)

// 간단한 Perlin Noise 보간 함수
static float SmoothNoise(float x, float seed)
{
	float intPart = floor(x);
	float fracPart = x - intPart;

	// 정수 부분을 시드와 혼합하여 랜덤 값 생성
	float v1 = sin(intPart * 12.9898f + seed * 78.233f) * 43758.5453f;
	v1 = v1 - floor(v1); // [0, 1] 범위로

	float v2 = sin((intPart + 1.0f) * 12.9898f + seed * 78.233f) * 43758.5453f;
	v2 = v2 - floor(v2);

	// Smoothstep 보간
	float t = fracPart * fracPart * (3.0f - 2.0f * fracPart);
	return v1 * (1.0f - t) + v2 * t;
}

void UCameraShakeModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& ViewInfo)
{
	if (ShakeTime >= ShakeDuration)
	{
		SetDisabled(true);
		return;
	}

	// ShakeTime 업데이트
	ShakeTime += DeltaTime;

    // Blend 계산 (시작/종료 부드럽게)
    float BlendWeight = 1.0f;
    if (bBlendIn && ShakeTime < BlendInTime)
    {
        BlendWeight = ShakeTime / BlendInTime;
    }
    else if (bBlendOut && ShakeTime > ShakeDuration - BlendOutTime)
    {
        BlendWeight = (ShakeDuration - ShakeTime) / BlendOutTime;
    }

	FVector LocationOffset;
	FVector RotationOffset;
	float FOVOffset;

	// 패턴에 따라 분기
	if (ShakePattern == EShakePattern::SineWave)
	{
		// Sine wave 기반 쉐이크
		float Phase = ShakeTime * ShakeFrequency * 2.0f * PI;

		// 위치 오프셋
		LocationOffset = FVector(
			sin(Phase) * LocationAmplitude.X,
			sin(Phase * 1.3f) * LocationAmplitude.Y,  // 다른 주파수로 자연스럽게
			sin(Phase * 0.7f) * LocationAmplitude.Z
		);

		// 회전 오프셋 (Pitch, Yaw, Roll)
		RotationOffset = FVector(
			sin(Phase * 1.5f) * RotationAmplitude.X,  // Pitch
			sin(Phase * 1.2f) * RotationAmplitude.Y,  // Yaw
			sin(Phase * 0.9f) * RotationAmplitude.Z   // Roll
		);

		// FOV 오프셋
		FOVOffset = sin(Phase * 2.0f) * FOVAmplitude;
	}
	else // EShakePattern::PerlinNoise
	{
		// Perlin Noise 기반 쉐이크 (더 자연스럽고 불규칙)
		float NoiseTime = ShakeTime * ShakeFrequency;

		// 위치 오프셋 (각 축마다 다른 시드 사용)
		LocationOffset = FVector(
			(SmoothNoise(NoiseTime, NoiseSeedX) * 2.0f - 1.0f) * LocationAmplitude.X,
			(SmoothNoise(NoiseTime, NoiseSeedY) * 2.0f - 1.0f) * LocationAmplitude.Y,
			(SmoothNoise(NoiseTime, NoiseSeedZ) * 2.0f - 1.0f) * LocationAmplitude.Z
		);

		// 회전 오프셋
		RotationOffset = FVector(
			(SmoothNoise(NoiseTime, NoiseSeedX + 10.0f) * 2.0f - 1.0f) * RotationAmplitude.X,
			(SmoothNoise(NoiseTime, NoiseSeedY + 20.0f) * 2.0f - 1.0f) * RotationAmplitude.Y,
			(SmoothNoise(NoiseTime, NoiseSeedZ + 30.0f) * 2.0f - 1.0f) * RotationAmplitude.Z
		);

		// FOV 오프셋
		FOVOffset = (SmoothNoise(NoiseTime, NoiseSeedX + 40.0f) * 2.0f - 1.0f) * FOVAmplitude;
	}

	// 오프셋 적용
    ViewInfo.Location += LocationOffset * BlendWeight;

    FQuat ShakeRotation = FQuat::MakeFromEulerZYX(RotationOffset * BlendWeight);
    ViewInfo.Rotation = ShakeRotation * ViewInfo.Rotation;

    ViewInfo.Fov += FOVOffset * BlendWeight;
}

void UCameraShakeModifier::StartShake(float InDuration, FVector InLocationAmplitude, FVector InRotationAmplitude, float InFovAmplitude, float InFrequency, EShakePattern InPattern)
{
	LocationAmplitude = InLocationAmplitude;
	RotationAmplitude = InRotationAmplitude;
	FOVAmplitude = InFovAmplitude;
	ShakeDuration = InDuration;
	ShakeFrequency = InFrequency;
	ShakePattern = InPattern;
	ShakeTime = 0.0f;
	SetDisabled(false);

	// Perlin Noise용 랜덤 시드 생성
	if (ShakePattern == EShakePattern::PerlinNoise)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dis(0.0f, 1000.0f);

		NoiseSeedX = dis(gen);
		NoiseSeedY = dis(gen);
		NoiseSeedZ = dis(gen);
	}
}
