#include "pch.h"
#include "CollisionBVH.h"
#include "ShapeComponent.h"

#include <algorithm>
#include <limits>

FCollisionBVH::FCollisionBVH(int32 InMaxDepth, int32 InMaxLeafSize)
	: MaxDepth(InMaxDepth)
	, MaxLeafSize(InMaxLeafSize)
{
}

void FCollisionBVH::Clear()
{
	CachedBounds = TMap<UShapeComponent*, FAABB>();
	ShapeArray = TArray<UShapeComponent*>();
	Nodes = TArray<FNode>();
	bPendingRebuild = false;
}

void FCollisionBVH::BulkUpdate(const TArray<UShapeComponent*>& InShapes)
{
	CachedBounds = TMap<UShapeComponent*, FAABB>();

	for (UShapeComponent* Shape : InShapes)
	{
		if (!Shape)
		{
			continue;
		}
		CachedBounds.Add(Shape, Shape->GetWorldAABB());
	}

	Build();
	bPendingRebuild = false;
}

void FCollisionBVH::Update(UShapeComponent* InShape)
{
	if (!InShape)
	{
		return;
	}

	CachedBounds.Add(InShape, InShape->GetWorldAABB());
	bPendingRebuild = true;
}

void FCollisionBVH::Remove(UShapeComponent* InShape)
{
	if (!InShape)
	{
		return;
	}

	if (CachedBounds.Find(InShape))
	{
		CachedBounds.Remove(InShape);
		bPendingRebuild = true;
	}
}

void FCollisionBVH::FlushRebuild()
{
	EnsureRebuilt();
}

TArray<UShapeComponent*> FCollisionBVH::Query(const FAABB& InBounds) const
{
	EnsureRebuilt();

	TArray<UShapeComponent*> Result;
	if (Nodes.empty())
	{
		return Result;
	}

	TArray<int32> Stack;
	Stack.push_back(0);

	while (!Stack.empty())
	{
		const int32 Index = Stack.back();
		Stack.pop_back();

		const FNode& Node = Nodes[Index];
		if (!Node.Bounds.Intersects(InBounds))
		{
			continue;
		}

		if (Node.IsLeaf())
		{
			for (int32 Offset = 0; Offset < Node.Count; ++Offset)
			{
				UShapeComponent* Shape = ShapeArray[Node.First + Offset];
				if (!Shape)
				{
					continue;
				}

				const FAABB* Cached = CachedBounds.Find(Shape);
				const FAABB Bound = Cached ? *Cached : Shape->GetWorldAABB();
				if (Bound.Intersects(InBounds))
				{
					Result.push_back(Shape);
				}
			}
		}
		else
		{
			if (Node.Left >= 0)
			{
				Stack.push_back(Node.Left);
			}
			if (Node.Right >= 0)
			{
				Stack.push_back(Node.Right);
			}
		}
	}

	return Result;
}

TArray<UShapeComponent*> FCollisionBVH::Query(const UShapeComponent* InShape) const
{
	if (!InShape)
	{
		return TArray<UShapeComponent*>();
	}

	UShapeComponent* NonConstShape = const_cast<UShapeComponent*>(InShape);

	const FAABB* Cached = CachedBounds.Find(NonConstShape);
	FAABB QueryBound = Cached ? *Cached : NonConstShape->GetWorldAABB();

	TArray<UShapeComponent*> Candidates = Query(QueryBound);
	if (Candidates.empty())
	{
		return TArray<UShapeComponent*>();
	}
	// 자기 자신 제외
	Candidates.erase(std::remove(Candidates.begin(), Candidates.end(), NonConstShape), Candidates.end());
	
	TArray<UShapeComponent*> Result;
	for (UShapeComponent* Candidate : Candidates)
	{
		if (!Candidate)
			continue;
		if (Candidate->Intersects(NonConstShape))
			Result.push_back(Candidate);
	}
	
	return Result;
}

