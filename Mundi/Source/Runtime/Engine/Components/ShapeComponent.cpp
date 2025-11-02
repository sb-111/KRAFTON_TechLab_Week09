#include "pch.h"
#include "ShapeComponent.h"
#include "World.h"
#include "WorldPhysics.h"

IMPLEMENT_CLASS(UShapeComponent)

void UShapeComponent::Destroy()
{
    Super::Destroy();

    if (UWorld* World = GetWorld())
    {
        if (UWorldPhysics* Physics = World->GetWorldPhysics())
        {
            Physics->UnregisterCollision(this);
        }
       
    }
}

void UShapeComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
}

void UShapeComponent::OnSerialized()
{
    Super::OnSerialized();
}

bool UShapeComponent::Intersects(const UShapeComponent* A, const UShapeComponent* B)
{
	if (A->GetShapeType() == EShapeType::None || B->GetShapeType() == EShapeType::None) return false;

	return A->Intersects(B);
}
