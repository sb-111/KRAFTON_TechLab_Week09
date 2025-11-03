#include "pch.h"
#include "GameOverWidget.h"
#include "ImGui/imgui.h"
#include "USlateManager.h"

IMPLEMENT_CLASS(UGameOverWidget)

UGameOverWidget::UGameOverWidget()
	: UWidget("GameOver")
{
}

void UGameOverWidget::Initialize()
{
	FinalScore = 0;
	bIsVisible = false;
	RestartCallback = sol::nil;
}

void UGameOverWidget::Update()
{
	// 필요시 업데이트 로직 추가
}

void UGameOverWidget::RenderWidget()
{
	if (!bIsVisible)
	{
		return;
	}

	// 메인 Viewport의 영역 가져오기 (게임이 실행되는 뷰포트)
	SViewportWindow* mainViewport = USlateManager::GetInstance().GetMainViewport();
	if (!mainViewport)
	{
		return; // Viewport가 없으면 렌더링하지 않음
	}

	FRect viewportRect = mainViewport->GetRect();
	float viewportWidth = viewportRect.GetWidth();
	float viewportHeight = viewportRect.GetHeight();

	// Viewport 영역 내에서 중앙에 모달 형태로 표시
	ImVec2 windowSize = ImVec2(400, 300);
	ImVec2 windowPos = ImVec2(
		viewportRect.Left + (viewportWidth - windowSize.x) * 0.5f,
		viewportRect.Top + (viewportHeight - windowSize.y) * 0.5f
	);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoScrollbar;

	if (ImGui::Begin("##GameOver", nullptr, flags))
	{
		// 배경
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 min = ImGui::GetWindowPos();
		ImVec2 max = ImVec2(min.x + windowSize.x, min.y + windowSize.y);
		drawList->AddRectFilled(min, max, IM_COL32(20, 20, 20, 240), 15.0f);

		// 제목
		ImGui::SetCursorPosY(50);
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

		const char* titleText = "GAME OVER";
		ImVec2 titleSize = ImGui::CalcTextSize(titleText);
		ImGui::SetCursorPosX((windowSize.x - titleSize.x) * 0.5f);
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), titleText);

		// 최종 점수
		ImGui::SetCursorPosY(120);
		FString scoreText = "Final Score: " + std::to_string(FinalScore);
		ImVec2 scoreSize = ImGui::CalcTextSize(scoreText.c_str());
		ImGui::SetCursorPosX((windowSize.x - scoreSize.x) * 0.5f);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), scoreText.c_str());

		// 재시작 버튼
		ImGui::SetCursorPosY(180);
		ImVec2 buttonSize = ImVec2(200, 50);
		ImGui::SetCursorPosX((windowSize.x - buttonSize.x) * 0.5f);

		if (ImGui::Button("Restart Game", buttonSize))
		{
			InvokeRestartCallback();
		}

		ImGui::PopFont();
	}
	ImGui::End();
}

void UGameOverWidget::SetRestartCallback(sol::function InCallback)
{
	// 이전 콜백을 명시적으로 정리 (죽은 Lua State 참조 방지)
	if (RestartCallback.valid())
	{
		RestartCallback = sol::nil;
	}

	// 새로운 콜백 설정
	RestartCallback = InCallback;
}

void UGameOverWidget::InvokeRestartCallback()
{
	if (RestartCallback.valid())
	{
		try
		{
			RestartCallback();
		}
		catch (const sol::error& e)
		{
			UE_LOG("GameOverWidget: Error invoking restart callback: %s", e.what());
		}
	}
	else
	{
		UE_LOG("GameOverWidget: Warning: Restart callback not set!");
	}
}
