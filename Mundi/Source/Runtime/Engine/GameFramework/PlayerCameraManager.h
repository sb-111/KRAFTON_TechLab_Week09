#pragma once
#include "Actor.h"
#include "CameraComponent.h"

class UCameraModifier;

struct FMinimalViewInfo
{
    FVector Location;
    FQuat Rotation;
    // 이번 발제에서는 뷰포트 aspect를 그대로 쓸 것 같은데 일단 추가함
    float Aspect;
    float ZNear;
    float ZFar;
    float Fov;
    ECameraProjectionMode ProjectionMode = ECameraProjectionMode::Perspective;
};

struct FViewTarget
{
    // 타겟에 카메라 컴포넌트가 있는 경우 그 카메라 컴포넌트로 값 설정, 없는 경우 기본값
    TWeakPtr<AActor> TargetActor;

    FMinimalViewInfo ViewInfo;

};


class APlayerCameraManager : public AActor
{
public:

    DECLARE_CLASS(APlayerCameraManager, AActor)

    void Tick(float DeltaTime) override;
    void UpdateViewInfo();
    void SetViewTarget(AActor* InTargetActor, float TransitionTime = 0.0f);
    const FMinimalViewInfo& GetCameraViewInfo() { return ViewTarget.ViewInfo; }
private:
    FLinearColor FadeColor;
    float FadeAmount;
    FVector2D FadeAlpha;
    float FadeTime;
    float FadeTimeRemaining;

    FName CameraStyle;
    FViewTarget ViewTarget;

    TArray<TWeakPtr<UCameraModifier>> ModifierList;
};