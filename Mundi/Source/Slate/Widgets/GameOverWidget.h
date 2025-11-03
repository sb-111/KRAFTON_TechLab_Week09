#pragma once
#include "Widget.h"
#include "sol/sol.hpp"

/**
 * @brief 게임 오버 화면을 표시하는 위젯
 * 최종 점수와 재시작 버튼을 포함
 */
class UGameOverWidget : public UWidget
{
public:
	DECLARE_CLASS(UGameOverWidget, UWidget)

	UGameOverWidget();
	~UGameOverWidget() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// 최종 점수 설정
	void SetFinalScore(int32 InScore) { FinalScore = InScore; }
	int32 GetFinalScore() const { return FinalScore; }

	// 가시성
	void SetVisible(bool bInVisible) { bIsVisible = bInVisible; }
	bool IsWidgetVisible() const { return bIsVisible; }

	// 재시작 콜백 설정 (Lua 함수)
	void SetRestartCallback(sol::function InCallback);
	bool HasRestartCallback() const { return RestartCallback.valid(); }
	void InvokeRestartCallback();

private:
	int32 FinalScore = 0;
	bool bIsVisible = false;
	sol::function RestartCallback; // Lua 재시작 콜백 함수
};
