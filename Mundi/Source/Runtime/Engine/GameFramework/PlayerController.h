#pragma once

#include "Actor.h"


class APawn;

class APlayerController : public AActor
{
public:
	DECLARE_CLASS(APlayerController, AActor)

	void Possess(APawn* InPawn) { Pawn = InPawn; }
	

	void Tick(float DeltaSecond) override;

	void ProcessInput();

private:

	// 빙의 대상
	APawn* Pawn = nullptr;
};