void FCollisionBVH::Build()
{
	ShapeArray = CachedBounds.GetKeys();
	const int32 Count = static_cast<int32>(ShapeArray.Num());

	Nodes.clear();

	if (Count == 0)
	{
		return;
	}

	Nodes.reserve(std::max(1, 2 * Count));
	Nodes.clear();
	BuildRange(0, Count, 0);
}

int32 FCollisionBVH::BuildRange(int32 StartIndex, int32 EndIndex, int32 Depth)
{
	const int32 Count = EndIndex - StartIndex;
	if (Count <= 0)
	{
		return -1;
	}

	const int32 NodeIndex = static_cast<int32>(Nodes.size());
	Nodes.push_back(FNode{});
	FNode& Node = Nodes[NodeIndex];

	bool bHasBounds = false;
	FAABB Accumulated;
	for (int32 It = StartIndex; It < EndIndex; ++It)
	{
		UShapeComponent* Shape = ShapeArray[It];
		if (!Shape)
		{
			continue;
		}
		const FAABB ShapeBound = GetShapeBound(Shape);
		if (!bHasBounds)
		{
			Accumulated = ShapeBound;
			bHasBounds = true;
		}
		else
		{
			Accumulated = FAABB::Union(Accumulated, ShapeBound);
		}
	}

	if (!bHasBounds)
	{
		Node.Bounds = FAABB();
		Node.First = StartIndex;
		Node.Count = Count;
		return NodeIndex;
	}

	Node.Bounds = Accumulated;

	if (Count <= MaxLeafSize || Depth >= MaxDepth)
	{
		Node.First = StartIndex;
		Node.Count = Count;
		return NodeIndex;
	}

	const FVector HalfExtent = Accumulated.GetHalfExtent();
	float Extents[3] = { HalfExtent.X, HalfExtent.Y, HalfExtent.Z };
	int32 SplitAxis = 0;
	if (Extents[1] > Extents[SplitAxis]) SplitAxis = 1;
	if (Extents[2] > Extents[SplitAxis]) SplitAxis = 2;

	if (Extents[SplitAxis] <= KINDA_SMALL_NUMBER)
	{
		Node.First = StartIndex;
		Node.Count = Count;
		return NodeIndex;
	}

	auto AxisCenter = [this, SplitAxis](UShapeComponent* Shape) -> float
	{
		const FAABB Bound = GetShapeBound(Shape);
		const FVector Center = Bound.GetCenter();
		switch (SplitAxis)
		{
		case 0: return Center.X;
		case 1: return Center.Y;
		default: return Center.Z;
		}
	};

	const int32 MidIndex = StartIndex + Count / 2;
	std::nth_element(ShapeArray.begin() + StartIndex, ShapeArray.begin() + MidIndex, ShapeArray.begin() + EndIndex,
		[&](UShapeComponent* A, UShapeComponent* B)
		{
			return AxisCenter(A) < AxisCenter(B);
		});

	const int32 LeftChild = BuildRange(StartIndex, MidIndex, Depth + 1);
	const int32 RightChild = BuildRange(MidIndex, EndIndex, Depth + 1);

	Node.Left = LeftChild;
	Node.Right = RightChild;
	Node.First = -1;
	Node.Count = 0;

	if (LeftChild >= 0 && RightChild >= 0)
	{
		Node.Bounds = FAABB::Union(Nodes[LeftChild].Bounds, Nodes[RightChild].Bounds);
	}

	return NodeIndex;
}

FAABB FCollisionBVH::GetShapeBound(UShapeComponent* Shape) const
{
	if (!Shape)
	{
		return FAABB();
	}

	const FAABB* Cached = CachedBounds.Find(Shape);
	if (Cached)
	{
		return *Cached;
	}

	return Shape->GetWorldAABB();
}

void FCollisionBVH::EnsureRebuilt() const
{
	if (!bPendingRebuild)
	{
		return;
	}

	// const_cast를 통해 내부 구조를 갱신 (외부 API는 const 보장)
	FCollisionBVH* Self = const_cast<FCollisionBVH*>(this);
	Self->Build();
	Self->bPendingRebuild = false;
}
