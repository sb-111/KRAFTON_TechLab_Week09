#include "pch.h"

#include "PlayerController.h"
#include "InputManager.h"
#include "Pawn.h"

void APlayerController::Tick(float DeltaSecond)
{
	ProcessInput();
}

void APlayerController::ProcessInput()
{
	if (!Pawn)
	{
		return;
	}

	UInputManager& InputManager = UInputManager::GetInstance();

	if (InputManager.IsKeyDown('W'))
	{
		Pawn->HandleThrustInput(1.0f);
	}
	if (InputManager.IsKeyDown('S'))
	{
		Pawn->HandleThrustInput(-1.0f);
	}

	if (InputManager.IsKeyDown('D'))
	{
		Pawn->HandleSteerInput(1.0f);
	}
	if (InputManager.IsKeyDown('A'))
	{
		Pawn->HandleSteerInput(-1.0f);
	}
}