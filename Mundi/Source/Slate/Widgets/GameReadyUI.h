#pragma once
#include "Widget.h"
#include <functional>

/**
 * @brief 게임 시작 전 표시되는 준비 UI
 * PIE 및 Standalone 모드에서 시작 버튼을 통해 게임 진행을 제어한다.
 */
class UGameReadyUI : public UWidget
{
public:
	DECLARE_CLASS(UGameReadyUI, UWidget)

	UGameReadyUI();
	~UGameReadyUI() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	void SetVisible(bool bInVisible) { bIsVisible = bInVisible; }
	bool IsVisible() const { return bIsVisible; }

	void SetStartCallback(std::function<void()> InCallback);
	bool HasStartCallback() const { return StartCallback != nullptr; }
	void InvokeStartCallback();

private:
	bool bIsVisible = false;
	std::function<void()> StartCallback;
};
