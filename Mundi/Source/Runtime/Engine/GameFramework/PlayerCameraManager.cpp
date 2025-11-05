#include "pch.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
#include "CameraModifier.h"
#include "Actor.h"

IMPLEMENT_CLASS(APlayerCameraManager)
void APlayerCameraManager::Tick(float DeltaTime)
{
	UpdateViewInfo();
	//UpdateFadeInOut(DeltaTime); // FadeAmount 계산 (보간)
	UpdateVignetteBlend(DeltaTime); // VignetteIntensity/Radius 계산 (보간)
	UpdateLetterboxBlend(DeltaTime); // LetterBoxSize 계산(보간)
	UpdatePostProcess(DeltaTime); // Modifier들이 수정(선택)해서 GPU로 전송
}

//void APlayerCameraManager::UpdateFadeInOut(float DeltaTime)
//{
//	if (FadeTimeRemaining > 0.0f)
//	{
//		// Fade in out 처리
//		FadeTimeRemaining -= DeltaTime;
//
//		if (FadeTime > 0.0f)
//		{
//			float RemainingRatio = FadeTimeRemaining / FadeTime;
//			float ProgressRatio = FMath::Clamp(1.0f - RemainingRatio, 0.0f, 1.0f);
//			// X to Y까지 Lerp (진행률 이용해서)
//			FadeAmount = FMath::Lerp(FadeAlpha.X, FadeAlpha.Y, ProgressRatio);
//			DefaultPostProcessSettings.FadeAmount = FadeAmount;
//		}
//		else // FadeTime이 0이면 즉시 페이드, 목표값 즉시 설정
//		{
//			FadeTimeRemaining = 0.0f; // 타이머 종료
//			FadeAmount = FadeAlpha.Y;
//			DefaultPostProcessSettings.FadeAmount = FadeAmount;
//		}
//	}
//	else
//	{
//		// 페이드 진행 중 아니면 값을 마지막 목표 값으로 고정
//		FadeAmount = FadeAlpha.Y;
//		DefaultPostProcessSettings.FadeAmount = FadeAmount;
//	}
//}

void APlayerCameraManager::UpdateViewInfo()
{
	if (ViewTarget.TargetActor.Get())
	{
		if (UCameraComponent* CameraComponent = ViewTarget.TargetActor->GetComponentByClass<UCameraComponent>())
		{
			ViewTarget.ViewInfo.Aspect = CameraComponent->GetAspectRatio();
			ViewTarget.ViewInfo.Fov = CameraComponent->GetFOV();
			ViewTarget.ViewInfo.ZNear = CameraComponent->GetNearClip();
			ViewTarget.ViewInfo.ZFar = CameraComponent->GetFarClip();
			ViewTarget.ViewInfo.ProjectionMode = CameraComponent->GetProjectionMode();
			ViewTarget.ViewInfo.Location = CameraComponent->GetWorldLocation();
			ViewTarget.ViewInfo.Rotation = CameraComponent->GetWorldRotation();
		}
	}
}

void APlayerCameraManager::SetViewTarget(AActor* InTargetActor, float TransitionTime)
{
	ViewTarget.TargetActor = InTargetActor;

	UpdateViewInfo();
}

