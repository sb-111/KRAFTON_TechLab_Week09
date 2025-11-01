#pragma once
#include "ShapeComponent.h"
#include "BoundingCapsule.h"

class UCapsuleComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(UCapsuleComponent, UShapeComponent)
    GENERATED_REFLECTION_BODY()
	UCapsuleComponent();
	~UCapsuleComponent() override = default;

	// ───── Shape Component 속성 ──────────────────────────
	EShapeType GetShapeType() const override { return EShapeType::Capsule; }
	FAABB GetWorldAABB() override;
	bool Intersects(const UShapeComponent* Other) const override;

	// ───── Capsule Component 속성 ────────────────────────
	void SetCapsuleHalfHeight(const float InHalfHeight);
	void SetCapsuleRadius(const float InRadius);
	const FBoundingCapsule& GetBoundingCapsule() const { return CachedBound; }

	// ───── Transform 업데이트 ────────────────────────────
	void OnTransformUpdated() override;

	// ───── 복사 관련 ─────────────────────────────────────
	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(UCapsuleComponent)

	// ───── 직렬화 ────────────────────────────────────────
	void OnSerialized() override;

private:
	void UpdateBoundingCapsule();

    float CapsuleHalfHeight = 1.0f;
    float CapsuleRadius = 5.0f;
	FBoundingCapsule CachedBound; // World capsule volume (충돌 검사용)
};