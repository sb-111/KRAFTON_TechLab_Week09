#include "pch.h"
#include "WorldPhysics.h"
#include "CollisionBVH.h"
#include "ShapeComponent.h"
#include "Renderer.h"
#include "SelectionManager.h"
#include "BoxComponent.h"
#include "SphereComponent.h"
#include "CapsuleComponent.h"
#include "OBB.h"
#include "BoundingSphere.h"
#include "BoundingCapsule.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
	static uint64 MakeCollisionPairKey(const UShapeComponent* A, const UShapeComponent* B)
	{
		if (!A || !B)
		{
			return 0;
		}

		// 동일한 두 ShapeComponent 조합을 유일한 키로 만들기 위해 포인터 순서를 정규화
		uintptr_t AddressA = reinterpret_cast<uintptr_t>(A);
		uintptr_t AddressB = reinterpret_cast<uintptr_t>(B);
		if (AddressA > AddressB)
		{
			std::swap(AddressA, AddressB);
		}

		const uint64 MixedLow = static_cast<uint64>(AddressA);
		const uint64 MixedHigh = static_cast<uint64>(AddressB);
		return MixedLow ^ (MixedHigh + 0x9e3779b97f4a7c15ULL + (MixedLow << 6) + (MixedLow >> 2));
	}
}

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

	TSet<uint64> ProcessedPairs;

	if (const TSet<UShapeComponent*>* CurrentCollisions = CollisionMap.Find(InShape))
	{
		for (UShapeComponent* Other : *CurrentCollisions)
		{
			if (!Other)
			{
				continue;
			}

			const uint64 Key = MakeCollisionPairKey(InShape, Other);
			if (Key != 0 && !ProcessedPairs.Contains(Key))
			{
				ProcessedPairs.Add(Key);
				// 등록 해제 직전, 여전히 겹치고 있던 상대에게 EndOverlap 즉시 전달
				EndOverlapEvent.Broadcast(InShape, Other);
			}
		}
	}

	for (auto& Pair : CollisionMap)
	{
		if (Pair.first == InShape)
		{
			continue;
		}

		if (Pair.second.Remove(InShape))
		{
			UShapeComponent* OtherOwner = const_cast<UShapeComponent*>(Pair.first);
			const uint64 Key = MakeCollisionPairKey(OtherOwner, InShape);
			if (Key != 0 && !ProcessedPairs.Contains(Key))
			{
				ProcessedPairs.Add(Key);
				// 다른 컴포넌트의 캐시에도 InShape가 남아 있을 수 있으니 종료 이벤트를 한 번 더 보장
				EndOverlapEvent.Broadcast(OtherOwner, InShape);
			}
		}
	}

	// Collision Map에서 제거
	CollisionMap.Remove(InShape);

	PreviousCollisionMap.Remove(InShape);
	for (auto& Pair : PreviousCollisionMap)
	{
		Pair.second.Remove(InShape);
	}

	// Update 대기중이었을 수 있으므로 DirtySet에서도 제거
	CollisionDirtySet.erase(InShape);
}

void UWorldPhysics::MarkCollisionDirty(UShapeComponent* PhysicsObject)
{
	if (!PhysicsObject || PhysicsObject->GetShapeType() == EShapeType::None)
	{
		return;
	}

	// 캐시된 CollisionMap이 있다면 즉시 무효화
	if (!CollisionMap.IsEmpty())
	{
		CollisionMap.Remove(PhysicsObject);
		for (auto& Pair : CollisionMap)
		{
			Pair.second.Remove(PhysicsObject);
		}
	}

	if (CollisionDirtySet.insert(PhysicsObject).second)
	{
		CollisionDirtyQueue.push(PhysicsObject);
	}
}

void UWorldPhysics::Update(float DeltaTime)
{
	(void)DeltaTime;

	// 1. DirtyQueue 처리
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
		// 2. BVH 갱신
		BVH->FlushRebuild();

		// 3. CollisionMap 갱신
		CollisionMap.clear();
		TArray<UShapeComponent*> Shapes = BVH->GetShapeArray();
		for (UShapeComponent* Shape : Shapes)
		{
			if (!Shape)
			{
				continue;
			}

			TArray<UShapeComponent*> Collided = BVH->Query(Shape);
			TSet<UShapeComponent*> CollidedSet(Collided.begin(), Collided.end());
			CollisionMap.Add(Shape, CollidedSet);
		}
	}
	else
	{
		CollisionMap.clear();
	}
	
	// 갱신된 충돌 집합을 기반으로 Begin/End 이벤트 발송
	BroadcastCollisionEvents();
	// 이후 프레임 비교를 위해 현재 프레임 결과를 스냅샷으로 보관
	PreviousCollisionMap = CollisionMap;
}

/**
 * @brief PhysicsObject와 충돌한 모든 SHapeComponent를 반환합니다.
 * @note CollisionMap에 캐싱된 결과가 이미 존재할 경우 우선적으로 사용합니다.
 */