void APlayerCameraManager::UpdatePostProcess(float DeltaTime)
{
	//  Default → Cached 복사
	CachedPostProcessSettings = DefaultPostProcessSettings;
	
	// 각 CameraModifier가 Cached를 수정 가능(선택적)
	for (auto& Modifier : ModifierList)
	{
		if (Modifier && !Modifier->IsDisabled())
		{
			Modifier->ModifyPostProcess(DeltaTime, CachedPostProcessSettings);
		}
	}

	// 4. 추후 PostProcessVolume과 블렌딩 가능
	// TODO: PostProcessVolume 지원 시 여기서 블렌딩
	
	// SceneRender는 Cached를 읽어서 GPU로 전송
}
void APlayerCameraManager::UpdateLetterboxBlend(float DeltaTime)
{
	if (LetterboxBlendTimeRemaining > 0.0f)
	{
		LetterboxBlendTimeRemaining -= DeltaTime;

		if (LetterboxBlendTime > 0.0f)
		{
			float ProgressRatio = FMath::Clamp(1.0f - (LetterboxBlendTimeRemaining / LetterboxBlendTime), 0.0f, 1.0f);
			DefaultPostProcessSettings.LetterboxSize = FMath::Lerp(LetterboxSizeAlpha.X, LetterboxSizeAlpha.Y, ProgressRatio);
		}
		else
		{
			// 즉시 적용
			LetterboxBlendTimeRemaining = 0.0f;
			DefaultPostProcessSettings.LetterboxSize = LetterboxSizeAlpha.Y;
		}
	}
	else
	{
		// 보간 완료 후 목표값 유지
		DefaultPostProcessSettings.LetterboxSize = LetterboxSizeAlpha.Y;
		DefaultPostProcessSettings.bEnableLetterbox = bLetterboxBlendTargetEnable;
	}
}
void APlayerCameraManager::UpdateVignetteBlend(float DeltaTime)
{
	if (VignetteBlendTimeRemaining > 0.0f)
	{
		VignetteBlendTimeRemaining -= DeltaTime;

		if (VignetteBlendTime > 0.0f)
		{
			float ProgressRatio = FMath::Clamp(1.0f - (VignetteBlendTimeRemaining / VignetteBlendTime), 0.0f, 1.0f);

			// Intensity와 Radius 모두 보간
			DefaultPostProcessSettings.VignetteIntensity = FMath::Lerp(VignetteIntensityAlpha.X, VignetteIntensityAlpha.Y, ProgressRatio);
			DefaultPostProcessSettings.VignetteRadius = FMath::Lerp(VignetteRadiusAlpha.X, VignetteRadiusAlpha.Y, ProgressRatio);
		}
		else
		{
			// 즉시 적용
			VignetteBlendTimeRemaining = 0.0f;
			DefaultPostProcessSettings.VignetteIntensity = VignetteIntensityAlpha.Y;
			DefaultPostProcessSettings.VignetteRadius = VignetteRadiusAlpha.Y;
		}
	}
	else
	{
		// 보간 완료 후 목표값 유지
		DefaultPostProcessSettings.VignetteIntensity = VignetteIntensityAlpha.Y;
		DefaultPostProcessSettings.VignetteRadius = VignetteRadiusAlpha.Y;
		DefaultPostProcessSettings.bEnableVignetting = bVignetteBlendTargetEnable;
	}
}





/**
* @brief Vignette Blend 시작 (시간에 따라 서서히 변화)
*/
void APlayerCameraManager::StartVignetteBlend(float TargetIntensity, float TargetRadius, float Duration, bool bEnable)
{
	VignetteIntensityAlpha.X = DefaultPostProcessSettings.VignetteIntensity;
	VignetteIntensityAlpha.Y = TargetIntensity;
	VignetteRadiusAlpha.X = DefaultPostProcessSettings.VignetteRadius;
	VignetteRadiusAlpha.Y = TargetRadius;
	VignetteBlendTime = Duration;
	VignetteBlendTimeRemaining = Duration;
	bVignetteBlendTargetEnable = bEnable;
	DefaultPostProcessSettings.bEnableVignetting = true;  // 보간 중에는 활성화
}

/**
* @brief Letterbox Blend 시작 (시간에 따라 서서히 변화)
*/
void APlayerCameraManager::StartLetterboxBlend(float TargetSize, float Duration, bool bEnable)
{
	LetterboxSizeAlpha.X = DefaultPostProcessSettings.LetterboxSize;
	LetterboxSizeAlpha.Y = TargetSize;
	LetterboxBlendTime = Duration;
	LetterboxBlendTimeRemaining = Duration;
	bLetterboxBlendTargetEnable = bEnable;
	DefaultPostProcessSettings.bEnableLetterbox = true;  // 보간 중에는 활성화
}