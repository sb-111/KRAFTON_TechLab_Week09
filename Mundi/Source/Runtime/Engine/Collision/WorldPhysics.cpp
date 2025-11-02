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
#include <cmath>

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
	if (!Renderer || !BVH)
	{
		return;
	}

	const TArray<UShapeComponent*> Shapes = BVH->GetShapeArray();
	if (Shapes.IsEmpty())
	{
		return;
	}

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
