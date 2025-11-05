#include "pch.h"
#include "SplashScreen.h"
#include "Texture.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(USplashScreen)

USplashScreen::USplashScreen()
{
}

USplashScreen::~USplashScreen()
{
    if (LogoTexture)
    {
        DeleteObject(LogoTexture);
        LogoTexture = nullptr;
    }
}

bool USplashScreen::Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, const FString& LogoPath)
{
    D3DDevice = Device;
    D3DDeviceContext = Context;

    // 로고 텍스처 로드
    LogoTexture = UResourceManager::GetInstance().Load<UTexture>(LogoPath);
    if (!LogoTexture)
    {
        UE_LOG("SplashScreen: Failed to load logo texture: %s", LogoPath.c_str());
        bIsFinished = true;
        bIsActive = false;
        return false;
    }

    UE_LOG("SplashScreen: Logo loaded successfully");
    ElapsedTime = 0.0f;
    CurrentAlpha = 1.0f;
    bIsActive = true;
    bIsFinished = false;

    return true;
}

void USplashScreen::Update(float DeltaTime)
{
    if (bIsFinished || !bIsActive)
    {
        return;
    }

    ElapsedTime += DeltaTime;

    // 1초 표시 후 페이드 아웃 시작
    if (ElapsedTime < DisplayDuration)
    {
        // 완전히 불투명
        CurrentAlpha = 1.0f;
    }
    else if (ElapsedTime < TotalDuration)
    {
        // 페이드 아웃
        float fadeElapsed = ElapsedTime - DisplayDuration;
        CurrentAlpha = 1.0f - (fadeElapsed / FadeDuration);
        CurrentAlpha = std::max(0.0f, CurrentAlpha);
    }
    else
    {
        // 완료
        CurrentAlpha = 0.0f;
        bIsFinished = true;
        bIsActive = false;
    }
}

void USplashScreen::Render()
{
    if (!bIsActive || !LogoTexture || CurrentAlpha <= 0.0f)
    {
        return;
    }

    // 전체 화면 크기 가져오기
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    // 로고 텍스처의 원본 크기
    float logoWidth = static_cast<float>(LogoTexture->GetWidth());
    float logoHeight = static_cast<float>(LogoTexture->GetHeight());

    // 화면 중앙에 꽉채워서 표시 가로 세로 중 작은거를 기준으로
    float targetWidth = displaySize.x;
    float targetHeight = displaySize.y;
    float scaleX = targetWidth / logoWidth;
    float scaleY = targetHeight / logoHeight;
    float scale = std::max(scaleX, scaleY);
    targetWidth = logoWidth * scale;
    targetHeight = logoHeight * scale;

    // 중앙 위치 계산
    ImVec2 imagePos = ImVec2(
        (displaySize.x - targetWidth) * 0.5f,
        (displaySize.y - targetHeight) * 0.5f
    );

    // 전체 화면을 덮는 검은 배경 (알파 적용)
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(displaySize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNavFocus |
                             ImGuiWindowFlags_NoInputs;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin("##SplashScreen", nullptr, flags))
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 검은 배경 (페이드 효과 적용)
        ImU32 bgColor = IM_COL32(35, 31, 32, static_cast<int>(255 * CurrentAlpha));
        drawList->AddRectFilled(ImVec2(0, 0), displaySize, bgColor);

        // 로고 이미지 (페이드 효과 적용)
        ID3D11ShaderResourceView* srv = LogoTexture->GetShaderResourceView();
        if (srv)
        {
            ImU32 tintColor = IM_COL32(255, 255, 255, static_cast<int>(255 * CurrentAlpha));
            drawList->AddImage(
                (ImTextureID)srv,
                imagePos,
                ImVec2(imagePos.x + targetWidth, imagePos.y + targetHeight),
                ImVec2(0, 0),
                ImVec2(1, 1),
                tintColor
            );
        }
    }
    ImGui::End();

    ImGui::PopStyleVar();
}
