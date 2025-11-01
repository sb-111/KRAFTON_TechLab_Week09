#include "pch.h"
#include "PhysicsManager.h"
#include "CollisionBVH.h"
#include "ShapeComponent.h"

IMPLEMENT_CLASS(UPhysicsManager)

UPhysicsManager::UPhysicsManager()
{
	BVH = new FCollisionBVH();
}

UPhysicsManager::~UPhysicsManager()
{
	if (BVH)
	{
		delete BVH;
		BVH = nullptr;
	}
}

void UPhysicsManager::Clear()
{
	if (BVH)
	{
		BVH->Clear();
	}

	CollisionDirtyQueue.Empty();
	CollisionDirtySet.Empty();
}

void UPhysicsManager::RegisterCollision(UShapeComponent* InShape)
{
	MarkCollisionDirty(InShape);
}

void UPhysicsManager::BulkRegisterCollision(const TArray<UShapeComponent*>& InShapes)
{
	if (InShapes.empty())
	{
		return;
	}

	for (UShapeComponent* Shape : InShapes)
	{
		if (!Shape)
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

void UPhysicsManager::UnregisterCollision(UShapeComponent* InShape)
{
	if (!InShape)
	{
		return;
	}

	if (BVH)
	{
		BVH->Remove(InShape);
	}

	CollisionDirtySet.erase(InShape);
}

void UPhysicsManager::MarkCollisionDirty(UShapeComponent* PhysicsObject)
{
	if (!PhysicsObject)
	{
		return;
	}

	if (CollisionDirtySet.insert(PhysicsObject).second)
	{
		CollisionDirtyQueue.push(PhysicsObject);
	}
}

void UPhysicsManager::Update(float DeltaTime)
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

		if (!Shape)
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
