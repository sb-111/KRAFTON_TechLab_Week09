#include "pch.h"
#include "ShapeComponent.h"
#include "World.h"
#include "WorldPhysics.h"
#include "JsonSerializer.h"

IMPLEMENT_CLASS(UShapeComponent)

void UShapeComponent::Destroy()
{
    UWorld* World = GetWorld();
    UWorldPhysics* Physics = World ? World->GetWorldPhysics() : nullptr;

    UnbindCollisionDelegates();

    if (Physics)
    {
        Physics->UnregisterCollision(this);
    }

    Super::Destroy();
}

void UShapeComponent::OnRegister(UWorld* InWorld)
{
	Super::OnRegister(InWorld);
	UpdateBound();
	BindCollisionDelegates();
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

void UShapeComponent::BindCollisionDelegates()
{
	if (BeginOverlapHandle.ID != static_cast<uint32>(-1) || EndOverlapHandle.ID != static_cast<uint32>(-1))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UWorldPhysics* Physics = World->GetWorldPhysics())
	{
		BeginOverlapHandle = Physics->OnShapeBeginOverlap().AddDynamic(this, &UShapeComponent::HandleBeginOverlap);
		EndOverlapHandle = Physics->OnShapeEndOverlap().AddDynamic(this, &UShapeComponent::HandleEndOverlap);
	}
}

void UShapeComponent::UnbindCollisionDelegates()
{
	UWorld* World = GetWorld();
	UWorldPhysics* Physics = World ? World->GetWorldPhysics() : nullptr;

	if (Physics)
	{
		if (BeginOverlapHandle.ID != static_cast<uint32>(-1))
		{
			Physics->OnShapeBeginOverlap().Remove(BeginOverlapHandle);
		}
		if (EndOverlapHandle.ID != static_cast<uint32>(-1))
		{
			Physics->OnShapeEndOverlap().Remove(EndOverlapHandle);
		}
	}

	BeginOverlapHandle = FBindingHandle();
	EndOverlapHandle = FBindingHandle();
}

void UShapeComponent::HandleBeginOverlap(UShapeComponent* A, UShapeComponent* B)
{
	if (A == this && B && B != this)
	{
		OnCollisionBegin(B);
	}
	else if (B == this && A && A != this)
	{
		OnCollisionBegin(A);
	}
}

void UShapeComponent::HandleEndOverlap(UShapeComponent* A, UShapeComponent* B)
{
	if (A == this && B && B != this)
	{
		OnCollisionEnd(B);
	}
	else if (B == this && A && A != this)
	{
		OnCollisionEnd(A);
	}
}
