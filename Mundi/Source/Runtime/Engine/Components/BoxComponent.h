#pragma once
#include "ShapeComponent.h"
#include "OBB.h"

class UBoxComponent : public UShapeComponent
{
public:
	DECLARE_CLASS(UBoxComponent, UShapeComponent)
	GENERATED_REFLECTION_BODY()

	UBoxComponent();
	~UBoxComponent() override = default;

	// ───── Shape Component 속성 ──────────────────────────
	EShapeType GetShapeType() const override { return EShapeType::Box; }
	FAABB GetWorldAABB() override;
	bool Intersects(const UShapeComponent* Other) const override;

	// ───── Box Component 속성 ────────────────────────────
	void SetExtent(const FVector& InExtent);
	FVector GetExtent() const { return BoxExtent; }
	const FOBB& GetOBB() const { return CachedBound; }

	// ───── Update ───────────────────────────────────────
	void OnTransformUpdated() override;
	void UpdateBound() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// ───── 복사 관련 ─────────────────────────────────────
	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(UBoxComponent)

	// ───── 직렬화 ────────────────────────────────────────
	void OnSerialized() override;

private:
	FVector BoxExtent = FVector(5.0f, 5.0f, 5.0f); // Half Extent
	FOBB CachedBound; // World OBB (충돌 검사용. Transform과 Extent에 의해 갱신)
};
