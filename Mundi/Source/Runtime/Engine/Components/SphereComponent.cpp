#include "pch.h"
#include "BoxComponent.h"
#include "SphereComponent.h"
#include "CapsuleComponent.h"
#include "Collision.h"

IMPLEMENT_CLASS(USphereComponent)

BEGIN_PROPERTIES(USphereComponent)
    MARK_AS_COMPONENT("스피어 컴포넌트", "구 형태의 충돌 컴포넌트입니다.")
    ADD_PROPERTY(float, Radius, "Shape", true, "구의 반지름")
    ADD_PROPERTY(FLinearColor, ShapeColor, "Shape", true, "디버그 드로우 시 사용할 색상")
    ADD_PROPERTY(bool, bDrawOnlyIfSelected, "Shape", true, "선택되었을 때만 디버그 드로우")
END_PROPERTIES()

USphereComponent::USphereComponent() : Radius(5.0f)
{
    ShapeColor = FLinearColor(1.0f, 0.34f, 0.28f);
    bDrawOnlyIfSelected = false;
    UpdateBoundingSphere();
}

// Set radius and update bounding sphere
void USphereComponent::SetRadius(const float InRadius)
{
    Radius = InRadius;
    UpdateBoundingSphere();
}

FAABB USphereComponent::GetWorldAABB()
{
    const FVector Center = CachedBound.GetCenter();
    const float R = CachedBound.GetRadius();
    FVector Min = Center - FVector(R, R, R);
    FVector Max = Center + FVector(R, R, R);
    return FAABB(Min, Max);
}

bool USphereComponent::Intersects(const UShapeComponent* Other) const
{
    switch (Other->GetShapeType())
    {
    case EShapeType::Box:
        const UBoxComponent* OtherBox = Cast<UBoxComponent>(Other);
        return Collision::Intersects(OtherBox->GetOBB(), CachedBound);
    case EShapeType::Sphere:
        const USphereComponent* OtherSphere = Cast<USphereComponent>(Other);
        return CachedBound.Intersects(OtherSphere->GetBoundingSphere());
    case EShapeType::Capsule:
        const UCapsuleComponent* OtherCapsule = Cast<UCapsuleComponent>(Other);
        return Collision::Intersects(CachedBound, OtherCapsule->GetBoundingCapsule());
    default:
        UE_LOG("USphereComponent::Intersects: Unsupported shape type for collision detection.");
        return false;
    }
}

void USphereComponent::OnTransformUpdated()
{
    Super::OnTransformUpdated();
    UpdateBoundingSphere();
}

void USphereComponent::UpdateBoundingSphere()
{
    // 월드 위치와 반지름으로 구 볼륨 갱신
    const FVector WorldPos = GetWorldLocation();
    CachedBound = FBoundingSphere(WorldPos, Radius);
}

void USphereComponent::DuplicateSubObjects()
{
    // Radius와 cached sphere는 POD 타입이므로 자동 복사됨
    Super::DuplicateSubObjects();
    UpdateBoundingSphere();
}

void USphereComponent::OnSerialized()
{
    Super::OnSerialized();
    // 직렬화 후 bounding sphere 재계산 (bounding sphere 자체는 직렬화 대상이 아니기 때문)
    UpdateBoundingSphere();
}