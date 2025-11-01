#pragma once
#include "Vector.h"

struct FBoundingCapsule
{
	FVector Center;        // 중심점
	FVector Axis;          // 축 방향 (정규화된 벡터)
	float HalfHeight;      // 반쪽 높이 (축의 절반 길이)
	float Radius;          // 반지름

	FBoundingCapsule();
	FBoundingCapsule(const FVector& InCenter, const FVector& InAxis, float InHalfHeight, float InRadius);
	virtual ~FBoundingCapsule() = default;

	// Public 멤버가 있지만 bounding volume 구조체간 일관성을 위해 구현
	FVector GetCenter() const { return Center; }
	FVector GetAxis() const { return Axis; }
	float GetHalfHeight() const { return HalfHeight; }
	float GetRadius() const { return Radius; }

	bool Contains(const FVector& Point) const;
	bool Intersects(const FBoundingCapsule& Other) const;
	bool IntersectsRay(const FRay& Ray, float& TEnter, float& TExit) const;
};