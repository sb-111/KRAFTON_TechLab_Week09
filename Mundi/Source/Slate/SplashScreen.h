#pragma once
#include "Object.h"
#include <d3d11.h>

class UTexture;

/**
 * @brief 스플래시 스크린 (로고 표시 및 페이드 아웃)
 */
class USplashScreen : public UObject
{
public:
    DECLARE_CLASS(USplashScreen, UObject)

    USplashScreen();
    ~USplashScreen() override;

    // 초기화 (로고 이미지 경로)
    bool Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, const FString& LogoPath);

    // 업데이트 (델타타임)
    void Update(float DeltaTime);

    // 렌더링
    void Render();

    // 스플래시 스크린이 끝났는지 확인
    bool IsFinished() const { return bIsFinished; }

    // 스플래시 스크린 활성화 여부
    bool IsActive() const { return bIsActive; }

private:
    // 로고 텍스처
    UTexture* LogoTexture = nullptr;

    // 타이밍
    float ElapsedTime = 0.0f;
    float DisplayDuration = 1.0f;   // 로고 표시 시간 (1초)
    float FadeDuration = 1.0f;      // 페이드 아웃 시간 (1초)
    float TotalDuration = 2.0f;     // 전체 시간 (2초)

    // 상태
    bool bIsActive = true;
    bool bIsFinished = false;

    // 알파 값 (페이드 아웃)
    float CurrentAlpha = 1.0f;

    // D3D 리소스
    ID3D11Device* D3DDevice = nullptr;
    ID3D11DeviceContext* D3DDeviceContext = nullptr;
};
