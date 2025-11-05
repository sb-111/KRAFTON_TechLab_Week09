#pragma once
#include "Actor.h"

class UCameraModifier;

/**
* @brief PostProcess를 설정 값을 저장하는 구조체
*/
struct FPostProcessSettings
{
    // Vignette
    float VignetteIntensity = 0.6f;
    float VignetteRadius = 0.7f;
    FLinearColor VignetteColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // 기본값: 검은색

    // Letterbox
    float LetterboxSize = 0.1f;
    FLinearColor LetterboxColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // 기본값: 검은색

    // Gamma Correction
    float Gamma = 2.2f;

    // Enable flags
    bool bEnableVignetting = false;
    bool bEnableLetterbox = false;
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

    void StartFadeInOut(float InFadeTime, float InTargetAlpha);
    void StartFadeOut(float InFadeTime, FLinearColor InFadeColor);
    void StartFadeIn(float InFadeTime);
    bool IsFade() { return FadeTimeRemaining > 0; }
    const FLinearColor& GetFadeColor() const { return FadeColor; }
    float GetFadeAmount() const { return FadeAmount; }

    //void UpdateFadeInOut(float DeltaTime);
    void UpdateLetterboxBlend(float DeltaTime);
    void UpdateVignetteBlend(float DeltaTime);
    void UpdatePostProcess(float DeltaTime);
    void SetViewTarget(AActor* InTargetActor, float InTransitionTime = 0.0f);
    const FMinimalViewInfo& GetCameraViewInfo() { return ViewTarget.ViewInfo; }

    // PostProcess 설정 접근자
    const FPostProcessSettings& GetPostProcessSettings() const { return CachedPostProcessSettings; }
    FPostProcessSettings& GetDefaultPostProcessSettings() { return DefaultPostProcessSettings; }

    // ======= 루아에서 접근하기 위한 함수들 =======
    // Vignette
    void SetVignetteIntensity(float Intensity) { DefaultPostProcessSettings.VignetteIntensity = Intensity; }
    void SetVignetteRadius(float Radius) { DefaultPostProcessSettings.VignetteRadius = Radius; }
    void SetVignetteColor(const FLinearColor& Color) { DefaultPostProcessSettings.VignetteColor = Color; }
    void EnableVignetting(bool bEnable) { DefaultPostProcessSettings.bEnableVignetting = bEnable; }

    // Letterbox
    void SetLetterboxSize(float Size) { DefaultPostProcessSettings.LetterboxSize = Size; }
    void SetLetterboxColor(const FLinearColor& Color) { DefaultPostProcessSettings.LetterboxColor = Color; }
    void EnableLetterbox(bool bEnable) { DefaultPostProcessSettings.bEnableLetterbox = bEnable; }

    // Gamma
    void SetGamma(float InGamma) { DefaultPostProcessSettings.Gamma = InGamma; }

    // ========= Post Process 효과 보간 =============

    // Vignette Blend (시간에 따른 보간)
    // 현재  값에서 목표값으로 보간
    //void StartVignetteBlend(float TargetIntensity, float TargetRadius, float Duration, bool bEnable = true);
    // 시작값과 끝값을 모두 명시적으로 지정
    void StartVignetteBlend(float StartIntensity, float StartRadius, float TargetIntensity, float TargetRadius, float Duration, bool bEnable = true);
    void StopVignetteBlend() { VignetteBlendTimeRemaining = 0.0f; }

    // Letterbox Blend (시간에 따른 보간)
    // 현재값에서 목표값으로 보간
    //void StartLetterboxBlend(float TargetSize, float Duration, bool bEnable = true);
    // 시작값과 끝값을 모두 명시적으로 지정
    void StartLetterboxBlend(float StartSize, float TargetSize, float Duration, bool bEnable = true);
    void StopLetterboxBlend() { LetterboxBlendTimeRemaining = 0.0f; }

    // ============= 계산된 Cached를 GPU에서 읽어가는데 사용
    float GetVignetteIntensity() const { return CachedPostProcessSettings.VignetteIntensity; }
    float GetVignetteRadius() const { return CachedPostProcessSettings.VignetteRadius; }
       
private:
    // Fade In Out 관련 변수들
    FLinearColor FadeColor; // 페이드 인 아웃에 쓰일 배경색
    float FadeAmount;       // 셰이더로 최종 전달될 불투명도 (매 프레임 계산하는 현재 값), 현재 페이드 알파값
    FVector2D FadeAlpha;    // x: 시작 Alpha, y: 목표 Alpha
    float FadeTime;         // 총 Fade 시간
    float FadeTimeRemaining;// 남은 Fade 시간

    // Vignette Blend 관련 변수들
    FVector2D VignetteIntensityAlpha;  // x: 시작, y: 목표
    FVector2D VignetteRadiusAlpha;     // x: 시작, y: 목표
    float VignetteBlendTime;
    float VignetteBlendTimeRemaining;
    bool bVignetteBlendTargetEnable;

    // Letterbox Blend 관련 변수들
    FVector2D LetterboxSizeAlpha;      // x: 시작, y: 목표
    float LetterboxBlendTime;
    float LetterboxBlendTimeRemaining;
    bool bLetterboxBlendTargetEnable;

    float TransitionTime = 0.0f;
    float TransitionTimeRemaining = 0.0f;

    // Camera
    FName CameraStyle;
    FViewTarget ViewTarget;

    // PostProcess 설정
    /**
    * @brief 게임 전반에 걸쳐 항상 적용되는 기본 PostProcess 설정,
    * 설정 시점: 게임 시작, 특정 이벤트 등
    */
    FPostProcessSettings DefaultPostProcessSettings;
    /**
     * @brief 매 프레임 계산되는 최종 PostProcess 설정,
     * 계산 방식: Default + Modifier들의 수정
     * SceneRender가 이걸 GPU로 보내서 적용함.
     */
    FPostProcessSettings CachedPostProcessSettings;


    // Transition 전의 타겟에 대한 ViewInfo(보간 위해 저장)
    FMinimalViewInfo PreviousViewInfo;

    TArray<UCameraModifier*> ModifierList;
};