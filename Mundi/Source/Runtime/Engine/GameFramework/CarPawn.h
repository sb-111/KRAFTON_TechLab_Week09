#pragma once
#include "Pawn.h"
#include "StaticMeshComponent.h"
#include "sol/sol.hpp"

class UScriptComponent;

// HandleInput을 받을 수 있는 클래스
class ACarPawn : public APawn
{
public:
	DECLARE_CLASS(ACarPawn, APawn)
	GENERATED_REFLECTION_BODY()
	ACarPawn();

	void HandleThrustInput(float InValue) override;
	void HandleSteerInput(float InValue) override;

	void HandleMuteInput() override;
	void HandlePlayPauseInput() override;
	void HandleStopInput() override;
	void HandleLeftArrowInput() override;
	void HandleRightArrowInput() override;
	void HandleUpArrowInput() override;
	void HandleDownArrowInput() override;

	void BeginPlay() override;

	DECLARE_DUPLICATE(ACarPawn)

	void DuplicateSubObjects() override;
private:
	UScriptComponent* ScriptComponent = nullptr;
	UStaticMeshComponent* StaticMeshComponent = nullptr;
};