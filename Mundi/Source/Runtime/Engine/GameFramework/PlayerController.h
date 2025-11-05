#pragma once

#include "Actor.h"
class APawn;
class APlayerCameraManager;

class APlayerController : public AActor
{
public:
	DECLARE_CLASS(APlayerController, AActor)

	APlayerController();
	~APlayerController();

	void Possess(APawn* InPawn);
	
	void Tick(float DeltaSecond) override;

	APlayerCameraManager* GetPlayerCameraManager();

	void ProcessInput();

private:

	// 빙의 대상
	TWeakPtr<APawn> Pawn;

	TWeakPtr<APlayerCameraManager> PlayerCameraManager;
};