TArray<UShapeComponent*> UWorldPhysics::CollisionQuery(const UShapeComponent* PhysicsObject) const
{
	TArray<UShapeComponent*> Collisions;

	if (!PhysicsObject || PhysicsObject->GetShapeType() == EShapeType::None)
	{
		return Collisions;
	}

	if (const TSet<UShapeComponent*>* CachedSet = CollisionMap.Find(PhysicsObject))
	{
		Collisions = CachedSet->Array();
		return Collisions;
	}

	if (BVH)
	{
		Collisions = BVH->Query(PhysicsObject);
	}
	return Collisions;
}

int32 UWorldPhysics::GetCollisionShapeCount() const
{
	return BVH ? BVH->TotalShapeCount() : 0;
}

int32 UWorldPhysics::GetCollisionNodeCount() const
{
	return BVH ? BVH->TotalNodeCount() : 0;
}

// DebugDrawCollision을 위한 헬퍼 함수들
namespace
{
	static FVector4 ToLineColor(const FLinearColor& Color)
	{
		return FVector4(Color.R, Color.G, Color.B, Color.A);
	}

	static void DrawOBB(URenderer* Renderer, const FOBB& OBB, const FVector4& Color)
	{
		if (!Renderer) return;

		const TArray<FVector> Corners = OBB.GetCorners();
		if (Corners.Num() != 8)
		{
			return;
		}

		static const int EdgeIndices[12][2] =
		{
			{0,1}, {0,2}, {0,4}, {1,3}, {1,5}, {2,3},
			{2,6}, {3,7}, {4,5}, {4,6}, {5,7}, {6,7}
		};

		for (const auto& Edge : EdgeIndices)
		{
			const FVector& A = Corners[Edge[0]];
			const FVector& B = Corners[Edge[1]];
			Renderer->AddLine(A, B, Color);
		}
	}

	static void DrawCircle(URenderer* Renderer, const FVector& Center, const FVector& AxisX, const FVector& AxisY, float Radius, int32 Segments, const FVector4& Color)
	{
		if (!Renderer || Radius <= 0.0f || Segments < 3)
		{
			return;
		}

		FVector PrevPoint = Center + (AxisX * Radius);
		for (int32 Segment = 1; Segment <= Segments; ++Segment)
		{
			const float Angle = (static_cast<float>(Segment) / static_cast<float>(Segments)) * TWO_PI;
			const float CosAngle = std::cos(Angle);
			const float SinAngle = std::sin(Angle);
			const FVector NextPoint = Center + ((AxisX * CosAngle + AxisY * SinAngle) * Radius);
			Renderer->AddLine(PrevPoint, NextPoint, Color);
			PrevPoint = NextPoint;
		}
	}

	static void DrawSphere(URenderer* Renderer, const FBoundingSphere& Sphere, const FVector4& Color)
	{
		const FVector Center = Sphere.GetCenter();
		const float Radius = Sphere.GetRadius();

		const FVector AxisX = FVector(1.0f, 0.0f, 0.0f);
		const FVector AxisY = FVector(0.0f, 1.0f, 0.0f);
		const FVector AxisZ = FVector(0.0f, 0.0f, 1.0f);

		const int32 Segments = 32;
		DrawCircle(Renderer, Center, AxisX, AxisY, Radius, Segments, Color); // XY plane
		DrawCircle(Renderer, Center, AxisX, AxisZ, Radius, Segments, Color); // XZ plane
		DrawCircle(Renderer, Center, AxisY, AxisZ, Radius, Segments, Color); // YZ plane
	}

	static void DrawCapsule(URenderer* Renderer, const FBoundingCapsule& Capsule, const FVector4& Color)
	{
		if (!Renderer) return;

		const FVector Center = Capsule.GetCenter();
		FVector Axis = Capsule.GetAxis();
		if (Axis.IsZero())
		{
			Axis = FVector(0.0f, 0.0f, 1.0f);
		}
		const FVector AxisDir = Axis.GetSafeNormal();
		const float HalfHeight = Capsule.GetHalfHeight();
		const float Radius = Capsule.GetRadius();

		const FVector TopCenter = Center + AxisDir * HalfHeight;
		const FVector BottomCenter = Center - AxisDir * HalfHeight;

		// Create orthonormal basis
		FVector Right = FVector::Cross(AxisDir, FVector(0.0f, 0.0f, 1.0f));
		if (Right.SizeSquared() < KINDA_SMALL_NUMBER)
		{
			Right = FVector::Cross(AxisDir, FVector(1.0f, 0.0f, 0.0f));
		}
		Right = Right.GetSafeNormal();
		FVector Forward = FVector::Cross(Right, AxisDir).GetSafeNormal();

		const int32 CircleSegments = 24;
		DrawCircle(Renderer, TopCenter, Right, Forward, Radius, CircleSegments, Color);
		DrawCircle(Renderer, BottomCenter, Right, Forward, Radius, CircleSegments, Color);

		Renderer->AddLine(TopCenter + Right * Radius, BottomCenter + Right * Radius, Color);
		Renderer->AddLine(TopCenter - Right * Radius, BottomCenter - Right * Radius, Color);
		Renderer->AddLine(TopCenter + Forward * Radius, BottomCenter + Forward * Radius, Color);
		Renderer->AddLine(TopCenter - Forward * Radius, BottomCenter - Forward * Radius, Color);

		const int32 HemisphereSegments = CircleSegments;
		auto DrawHemisphereArc = [Renderer, Radius, Color, HemisphereSegments](const FVector& CenterPoint, const FVector& Axis, const FVector& BasisInput)
		{
			FVector Basis = BasisInput.GetSafeNormal();
			if (Basis.IsZero())
			{
				return;
			}

			FVector PrevPoint = CenterPoint + Basis * Radius;
			for (int32 Step = 1; Step <= HemisphereSegments; ++Step)
			{
				const float T = (static_cast<float>(Step) / static_cast<float>(HemisphereSegments)) * PI;
				const float CosT = std::cos(T);
				const float SinT = std::sin(T);
				const FVector NextPoint = CenterPoint + Basis * (Radius * CosT) + Axis * (Radius * SinT);
				Renderer->AddLine(PrevPoint, NextPoint, Color);
				PrevPoint = NextPoint;
			}
		};

		DrawHemisphereArc(TopCenter, AxisDir,  Right);
		DrawHemisphereArc(TopCenter, AxisDir,  Forward);
		DrawHemisphereArc(BottomCenter, -AxisDir, Right);
		DrawHemisphereArc(BottomCenter, -AxisDir, Forward);
	}

