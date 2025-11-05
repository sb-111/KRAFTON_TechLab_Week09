#include "pch.h"
#include "GameUIWidget.h"
#include "ImGui/imgui.h"
#include "USlateManager.h"

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

	// 메인 Viewport의 영역 가져오기 (게임이 실행되는 뷰포트)
	float viewportWidth, viewportHeight;
	float viewportLeft = 0.0f, viewportTop = 0.0f;

	SViewportWindow* mainViewport = USlateManager::GetInstance().GetMainViewport();
	if (mainViewport)
	{
		// 에디터 모드: Viewport 영역 사용
		FRect viewportRect = mainViewport->GetRect();
		viewportWidth = viewportRect.GetWidth();
		viewportHeight = viewportRect.GetHeight();
		viewportLeft = viewportRect.Left;
		viewportTop = viewportRect.Top;
	}
	else
	{
		// Release_StandAlone 모드: 전체 화면 사용
		ImGuiIO& io = ImGui::GetIO();
		viewportWidth = io.DisplaySize.x;
		viewportHeight = io.DisplaySize.y;
		viewportLeft = 0.0f;
		viewportTop = 0.0f;
	}

	// Viewport 영역 내에서 상단 중앙에 UI 배치
	// 툴바 높이를 고려하여 충분한 마진 확보 (에디터: 60픽셀, StandAlone: 20픽셀)
	ImVec2 windowSize = ImVec2(300, 100);
	float topMargin = mainViewport ? 60.0f : 20.0f; // 에디터는 툴바 고려, StandAlone은 작은 마진
	ImVec2 windowPos = ImVec2(
		viewportLeft + (viewportWidth - windowSize.x) * 0.5f,
		viewportTop + topMargin
	);

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

		if (AfterCollisionTime > 0.0f)
		{
			FString timeText = "Speed Down After Collision: " + std::to_string(AfterCollisionTime);
			textSize = ImGui::CalcTextSize(timeText.c_str());
			ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), timeText.c_str());
		}

		ImGui::PopFont();
	}
	ImGui::End();
}
