#include "pch.h"
#include "PlayerCameraManager.h"
#include "Actor.h"
#include "math.h"

IMPLEMENT_CLASS(APlayerCameraManager)

void APlayerCameraManager::Tick(float DeltaTime)
{
	if (FadeTimeRemaining > 0)
	{
		FadeTimeRemaining -= DeltaTime;

		if (FadeTimeRemaining <= 0)
		{
			FadeAmount = FadeAlpha.Y;
			FadeTimeRemaining = 0.0f;
		}
		else
		{
			float TimePercentage = 1.0f - FadeTimeRemaining / FadeTime;
			FadeAmount = FadeAlpha.X + (FadeAlpha.Y - FadeAlpha.X) * TimePercentage;
		}
	}

	if (ViewTarget.TargetActor.Get())
	{
		// 매번 새로 계산
		ViewTarget.ViewInfo = ViewTarget.TargetActor->CalcCamera();
	}
	// TODO: 타겟 액터가 없는 경우 처리
}

void APlayerCameraManager::StartFadeInOut(float InFadeTime, float InTargetAlpha)
{
	FadeTime = InFadeTime;
	// 시작 알파
	FadeAlpha.X = FadeAmount;
	FadeAlpha.Y = InTargetAlpha;

	FadeTimeRemaining = FadeTime;

	if (InFadeTime <= 0)
	{
		FadeAmount = FadeAlpha.Y;
		FadeTime = 0.0f;
	}
}

void APlayerCameraManager::StartFadeOut(float InFadeTime, FLinearColor InFadeColor)
{
	FadeColor = InFadeColor;

	StartFadeInOut(InFadeTime, 1.0f);
}

void APlayerCameraManager::StartFadeIn(float InFadeTime)
{
	StartFadeInOut(InFadeTime, 0.0f);
}

void APlayerCameraManager::SetViewTarget(AActor* InTargetActor, float TransitionTime)
{
	ViewTarget.TargetActor = InTargetActor;
}