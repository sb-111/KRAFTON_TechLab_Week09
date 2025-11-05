#include "pch.h"
#include "GameReadyUI.h"
#include "ImGui/imgui.h"
#include "USlateManager.h"
#include <utility>

IMPLEMENT_CLASS(UGameReadyUI)

UGameReadyUI::UGameReadyUI()
	: UWidget("GameReadyUI")
{
}

void UGameReadyUI::Initialize()
{
	bIsVisible = true;
	StartCallback = nullptr;
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

	ImVec2 windowSize(420.0f, 240.0f);
	ImVec2 windowPos(
		viewportLeft + (viewportWidth - windowSize.x) * 0.5f,
		viewportTop + (viewportHeight - windowSize.y) * 0.5f
	);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoScrollbar;

	if (ImGui::Begin("##GameReady", nullptr, flags))
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 min = ImGui::GetWindowPos();
		ImVec2 max(min.x + windowSize.x, min.y + windowSize.y);
		drawList->AddRectFilled(min, max, IM_COL32(10, 10, 10, 230), 16.0f);

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

		const char* titleText = "READY TO PLAY?";
		ImVec2 titleSize = ImGui::CalcTextSize(titleText);
		ImGui::SetCursorPosY(50.0f);
		ImGui::SetCursorPosX((windowSize.x - titleSize.x) * 0.5f);
		ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), titleText);

		const char* guideText = "Press the button below to start the game.";
		ImVec2 guideSize = ImGui::CalcTextSize(guideText);
		ImGui::SetCursorPosY(110.0f);
		ImGui::SetCursorPosX((windowSize.x - guideSize.x) * 0.5f);
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), guideText);

		ImVec2 buttonSize(220.0f, 50.0f);
		ImGui::SetCursorPosY(160.0f);
		ImGui::SetCursorPosX((windowSize.x - buttonSize.x) * 0.5f);

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
