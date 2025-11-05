#include "pch.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
#include "CameraModifier.h"
#include "Actor.h"
#include "math.h"
#include "Bezier.h"

IMPLEMENT_CLASS(APlayerCameraManager)

void APlayerCameraManager::Tick(float DeltaTime)
{
	float WorldDeltaTime = DeltaTime / CustomTimeDilation;

	//UpdateFadeInOut(DeltaTime); // FadeAmount 계산 (보간)
	UpdateVignetteBlend(WorldDeltaTime); // VignetteIntensity/Radius 계산 (보간)
	UpdateLetterboxBlend(WorldDeltaTime); // LetterBoxSize 계산(보간)
	UpdatePostProcess(WorldDeltaTime); // Modifier들이 수정(선택)해서 GPU로 전송
	if (FadeTimeRemaining > 0)
	{
		FadeTimeRemaining -= WorldDeltaTime; // fade effect is depentent on WORLD TIME, not actor time 

		if (FadeTimeRemaining <= 0)
		{
			FadeAmount = FadeAlpha.Y;
			FadeTimeRemaining = 0.0f;
		}
		else
		{
			float TimePercentage = 1.0f - FadeTimeRemaining / FadeTime;
			FadeAmount =  std::lerp(FadeAlpha.X, FadeAlpha.Y, TimePercentage);
		}
	}

	if (ViewTarget.TargetActor.Get())
	{
		// 매번 새로 계산
		ViewTarget.ViewInfo = ViewTarget.TargetActor->CalcCamera();
	}
	// TODO: 타겟 액터가 없는 경우 처리

	if (TransitionTimeRemaining > 0)
	{
		TransitionTimeRemaining -= DeltaTime;

		float TimePercentage = 1.0f - TransitionTimeRemaining / TransitionTime;
		//TimePercentage = TimePercentage * TimePercentage * (3.0f - 2.0f * TimePercentage);
		TimePercentage = ImGui::BezierValue(TimePercentage, BezierValue);
		const FMinimalViewInfo& ViewInfo = ViewTarget.ViewInfo;
		ViewTarget.ViewInfo.Location = FVector::Lerp(PreviousViewInfo.Location, ViewInfo.Location, TimePercentage);
		ViewTarget.ViewInfo.Rotation = FQuat::Slerp(PreviousViewInfo.Rotation, ViewInfo.Rotation, TimePercentage);
		ViewTarget.ViewInfo.Aspect = std::lerp(PreviousViewInfo.Aspect, ViewInfo.Aspect, TimePercentage);
		ViewTarget.ViewInfo.ZNear = std::lerp(PreviousViewInfo.ZNear, ViewInfo.ZNear, TimePercentage);
		ViewTarget.ViewInfo.ZFar = std::lerp(PreviousViewInfo.ZFar, ViewInfo.ZFar, TimePercentage);
		ViewTarget.ViewInfo.Fov = std::lerp(PreviousViewInfo.Fov, ViewInfo.Fov, TimePercentage);
	}

	// 카메라 Modifier 적용 (쉐이크 등)
	UpdateCameraModifiers(DeltaTime);
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

void APlayerCameraManager::SetViewTarget(AActor* InTargetActor, float InTransitionTime)
{
	PreviousViewInfo = ViewTarget.ViewInfo;
	ViewTarget.TargetActor = InTargetActor;
	TransitionTime = InTransitionTime;
	TransitionTimeRemaining = TransitionTime;
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
	// Blend가 활성화되어 있을 때만 업데이트
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

		// 보간 완료 시 최종 상태 적용
		if (LetterboxBlendTimeRemaining <= 0.0f)
		{
			DefaultPostProcessSettings.bEnableLetterbox = bLetterboxBlendTargetEnable;
		}
	}
	// Blend가 시작되지 않았으면 아무것도 하지 않음 (즉시 설정된 값 유지)
}
void APlayerCameraManager::UpdateVignetteBlend(float DeltaTime)
{
	// Blend가 활성화되어 있을 때만 업데이트
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

		// 보간 완료 시 최종 상태 적용
		if (VignetteBlendTimeRemaining <= 0.0f)
		{
			DefaultPostProcessSettings.bEnableVignetting = bVignetteBlendTargetEnable;
		}
	}
	// Blend가 시작되지 않았으면 아무것도 하지 않음 (즉시 설정된 값 유지)
}

