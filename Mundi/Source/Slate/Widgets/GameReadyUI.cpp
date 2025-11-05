#include "pch.h"
#include "GameReadyUI.h"
#include "ImGui/imgui.h"
#include "USlateManager.h"
#include "Texture.h"
#include "ResourceManager.h"
#include <utility>
#include <algorithm>

IMPLEMENT_CLASS(UGameReadyUI)

UGameReadyUI::UGameReadyUI()
	: UWidget("GameReadyUI")
{
}

void UGameReadyUI::Initialize()
{
	bIsVisible = true;
	StartCallback = nullptr;

	if (!TitleTexture)
	{
		TitleTexture = UResourceManager::GetInstance().Load<UTexture>("Data/Textures/Title.png");
		if (!TitleTexture)
		{
			UE_LOG("GameReadyUI: Failed to load title texture: Data/Textures/Title.png");
		}
	}
}

void UGameReadyUI::Update()
{
	// 필요 시 업데이트 로직 추가
}

void UGameReadyUI::RenderWidget()
{
	if (!bIsVisible)
	{
		return;
	}

	float viewportWidth = 0.0f, viewportHeight = 0.0f;
	float viewportLeft = 0.0f, viewportTop = 0.0f;

	SViewportWindow* mainViewport = USlateManager::GetInstance().GetMainViewport();
	if (mainViewport)
	{
		FRect viewportRect = mainViewport->GetRect();
		viewportWidth = viewportRect.GetWidth();
		viewportHeight = viewportRect.GetHeight();
		viewportLeft = viewportRect.Left;
		viewportTop = viewportRect.Top;
	}
	else
	{
		ImGuiIO& io = ImGui::GetIO();
		viewportWidth = io.DisplaySize.x;
		viewportHeight = io.DisplaySize.y;
	}
	
	ImVec2 windowPos(
		viewportLeft + (viewportWidth - WindowSize.x) * 0.5f,
		viewportTop + (viewportHeight - WindowSize.y) * 0.5f
	);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoScrollbar;

	if (ImGui::Begin("##GameReady", nullptr, flags))
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 min = ImGui::GetWindowPos();
		ImVec2 max(min.x + WindowSize.x, min.y + WindowSize.y);
		drawList->AddRectFilled(min, max, IM_COL32(10, 10, 10, 230), 16.0f);

		float cursorY = 0.0f;

		if (TitleTexture && TitleTexture->GetShaderResourceView())
		{
			const float textureWidth = static_cast<float>(TitleTexture->GetWidth());
			const float textureHeight = static_cast<float>(TitleTexture->GetHeight());

			if (textureWidth > 0.0f && textureHeight > 0.0f)
			{
				const float maxImageWidth = WindowSize.x - 40.0f;
				const float scale = std::min(maxImageWidth / textureWidth, 1.0f) * 1.2f;
				const float imageWidth = textureWidth * scale;
				const float imageHeight = textureHeight * scale;

				const float imageX = (WindowSize.x - imageWidth) * 0.5f;
				ImGui::SetCursorPos(ImVec2(imageX, cursorY));
				ImGui::Image(
					(ImTextureID)TitleTexture->GetShaderResourceView(),
					ImVec2(imageWidth, imageHeight)
				);

				cursorY += imageHeight + 10.0f;
			}
		}

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

		const char* guideText = "제한 시간 안에 장애물을 피해 목표 지점까지 달리세요!";
		ImVec2 guideSize = ImGui::CalcTextSize(guideText);
		ImGui::SetCursorPos(ImVec2((WindowSize.x - guideSize.x) * 0.5f, cursorY));
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), guideText);

		cursorY += guideSize.y + 35.0f;

		ImVec2 buttonSize(220.0f, 50.0f);
		ImGui::SetCursorPos(ImVec2((WindowSize.x - buttonSize.x) * 0.5f, cursorY));

		if (ImGui::Button("Start Game", buttonSize))
		{
			InvokeStartCallback();
		}

		ImGui::PopFont();
	}
	ImGui::End();
}

void UGameReadyUI::SetStartCallback(std::function<void()> InCallback)
{
	StartCallback = std::move(InCallback);
}

void UGameReadyUI::InvokeStartCallback()
{
	if (StartCallback)
	{
		StartCallback();
	}
}
