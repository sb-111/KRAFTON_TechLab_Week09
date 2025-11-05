#pragma once

#include "Actor.h"

class APawn : public AActor
{
public:

	APawn() = default;

	DECLARE_CLASS(APawn, AActor)

	// 앞뒤 인풋 처리
	virtual void HandleThrustInput(float InValue) {};
	// 좌우 인풋 처리
	virtual void HandleSteerInput(float InValue) {};

	// 오디오 제어 인풋 처리
	virtual void HandleMuteInput() {};
	virtual void HandlePlayPauseInput() {};
	virtual void HandleStopInput() {};
	virtual void HandleLeftArrowInput() {};
	virtual void HandleRightArrowInput() {};
	virtual void HandleUpArrowInput() {};
	virtual void HandleDownArrowInput() {};

	DECLARE_DUPLICATE(APawn)
};