	static bool IsShapeSelected(const UShapeComponent* Shape, USelectionManager* SelectionManager)
	{
		if (!SelectionManager || !Shape)
		{
			return false;
		}

		if (SelectionManager->GetSelectedActorComponent() == Shape)
		{
			return true;
		}

		if (AActor* Owner = Shape->GetOwner())
		{
			return SelectionManager->IsActorSelected(Owner);
		}

		return false;
	}
}

void UWorldPhysics::DebugDrawCollision(URenderer* Renderer, USelectionManager* SelectionManager) const
{
	// Renderer, BVH가 없거나 ShapeArray가 비었으면 즉시 종료
	if (!Renderer || !BVH)
	{
		return;
	}

	const TArray<UShapeComponent*> Shapes = BVH->GetShapeArray();
	if (Shapes.IsEmpty())
	{
		return;
	}

	// Shape Component 순회하며 Bounding Volume 가져와 그리기
	for (UShapeComponent* Shape : Shapes)
	{
		if (!Shape || Shape->GetShapeType() == EShapeType::None)
		{
			continue;
		}

		if (Shape->IsDrawOnlyWhenSelected() && !IsShapeSelected(Shape, SelectionManager))
		{
			continue;
		}

		const FVector4 Color = ToLineColor(Shape->GetShapeColor());

		switch (Shape->GetShapeType())
		{
		case EShapeType::Box:
			if (const UBoxComponent* Box = dynamic_cast<UBoxComponent*>(Shape))
			{
				DrawOBB(Renderer, Box->GetOBB(), Color);
			}
			break;
		case EShapeType::Sphere:
			if (const USphereComponent* Sphere = dynamic_cast<USphereComponent*>(Shape))
			{
				DrawSphere(Renderer, Sphere->GetBoundingSphere(), Color);
			}
			break;
		case EShapeType::Capsule:
			if (const UCapsuleComponent* Capsule = dynamic_cast<UCapsuleComponent*>(Shape))
			{
				DrawCapsule(Renderer, Capsule->GetBoundingCapsule(), Color);
			}
			break;
		default:
			break;
		}
	}
}

void UWorldPhysics::BroadcastCollisionEvents()
{
	TSet<uint64> ProcessedPairs;

	for (const auto& Pair : CollisionMap)
	{
		const UShapeComponent* OwnerConst = Pair.first;
		UShapeComponent* Owner = const_cast<UShapeComponent*>(OwnerConst);
		const TSet<UShapeComponent*>& Current = Pair.second;
		const TSet<UShapeComponent*>* Previous = PreviousCollisionMap.Find(OwnerConst);

		for (UShapeComponent* Other : Current)
		{
			if (!Previous || !Previous->Contains(Other))
			{
				if (!Other)
				{
					continue;
				}

				const uint64 Key = MakeCollisionPairKey(OwnerConst, Other);
				if (Key != 0 && !ProcessedPairs.Contains(Key))
				{
					ProcessedPairs.Add(Key);
					// 처음으로 겹치기 시작한 경우에만 BeginOverlap을 브로드캐스트
					BeginOverlapEvent.Broadcast(Owner, Other);
				}
			}
		}

		if (Previous)
		{
			for (UShapeComponent* PrevOther : *Previous)
			{
				if (!Current.Contains(PrevOther))
				{
					if (!PrevOther)
					{
						continue;
					}

					const uint64 Key = MakeCollisionPairKey(OwnerConst, PrevOther);
					if (Key != 0 && !ProcessedPairs.Contains(Key))
					{
						ProcessedPairs.Add(Key);
						// 앞선 프레임까지 겹쳤다가 이번 프레임에 분리된 경우 EndOverlap
						EndOverlapEvent.Broadcast(Owner, PrevOther);
					}
				}
			}
		}
	}
}
