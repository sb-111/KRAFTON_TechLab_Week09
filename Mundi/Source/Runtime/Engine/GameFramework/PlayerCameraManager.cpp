#include "pch.h"
#include "PlayerCameraManager.h"
#include "Actor.h"

IMPLEMENT_CLASS(APlayerCameraManager)

void APlayerCameraManager::Tick(float DeltaTime)
{
	if (ViewTarget.TargetActor.Get())
	{
		// 매번 새로 계산
		ViewTarget.ViewInfo = ViewTarget.TargetActor->CalcCamera();
	}
	// TODO: 타겟 액터가 없는 경우 처리
}

void APlayerCameraManager::SetViewTarget(AActor* InTargetActor, float TransitionTime)
{
	ViewTarget.TargetActor = InTargetActor;
}