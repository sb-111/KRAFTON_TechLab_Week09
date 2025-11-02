#include "pch.h"
#include "ShapeComponent.h"
#include "World.h"
#include "WorldPhysics.h"
#include "JsonSerializer.h"

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

void UShapeComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FVector4 ColorVec = ShapeColor.ToFVector4();
		if (FJsonSerializer::ReadVector4(InOutHandle, "ShapeColor", ColorVec, ColorVec, false))
		{
			ShapeColor = FLinearColor(ColorVec);
		}

		FJsonSerializer::ReadBool(InOutHandle, "bDrawOnlyIfSelected", bDrawOnlyIfSelected, bDrawOnlyIfSelected, false);
	}
	else
	{
		InOutHandle["ShapeColor"] = FJsonSerializer::Vector4ToJson(ShapeColor.ToFVector4());
		InOutHandle["bDrawOnlyIfSelected"] = bDrawOnlyIfSelected;
	}
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
