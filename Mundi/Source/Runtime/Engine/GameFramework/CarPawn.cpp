#include "pch.h"
#include "Actor.h"
#include "CarPawn.h"
#include "StaticMeshComponent.h"
#include "ScriptComponent.h"

IMPLEMENT_CLASS(ACarPawn)
BEGIN_PROPERTIES(ACarPawn)
	MARK_AS_SPAWNABLE("카 폰", "카 폰")
END_PROPERTIES()

ACarPawn::ACarPawn()
{
	Name = "Car Pawn";
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("CarStaticMeshComponent");
	RootComponent = StaticMeshComponent;
}

void ACarPawn::HandleThrustInput(float InValue)
{
	if (ScriptComponent)
	{
		ScriptComponent->HandleThrustInput(InValue);
	}
}

void ACarPawn::HandleSteerInput(float InValue)
{
	if (ScriptComponent)
	{
		ScriptComponent->HandleSteerInput(InValue);
	}
}

void ACarPawn::BeginPlay()
{
	Super_t::BeginPlay();

	ScriptComponent = GetComponentByClass<UScriptComponent>();

}

void ACarPawn::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();
}
