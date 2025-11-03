#include "pch.h"
#include "GameUIWidget.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UGameUIWidget)

UGameUIWidget::UGameUIWidget()
	: UWidget("GameUI")
{
}

void UGameUIWidget::Initialize()
{
	CurrentScore = 0;
	PlayTime = 0.0f;
	bIsVisible = true;
}

void UGameUIWidget::Update()
{
	// 필요시 업데이트 로직 추가
}

void UGameUIWidget::RenderWidget()
{
	if (!bIsVisible)
	{
		return;
	}

	// 화면 상단 중앙에 고정된 UI 렌더링
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 windowSize = ImVec2(300, 100);
	ImVec2 windowPos = ImVec2((io.DisplaySize.x - windowSize.x) * 0.5f, 20);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoBackground |
							 ImGuiWindowFlags_NoScrollbar;

	if (ImGui::Begin("##GameUI", nullptr, flags))
	{
		// 반투명 배경
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 min = ImGui::GetWindowPos();
		ImVec2 max = ImVec2(min.x + windowSize.x, min.y + windowSize.y);
		drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 150), 10.0f);

		// 점수 표시
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

		FString scoreText = "Score: " + std::to_string(CurrentScore);
		ImVec2 textSize = ImGui::CalcTextSize(scoreText.c_str());
		ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), scoreText.c_str());

		// 시간 표시
		int minutes = static_cast<int>(PlayTime) / 60;
		int seconds = static_cast<int>(PlayTime) % 60;
		FString timeText = "Time: " + std::to_string(minutes) + ":" +
						   (seconds < 10 ? "0" : "") + std::to_string(seconds);
		textSize = ImGui::CalcTextSize(timeText.c_str());
		ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), timeText.c_str());

		ImGui::PopFont();
	}
	ImGui::End();
}
