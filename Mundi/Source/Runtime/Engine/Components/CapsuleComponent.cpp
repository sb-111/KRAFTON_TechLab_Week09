#include "pch.h"
#include "CapsuleComponent.h"
#include "BoxComponent.h"
#include "SphereComponent.h"
#include "Collision.h"
#include "WorldPhysics.h"
#include "JsonSerializer.h"

IMPLEMENT_CLASS(UCapsuleComponent)

BEGIN_PROPERTIES(UCapsuleComponent)
	MARK_AS_COMPONENT("캡슐 컴포넌트", "캡슐 형태의 충돌 컴포넌트입니다.")
END_PROPERTIES()

UCapsuleComponent::UCapsuleComponent()
{
	CapsuleHalfHeight = 1.0f;
	CapsuleRadius = 1.0f;
	ShapeColor = FLinearColor(1.0f, 0.34f, 0.28f);
	bDrawOnlyIfSelected = false;
	UpdateBound();
}

void UCapsuleComponent::SetCapsuleHalfHeight(const float InHalfHeight)
{
	CapsuleHalfHeight = FMath::Max(0.0f, InHalfHeight);
	UpdateBound();
}

void UCapsuleComponent::SetCapsuleRadius(const float InRadius)
{
	CapsuleRadius = FMath::Max(0.0f, InRadius);
	UpdateBound();
}

FAABB UCapsuleComponent::GetWorldAABB()
{
	const FVector Axis = CachedBound.GetAxis();
	const FVector Center = CachedBound.GetCenter();
	const float HalfHeight = CachedBound.GetHalfHeight();
	const float Radius = CachedBound.GetRadius();

	const FVector CapStart = Center - Axis * HalfHeight;
	const FVector CapEnd = Center + Axis * HalfHeight;

	FVector Min = CapStart.ComponentMin(CapEnd) - FVector(Radius, Radius, Radius);
	FVector Max = CapStart.ComponentMax(CapEnd) + FVector(Radius, Radius, Radius);

	return FAABB(Min, Max);
}

bool UCapsuleComponent::Intersects(const UShapeComponent* Other) const
{
	switch (Other->GetShapeType())
	{
		case EShapeType::Box:
		{
			const UBoxComponent* OtherBox = Cast<UBoxComponent>(Other);
			return Collision::Intersects(OtherBox->GetOBB(), CachedBound);
		}
		case EShapeType::Sphere:
		{
			const USphereComponent* OtherSphere = Cast<USphereComponent>(Other);
			return Collision::Intersects(OtherSphere->GetBoundingSphere(), CachedBound);
		}
		case EShapeType::Capsule:
		{
			const UCapsuleComponent* OtherCapsule = Cast<UCapsuleComponent>(Other);
			return CachedBound.Intersects(OtherCapsule->GetBoundingCapsule());
		}
		default:
		{
			UE_LOG("UCapsuleComponent::Intersects: Unsupported shape type for collision detection.");
			return false;
		}
	}
}

void UCapsuleComponent::OnTransformUpdated()
{
	Super::OnTransformUpdated();
	UpdateBound();
}

void UCapsuleComponent::UpdateBound()
{
	const FVector WorldPos = GetWorldLocation();
	const FQuat WorldRot = GetWorldRotation();
	FVector Axis = WorldRot.GetUpVector();
	Axis.Normalize();

	if (Axis.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		Axis = FVector(0.0f, 0.0f, 1.0f);
	}

	CachedBound = FBoundingCapsule(WorldPos, Axis, CapsuleHalfHeight, CapsuleRadius);

	if (UWorld* World = GetWorld())
	{
		if (UWorldPhysics* Physics = World->GetWorldPhysics())
		{
			Physics->MarkCollisionDirty(this);
		}
	}
}

void UCapsuleComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		float LoadedHalfHeight = CapsuleHalfHeight;
		if (FJsonSerializer::ReadFloat(InOutHandle, "CapsuleHalfHeight", LoadedHalfHeight, CapsuleHalfHeight, false))
		{
			CapsuleHalfHeight = FMath::Max(0.0f, LoadedHalfHeight);
		}

		float LoadedRadius = CapsuleRadius;
		if (FJsonSerializer::ReadFloat(InOutHandle, "CapsuleRadius", LoadedRadius, CapsuleRadius, false))
		{
			CapsuleRadius = FMath::Max(0.0f, LoadedRadius);
		}
	}
	else
	{
		InOutHandle["CapsuleHalfHeight"] = CapsuleHalfHeight;
		InOutHandle["CapsuleRadius"] = CapsuleRadius;
	}
}

void UCapsuleComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	UpdateBound();
}

void UCapsuleComponent::OnSerialized()
{
	Super::OnSerialized();
	UpdateBound();
}
