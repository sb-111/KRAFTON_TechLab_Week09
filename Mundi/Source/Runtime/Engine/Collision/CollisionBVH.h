#pragma once
#include "AABB.h"

class UShapeComponent;

/**
 * @brief UShapeComponent 전용 경량 BVH (Broad Phase)
 * 충돌 가능한 ShapeComponent를 빠르게 쿼리하기 위한 구조체.
 */
class FCollisionBVH
{
public:
	FCollisionBVH(int32 InMaxDepth = 12, int32 InMaxLeafSize = 4);
	~FCollisionBVH() = default;

	void Clear();

	void BulkUpdate(const TArray<UShapeComponent*>& InShapes);
	void Update(UShapeComponent* InShape);
	void Remove(UShapeComponent* InShape);

	void FlushRebuild();

	/**@brief AABB를 이용한 간이 충돌검사로 충돌한 shape들을 반환합니다. 다소 부정확할 수 있습니다.*/
	TArray<UShapeComponent*> Query(const FAABB& InBounds) const;
	/**@brief UShapeComponent끼리 정확한 충돌 검사를 진행하여 충돌한 shape들을 반환합니다.*/
	TArray<UShapeComponent*> Query(const UShapeComponent* InShape) const;

	TArray<UShapeComponent*> GetShapeArray() const { return ShapeArray; }
	int32 TotalShapeCount() const { return static_cast<int32>(ShapeArray.Num()); }
	int32 TotalNodeCount() const { return static_cast<int32>(Nodes.Num()); }

private:
	struct FNode
	{
		FAABB Bounds;
		int32 Left = -1;
		int32 Right = -1;
		int32 First = 0;
		int32 Count = 0;
		bool IsLeaf() const { return Count > 0; }
	};

	void Build();
	int32 BuildRange(int32 StartIndex, int32 EndIndex, int32 Depth);
	FAABB GetShapeBound(UShapeComponent* Shape) const;
	void EnsureRebuilt() const;

private:
	TMap<UShapeComponent*, FAABB> CachedBounds;
	TArray<UShapeComponent*> ShapeArray;
	TArray<FNode> Nodes;

	int32 MaxDepth = 12;
	int32 MaxLeafSize = 4;
	mutable bool bPendingRebuild = false;
};
