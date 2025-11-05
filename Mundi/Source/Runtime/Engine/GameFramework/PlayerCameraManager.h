#pragma once
#include "Actor.h"
#include "CameraComponent.h"

class UCameraModifier;

/**
* @brief PostProcess를 설정 값을 저장하는 구조체
*/
struct FPostProcessSettings
{
    // Vignette
    float VignetteIntensity = 0.6f;
    float VignetteRadius = 0.7f;

    // Letterbox
    float LetterboxSize = 0.1f;

    // Gamma Correction
    float Gamma = 2.2f;

    // Enable flags
    bool bEnableVignetting = true;
    bool bEnableLetterbox = true;
};

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
    //void UpdateFadeInOut(float DeltaTime);
    void UpdateViewInfo();
    void UpdateLetterboxBlend(float DeltaTime);
    void UpdateVignetteBlend(float DeltaTime);
    void UpdatePostProcess(float DeltaTime);
    void SetViewTarget(AActor* InTargetActor, float TransitionTime = 0.0f);
    const FMinimalViewInfo& GetCameraViewInfo() { return ViewTarget.ViewInfo; }

    // PostProcess 설정 접근자
    const FPostProcessSettings& GetPostProcessSettings() const { return CachedPostProcessSettings; }
    FPostProcessSettings& GetDefaultPostProcessSettings() { return DefaultPostProcessSettings; }

    // ======= 루아에서 접근하기 위한 함수들 =======
    // Vignette
    void SetVignetteIntensity(float Intensity) { DefaultPostProcessSettings.VignetteIntensity = Intensity; }
    void SetVignetteRadius(float Radius) { DefaultPostProcessSettings.VignetteRadius = Radius; }
    void EnableVignetting(bool bEnable) { DefaultPostProcessSettings.bEnableVignetting = bEnable; }

    // Letterbox
    void SetLetterboxSize(float Size) { DefaultPostProcessSettings.LetterboxSize = Size; }
    void EnableLetterbox(bool bEnable) { DefaultPostProcessSettings.bEnableLetterbox = bEnable; }

    // Gamma
    void SetGamma(float InGamma) { DefaultPostProcessSettings.Gamma = InGamma; }

    // ========= Post Process 효과 보간 =============

    // Vignette Blend (시간에 따른 보간)
    void StartVignetteBlend(float TargetIntensity, float TargetRadius, float Duration, bool bEnable = true);
    void StopVignetteBlend() { VignetteBlendTimeRemaining = 0.0f; }

    // Letterbox Blend (시간에 따른 보간)
    void StartLetterboxBlend(float TargetSize, float Duration, bool bEnable = true);
    void StopLetterboxBlend() { LetterboxBlendTimeRemaining = 0.0f; }

    // ============= 계산된 Cached를 GPU에서 읽어가는데 사용
    float GetVignetteIntensity() const { return CachedPostProcessSettings.VignetteIntensity; }
    float GetVignetteRadius() const { return CachedPostProcessSettings.VignetteRadius; }
       
private:
    // Fade In Out 관련 변수들
    FLinearColor FadeColor; // 페이드 색상
    float FadeAmount;       // 셰이더로 최종 전달될 불투명도 (매 프레임 계산하는 현재 값)
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

    // Camera
    FName CameraStyle;
    FViewTarget ViewTarget;
    TArray<UCameraModifier*> ModifierList;

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


};