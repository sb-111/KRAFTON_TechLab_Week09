#pragma once
#include "ShapeComponent.h"
#include "BoundingSphere.h"

class USphereComponent : public UShapeComponent
{
public:
	DECLARE_CLASS(USphereComponent, UShapeComponent)
	GENERATED_REFLECTION_BODY()

	USphereComponent();
	~USphereComponent() override = default;

	// ───── Shape Component 속성 ──────────────────────────
	EShapeType GetShapeType() const override { return EShapeType::Sphere; }
	FAABB GetWorldAABB() override;
	bool Intersects(const UShapeComponent* Other) const override;

	// ───── Sphere Component 속성 ─────────────────────────
	void SetRadius(const float InRadius);
	FVector GetRadius() const { return Radius; }
	const FBoundingSphere& GetBoundingSphere() const { return CachedBound; }

	// ───── Transform 업데이트 ────────────────────────────
	void OnTransformUpdated() override;

	// ───── 복사 관련 ─────────────────────────────────────
	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(USphereComponent)

	// ───── 직렬화 ────────────────────────────────────────
	void OnSerialized() override;

private:
	void UpdateBoundingSphere();

private:
	float Radius = 5.0f;
	FBoundingSphere CachedBound; // World sphere volume (충돌 검사용. transform과 Radius에 의해 갱신)
};