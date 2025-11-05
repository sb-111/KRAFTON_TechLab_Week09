#pragma once
#include "Actor.h"

class UCameraModifier;

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