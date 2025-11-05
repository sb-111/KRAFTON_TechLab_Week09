#include "pch.h"

#include "PlayerController.h"
#include "PlayerCameraManager.h"
#include "InputManager.h"
#include "Pawn.h"

IMPLEMENT_CLASS(APlayerController)

APlayerController::APlayerController()
	:PlayerCameraManager(NewObject<APlayerCameraManager>())
{
}

APlayerController::~APlayerController()
{
	if (PlayerCameraManager)
	{
		//PlayerCameraManager->Destroy();
		ObjectFactory::DeleteObject(PlayerCameraManager);
	}
}

// WeakPtr = 연산자가 APawn정의를 알아야 해서 cpp에 작성
void APlayerController::Possess(APawn* InPawn)
{
	Pawn = InPawn;
}

void APlayerController::Tick(float DeltaSecond)
{
	float WorldDeltaTime = DeltaSecond / CustomTimeDilation;

	ProcessInput();
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ExecuteTick(DeltaSecond);
	}
}

APlayerCameraManager* APlayerController::GetPlayerCameraManager()
{
	return PlayerCameraManager;
}

void APlayerController::ProcessInput()
{
	if (!Pawn.IsValid())
	{
		return;
	}

	UInputManager& InputManager = UInputManager::GetInstance();

	if (InputManager.IsKeyDown('W'))
	{
		Pawn->HandleThrustInput(1.0f);
	}
	if (InputManager.IsKeyReleased('W'))
	{
		Pawn->HandleThrustInput(0.0f);
	}
	if (InputManager.IsKeyDown('S'))
	{
		Pawn->HandleThrustInput(-1.0f);
	}
	if (InputManager.IsKeyReleased('S'))
	{
		Pawn->HandleThrustInput(0.0f);
	}

	if (InputManager.IsKeyDown('D'))
	{
		Pawn->HandleSteerInput(1.0f);
	}
	if (InputManager.IsKeyReleased('D'))
	{
		Pawn->HandleSteerInput(0.0f);
	}
	if (InputManager.IsKeyDown('A'))
	{
		Pawn->HandleSteerInput(-1.0f);
	}
	if (InputManager.IsKeyReleased('A'))
	{
		Pawn->HandleSteerInput(0.0f);
	}

	// 부스터 제어 키
	if (InputManager.IsKeyPressed(VK_SPACE))
	{
		Pawn->HandleBoosterInput();
	}
}