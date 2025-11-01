#include "pch.h"
#include "WorldPhysics.h"
#include "CollisionBVH.h"
#include "ShapeComponent.h"

IMPLEMENT_CLASS(UWorldPhysics)

UWorldPhysics::UWorldPhysics()
{
	BVH = new FCollisionBVH();
}

UWorldPhysics::~UWorldPhysics()
{
	if (BVH)
	{
		delete BVH;
		BVH = nullptr;
	}
}

void UWorldPhysics::Clear()
{
	if (BVH)
	{
		BVH->Clear();
	}

	CollisionDirtyQueue.Empty();
	CollisionDirtySet.Empty();
}

void UWorldPhysics::RegisterCollision(UShapeComponent* InShape)
{
	if (!InShape || InShape->GetShapeType() == EShapeType::None)
	{
		return;
	}

	MarkCollisionDirty(InShape);
}

void UWorldPhysics::BulkRegisterCollision(const TArray<UShapeComponent*>& InShapes)
{
	if (InShapes.empty())
	{
		return;
	}

	for (UShapeComponent* Shape : InShapes)
	{
		if (!Shape || Shape->GetShapeType() == EShapeType::None)
		{
			continue;
		}
		CollisionDirtySet.erase(Shape);
	}

	if (BVH)
	{
		BVH->BulkUpdate(InShapes);
	}
}

void UWorldPhysics::UnregisterCollision(UShapeComponent* InShape)
{
	if (!InShape || InShape->GetShapeType() == EShapeType::None)
	{
		return;
	}

	if (BVH)
	{
		BVH->Remove(InShape);
	}

	CollisionDirtySet.erase(InShape);
}

void UWorldPhysics::MarkCollisionDirty(UShapeComponent* PhysicsObject)
{
	if (!PhysicsObject || PhysicsObject->GetShapeType() == EShapeType::None)
	{
		return;
	}

	if (CollisionDirtySet.insert(PhysicsObject).second)
	{
		CollisionDirtyQueue.push(PhysicsObject);
	}
}

void UWorldPhysics::Update(float DeltaTime)
{
	(void)DeltaTime;
	while (true)
	{
		UShapeComponent* Shape = nullptr;
		if (!CollisionDirtyQueue.Dequeue(Shape))
		{
			break;
		}

		if (CollisionDirtySet.erase(Shape) == 0)
		{
			continue;
		}

		if (!Shape || Shape->GetShapeType() == EShapeType::None)
		{
			continue;
		}

		if (BVH)
		{
			BVH->Update(Shape);
		}
	}

	if (BVH)
	{
		BVH->FlushRebuild();
	}
}

TArray<UShapeComponent*> UWorldPhysics::CollisionQuery(const UShapeComponent* PhysicsObject) const
{
	TArray<UShapeComponent*> Collisions;
	if (BVH)
	{
		if (!PhysicsObject || PhysicsObject->GetShapeType() == EShapeType::None)
		{
			return Collisions;
		}

		Collisions = BVH->Query(PhysicsObject);
	}
	return Collisions;
}
