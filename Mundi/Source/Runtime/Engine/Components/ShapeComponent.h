#pragma once
#include "PrimitiveComponent.h"
#include "Color.h"
#include "AABB.h"
#include "MultiCastDelegate.h"

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

	void Destroy() override;
	void OnRegister(UWorld* InWorld) override;
	void OnUnregister() override;
	
	void SetShapeColor(FLinearColor InColor) { ShapeColor = InColor; }
	FLinearColor GetShapeColor() const { return ShapeColor; }
	void SetDrawOnlyWhenSelected(bool bInDrawOnlyWhenSelected) { bDrawOnlyIfSelected = bInDrawOnlyWhenSelected; }
	bool IsDrawOnlyWhenSelected() const { return bDrawOnlyIfSelected; }
	
    virtual FAABB GetWorldAABB() { return FAABB(); } // World Partition System에서 사용하기 위한 AABB
	virtual EShapeType GetShapeType() const { return EShapeType::None; }
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	virtual void UpdateBound() {};
	
    // ───── 복사 관련 ────────────────────────────
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UShapeComponent)

    // ───── 직렬화 ────────────────────────────
    void OnSerialized() override;

    // 충돌 처리
    virtual bool Intersects(const UShapeComponent* Other) const { return false; }
	static bool Intersects(const UShapeComponent* A, const UShapeComponent* B);

	virtual void OnCollisionBegin(UShapeComponent* Other) { UE_LOG("Collision Begin"); }
	virtual void OnCollisionEnd(UShapeComponent* Other) { UE_LOG("Collision End"); }

protected:
	void BindCollisionDelegates();
	void UnbindCollisionDelegates();

	void HandleBeginOverlap(UShapeComponent* A, UShapeComponent* B);
	void HandleEndOverlap(UShapeComponent* A, UShapeComponent* B);

    // 기본 디버그 표시 속성
    FLinearColor ShapeColor = FLinearColor(1.0f, 0.34f, 0.28f);
    bool bDrawOnlyIfSelected = false;

	FBindingHandle BeginOverlapHandle;
	FBindingHandle EndOverlapHandle;
};