/**
* @brief Vignette Blend 시작 (현재값에서 목표값으로 시간에 따라 서서히 변화)
*/
//void APlayerCameraManager::StartVignetteBlend(float TargetIntensity, float TargetRadius, float Duration, bool bEnable)
//{
//	VignetteIntensityAlpha.X = DefaultPostProcessSettings.VignetteIntensity;
//	VignetteIntensityAlpha.Y = TargetIntensity;
//	VignetteRadiusAlpha.X = DefaultPostProcessSettings.VignetteRadius;
//	VignetteRadiusAlpha.Y = TargetRadius;
//	VignetteBlendTime = Duration;
//	VignetteBlendTimeRemaining = Duration;
//	bVignetteBlendTargetEnable = bEnable;
//	DefaultPostProcessSettings.bEnableVignetting = true;  // 보간 중에는 활성화
//}

/**
* @brief Vignette Blend 시작 (시작값과 끝값을 모두 명시적으로 지정)
*/
void APlayerCameraManager::StartVignetteBlend(float StartIntensity, float StartRadius, float TargetIntensity, float TargetRadius, float Duration, bool bEnable)
{
	// 시작값을 즉시 적용
	DefaultPostProcessSettings.VignetteIntensity = StartIntensity;
	DefaultPostProcessSettings.VignetteRadius = StartRadius;

	// 보간 설정
	VignetteIntensityAlpha.X = StartIntensity;
	VignetteIntensityAlpha.Y = TargetIntensity;
	VignetteRadiusAlpha.X = StartRadius;
	VignetteRadiusAlpha.Y = TargetRadius;
	VignetteBlendTime = Duration;
	VignetteBlendTimeRemaining = Duration;
	bVignetteBlendTargetEnable = bEnable;
	DefaultPostProcessSettings.bEnableVignetting = true;  // 보간 중에는 활성화
}

/**
* @brief Letterbox Blend 시작 (현재값에서 목표값으로 시간에 따라 서서히 변화)
*/
//void APlayerCameraManager::StartLetterboxBlend(float TargetSize, float Duration, bool bEnable)
//{
//	LetterboxSizeAlpha.X = DefaultPostProcessSettings.LetterboxSize;
//	LetterboxSizeAlpha.Y = TargetSize;
//	LetterboxBlendTime = Duration;
//	LetterboxBlendTimeRemaining = Duration;
//	bLetterboxBlendTargetEnable = bEnable;
//	DefaultPostProcessSettings.bEnableLetterbox = true;  // 보간 중에는 활성화
//}

/**
* @brief Letterbox Blend 시작 (시작값과 끝값을 모두 명시적으로 지정)
*/
void APlayerCameraManager::StartLetterboxBlend(float StartSize, float TargetSize, float Duration, bool bEnable)
{
	// 시작값을 즉시 적용
	DefaultPostProcessSettings.LetterboxSize = StartSize;

	// 보간 설정
	LetterboxSizeAlpha.X = StartSize;
	LetterboxSizeAlpha.Y = TargetSize;
	LetterboxBlendTime = Duration;
	LetterboxBlendTimeRemaining = Duration;
	bLetterboxBlendTargetEnable = bEnable;
	DefaultPostProcessSettings.bEnableLetterbox = true;  // 보간 중에는 활성화
}

/**
* @brief 카메라 Modifier들을 업데이트하여 카메라 Transform 수정 (쉐이크 등)
*/
void APlayerCameraManager::UpdateCameraModifiers(float DeltaTime)
{
	// Modifier 업데이트 및 disabled된 것 제거
	for (auto it = ModifierList.begin(); it != ModifierList.end(); )
	{
		UCameraModifier* Modifier = *it;

		if (Modifier)
		{
			if (!Modifier->IsDisabled())
			{
				// 활성화된 Modifier는 적용
				Modifier->ModifyCamera(DeltaTime, ViewTarget.ViewInfo);
				++it;
			}
			else
			{
				// Disabled된 Modifier는 제거 (메모리 관리는 ObjectFactory에서)
				it = ModifierList.erase(it);
			}
		}
		else
		{
			// nullptr인 경우도 제거
			it = ModifierList.erase(it);
		}
	}
}

/**
* @brief 카메라 Modifier 추가
*/
void APlayerCameraManager::AddCameraModifier(UCameraModifier* Modifier)
{
	if (Modifier)
	{
		ModifierList.push_back(Modifier);
	}
}

/**
* @brief 카메라 Modifier 제거
*/
void APlayerCameraManager::RemoveCameraModifier(UCameraModifier* Modifier)
{
	if (Modifier)
	{
		for (auto it = ModifierList.begin(); it != ModifierList.end(); ++it)
		{
			if (*it == Modifier)
			{
				ModifierList.erase(it);
				break;
			}
		}
	}
}