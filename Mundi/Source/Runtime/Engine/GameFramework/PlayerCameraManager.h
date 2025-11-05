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

    void StartFadeInOut(float InFadeTime, float InTargetAlpha);
    void StartFadeOut(float InFadeTime, FLinearColor InFadeColor);
    void StartFadeIn(float InFadeTime);
    bool IsFade() { return FadeTimeRemaining > 0; }
    const FLinearColor& GetFadeColor() const { return FadeColor; }
    float GetFadeAmount() const { return FadeAmount; }

    void SetViewTarget(AActor* InTargetActor, float InTransitionTime = 0.0f);
    const FMinimalViewInfo& GetCameraViewInfo() { return ViewTarget.ViewInfo; }
private:
    // 페이드 인 아웃에 쓰일 배경색
    FLinearColor FadeColor;
    // 현재 페이드 알파값
    float FadeAmount;
    // 알파값 보간 위한 변수(시작, 목표 알파값 저장)
    FVector2D FadeAlpha;
    // 페이드 in out 총 시간
    float FadeTime;
    // 페이드 in out 남은 시간
    float FadeTimeRemaining = 0.0f;

    float TransitionTime = 0.0f;
    float TransitionTimeRemaining = 0.0f;

    FName CameraStyle;
    FViewTarget ViewTarget;

    // Transition 전의 타겟에 대한 ViewInfo(보간 위해 저장)
    FMinimalViewInfo PreviousViewInfo;

    TArray<UCameraModifier*> ModifierList;
};