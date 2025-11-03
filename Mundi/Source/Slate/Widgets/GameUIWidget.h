#pragma once
#include "Widget.h"

/**
 * @brief 게임 중 점수 및 시간을 표시하는 위젯
 * 게임 플레이 중에 화면에 표시되는 UI
 */
class UGameUIWidget : public UWidget
{
public:
	DECLARE_CLASS(UGameUIWidget, UWidget)

	UGameUIWidget();
	~UGameUIWidget() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// 점수 관리
	void SetScore(int32 InScore) { CurrentScore = InScore; }
	int32 GetScore() const { return CurrentScore; }
	void AddScore(int32 DeltaScore) { CurrentScore += DeltaScore; }

	// 시간 관리
	void SetPlayTime(float InTime) { PlayTime = InTime; }
	float GetPlayTime() const { return PlayTime; }

	// 가시성
	void SetVisible(bool bInVisible) { bIsVisible = bInVisible; }
	bool IsWidgetVisible() const { return bIsVisible; }

private:
	int32 CurrentScore = 0;
	float PlayTime = 0.0f;
	bool bIsVisible = true;
};
