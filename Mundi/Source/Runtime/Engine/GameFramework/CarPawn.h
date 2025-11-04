#pragma once
#include "Pawn.h"
#include "StaticMeshComponent.h"
#include "sol/sol.hpp"

class UScriptComponent;

class ACarPawn : public APawn
{
public:
	DECLARE_CLASS(ACarPawn, APawn)
	GENERATED_REFLECTION_BODY()
	ACarPawn();

	void HandleThrustInput(float InValue) override;
	void HandleSteerInput(float InValue) override;

	DECLARE_DUPLICATE(ACarPawn)

	void DuplicateSubObjects() override;
private:
	UScriptComponent* ScriptComponent = nullptr;
	UStaticMeshComponent* StaticMeshComponent = nullptr;
};