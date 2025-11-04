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

	DECLARE_DUPLICATE(APawn)
};