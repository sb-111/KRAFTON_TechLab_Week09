#pragma once
#include "Object.h"
#include "Vector.h"

class UShapeComponent;
class FCollisionBVH;

class UPhysicsManager : public UObject
{
public:
	DECLARE_CLASS(UPhysicsManager, UObject)
	UPhysicsManager();
	~UPhysicsManager() override;

	void Clear();

	void RegisterCollision(UShapeComponent* InShape);
	void BulkRegisterCollision(const TArray<UShapeComponent*>& InShapes);
	void UnregisterCollision(UShapeComponent* InShape);

	void MarkCollisionDirty(UShapeComponent* PhysicsObject);
	
	void Update(float DeltaTime);

	TArray<UShapeComponent*> CollisionQuery(const UShapeComponent* PhysicsObject) const;

private:
	// 싱글톤 
	UPhysicsManager(const UPhysicsManager&) = delete;
	UPhysicsManager& operator=(const UPhysicsManager&) = delete;

	TQueue<UShapeComponent*> CollisionDirtyQueue; // 추가 혹은 갱신이 필요한 요소의 대기 큐
	TSet<UShapeComponent*> CollisionDirtySet;     // 더티 큐 중복 추가를 막기 위한 Set

	FCollisionBVH* BVH = nullptr;
};
