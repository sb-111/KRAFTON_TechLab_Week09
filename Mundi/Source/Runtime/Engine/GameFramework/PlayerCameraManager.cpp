#include "pch.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
#include "Actor.h"

IMPLEMENT_CLASS(APlayerCameraManager)
void APlayerCameraManager::Tick(float DeltaTime)
{
	UpdateViewInfo();
}

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