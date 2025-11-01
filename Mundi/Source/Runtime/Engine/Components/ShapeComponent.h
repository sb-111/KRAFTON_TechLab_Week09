#pragma once
#include "PrimitiveComponent.h"
#include "Color.h"
#include "AABB.h"

enum class EShapeType: uint8
{
    None,
    Box,
    Sphere,
    Capsule
};

class UShapeComponent : public UPrimitiveComponent
{
public:
	DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)
    
	UShapeComponent() = default;
	~UShapeComponent() override = default;

    virtual FAABB GetWorldAABB() { return FAABB(); } // World Partition System에서 사용하기 위한 AABB
	virtual EShapeType GetShapeType() const { return EShapeType::None; }

    // ───── 복사 관련 ────────────────────────────
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UShapeComponent)

    // ───── 직렬화 ────────────────────────────
    void OnSerialized() override;

    // 충돌 처리
    virtual bool Intersects(const UShapeComponent* Other) const { return false; }
	static bool Intersects(const UShapeComponent* A, const UShapeComponent* B);

public:
    // ShapeComponent의 기본 속성들은 public
    FLinearColor ShapeColor = FLinearColor(1.0f, 0.34f, 0.28f);
    bool bDrawOnlyIfSelected = false;
};