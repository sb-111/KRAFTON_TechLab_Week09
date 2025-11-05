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
	// 부스터 인풋 처리
	virtual void HandleBoosterInput() {};

	DECLARE_DUPLICATE(APawn)
};