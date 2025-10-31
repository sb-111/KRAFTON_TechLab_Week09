#include "pch.h"
#include "BoxComponent.h"
#include "SphereComponent.h"
#include "Collision.h"

IMPLEMENT_CLASS(UBoxComponent)

BEGIN_PROPERTIES(UBoxComponent)
	MARK_AS_COMPONENT("박스 컴포넌트", "박스 형태의 충돌 컴포넌트입니다.")
	ADD_PROPERTY(FVector, BoxExtent, "Shape", true, "박스의 Half Extent (중심에서 각 축 방향으로의 거리)")
	ADD_PROPERTY(FLinearColor, ShapeColor, "Shape", true, "디버그 드로우 시 사용할 색상")
	ADD_PROPERTY(bool, bDrawOnlyIfSelected, "Shape", true, "선택되었을 때만 디버그 드로우")
END_PROPERTIES()

UBoxComponent::UBoxComponent()
{
	BoxExtent = FVector(5.0f, 5.0f, 5.0f);
	ShapeColor = FLinearColor(1.0f, 0.34f, 0.28f);
	bDrawOnlyIfSelected = false;
	UpdateOBB(); // OBB 초기화
}

// Set "HALF" extents and update OBB
void UBoxComponent::SetExtent(const FVector& InExtent)
{
	BoxExtent = InExtent;
	UpdateOBB();
}

// Create World AABB from 8 OBB corners
FAABB UBoxComponent::GetWorldAABB()
{
	const TArray<FVector> Corners = CachedBound.GetCorners();
	
	if (Corners.empty())
	{
		// OBB가 유효하지 않은 경우 중심점 반환
		const FVector WorldPos = GetWorldLocation();
		return FAABB(WorldPos, WorldPos);
	}

	FVector Min = Corners[0];
	FVector Max = Corners[0];

	for (const FVector& Corner : Corners)
	{
		Min.X = FMath::Min(Min.X, Corner.X);
		Min.Y = FMath::Min(Min.Y, Corner.Y);
		Min.Z = FMath::Min(Min.Z, Corner.Z);

		Max.X = FMath::Max(Max.X, Corner.X);
		Max.Y = FMath::Max(Max.Y, Corner.Y);
		Max.Z = FMath::Max(Max.Z, Corner.Z);
	}

	return FAABB(Min, Max);
}

bool UBoxComponent::Intersects(const UShapeComponent* Other) const
{
	EShapeType OtherType = Other->GetShapeType();

	if (OtherType == EShapeType::Box)
	{
		const UBoxComponent* OtherBox = Cast<UBoxComponent>(Other);
		return this->CachedBound.Intersects(OtherBox->CachedBound);
	}
	if (OtherType == EShapeType::Sphere)
	{
		// OBB vs Sphere 충돌 검사
		const USphereComponent* OtherSphere = Cast<USphereComponent>(Other);
		return Collision::Intersects(this->CachedBound, OtherSphere->GetBoundingSphere());
	}

	UE_LOG("UBoxComponent::Intersects: Unsupported shape type for collision detection.");
	return false;
}

void UBoxComponent::OnTransformUpdated()
{
	Super::OnTransformUpdated();
	UpdateOBB();
}

// Create Local AABB -> Transform to World OBB using Transform of the component
void UBoxComponent::UpdateOBB()
{
	const FVector LocalMin = -BoxExtent;
	const FVector LocalMax = BoxExtent;
	const FAABB LocalAABB(LocalMin, LocalMax);

	const FMatrix WorldMatrix = GetWorldMatrix();
	CachedBound = FOBB(LocalAABB, WorldMatrix);
}

void UBoxComponent::DuplicateSubObjects()
{
	// BoxExtent와 cached OBB는 POD 타입이므로 자동 복사됨
	Super::DuplicateSubObjects();
	// 혹시 모를 불일치 방지를 위해 복사 후 OBB 재계산
	UpdateOBB(); 
}

void UBoxComponent::OnSerialized()
{
	Super::OnSerialized();
	// 직렬화 후 OBB 재계산 (OBB 자체는 직렬화 대상이 아니기 때문)
	UpdateOBB();
}
