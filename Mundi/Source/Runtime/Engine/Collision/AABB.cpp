#include "pch.h"
#include "AABB.h"

FAABB::FAABB() : Min(FVector()), Max(FVector()) {}

FAABB::FAABB(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax) {}

// 중심점
FVector FAABB::GetCenter() const
{
	return (Min + Max) * 0.5f;
}

// 반쪽 크기 (Extent)
FVector FAABB::GetHalfExtent() const
{
	return (Max - Min) * 0.5f;
}

TArray<FVector> FAABB::GetPoints() const
{
	TArray<FVector> Points;
	Points.resize(8);

	Points[0] = FVector(Min.X, Min.Y, Min.Z);
	Points[1] = FVector(Min.X, Min.Y, Max.Z);
	Points[2] = FVector(Min.X, Max.Y, Min.Z);
	Points[3] = FVector(Min.X, Max.Y, Max.Z);

	Points[4] = FVector(Max.X, Min.Y, Min.Z);
	Points[5] = FVector(Max.X, Min.Y, Max.Z);
	Points[6] = FVector(Max.X, Max.Y, Min.Z);
	Points[7] = FVector(Max.X, Max.Y, Max.Z);
	
	return Points;
}

// 다른 박스를 완전히 포함하는지 확인
bool FAABB::Contains(const FAABB& Other) const
{
	return (Min.X <= Other.Min.X && Max.X >= Other.Max.X) &&
		(Min.Y <= Other.Min.Y && Max.Y >= Other.Max.Y) &&
		(Min.Z <= Other.Min.Z && Max.Z >= Other.Max.Z);
}

// 교차 여부만 확인
bool FAABB::Intersects(const FAABB& Other) const
{
	return (Min.X <= Other.Max.X && Max.X >= Other.Min.X) &&
		(Min.Y <= Other.Max.Y && Max.Y >= Other.Min.Y) &&
		(Min.Z <= Other.Max.Z && Max.Z >= Other.Min.Z);
}

// i번째 옥탄트 Bounds 반환
FAABB FAABB::CreateOctant(int i) const
{
	const FVector Center = GetCenter();

	FVector NewMin, NewMax;

	// X축 (i의 1비트)
	// 0 왼쪽 1 오른쪽 
	if (i & 1)
	{
		NewMin.X = Center.X;
		NewMax.X = Max.X;
	}
	else
	{
		NewMin.X = Min.X;
		NewMax.X = Center.X;
	}

	// Y축 (i의 2비트)
	// 0 앞 2 뒤 
	if (i & 2)
	{
		NewMin.Y = Center.Y;
		NewMax.Y = Max.Y;
	}
	else
	{
		NewMin.Y = Min.Y;
		NewMax.Y = Center.Y;
	}

	// Z축 (i의 4비트)
	// 0 아래 4 위 
	if (i & 4)
	{
		NewMin.Z = Center.Z;
		NewMax.Z = Max.Z;
	}
	else
	{
		NewMin.Z = Min.Z;
		NewMax.Z = Center.Z;
	}

	return FAABB(NewMin, NewMax);
}

bool FAABB::IntersectsRay(const FRay& InRay, float& OutEnterDistance, float& OutExitDistance) const
{
	// 레이가 박스를 통과할 수 있는 [Enter, Exit] 구간
	float TEnterMax = -FLT_MAX;
	float TExitMin = FLT_MAX;

	for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
	{
		const float RayOriginAxis = InRay.Origin[AxisIndex];
		const float RayDirectionAxis = InRay.Direction[AxisIndex];
		const float BoxMinAxis = Min[AxisIndex];
		const float BoxMaxAxis = Max[AxisIndex];

		// 레이가 축에 평행한데, 박스 범위를 벗어나면 교차 불가
		if (std::abs(RayDirectionAxis) < 1e-6f)
		{
			if (RayOriginAxis < BoxMinAxis || RayOriginAxis > BoxMaxAxis)
			{
				return false;
			}
		}
		else
		{
			const float InvDirection = 1.0f / RayDirectionAxis;

			// 평면과의 교차 거리 
			// 레이가 AABB의 min 평면과 max 평면을 만나는 t 값 (거리)
			float DistanceToMinPlane = (BoxMinAxis - RayOriginAxis) * InvDirection;
			float DistanceToMaxPlane = (BoxMaxAxis - RayOriginAxis) * InvDirection;

			if (DistanceToMinPlane > DistanceToMaxPlane)
			{
				std::swap(DistanceToMinPlane, DistanceToMaxPlane);
			}
			
			// 더 늦게 들어오는 값으로 갱신
			if (DistanceToMinPlane > TEnterMax)  TEnterMax = DistanceToMinPlane;

			
			// 더 빨리 나가는 값으로 갱신 
			if (DistanceToMaxPlane < TExitMin) TExitMin = DistanceToMaxPlane;

			// 가장 늦게 들어오는 시점이 빠르게 나가는 시점보다 늦다는 것은 교차하지 않음을 의미한다. 
			if (TEnterMax > TExitMin)
			{
				return false; // 레이가 박스를 관통하지 않음
			}
		}
	}
	if (TExitMin < 0.0f)
		return false;

	// 레이가 박스와 실제로 만나는 구간이다. 
	OutEnterDistance = (TEnterMax < 0.0f) ? 0.0f : TEnterMax;
	OutExitDistance = TExitMin;
	return true;
}

FAABB FAABB::Union(const FAABB& A, const FAABB& B)
{
	FAABB out;
	out.Min = FVector(
		std::min(A.Min.X, B.Min.X),
		std::min(A.Min.Y, B.Min.Y),
		std::min(A.Min.Z, B.Min.Z));
	out.Max = FVector(
		std::max(A.Max.X, B.Max.X),
		std::max(A.Max.Y, B.Max.Y),
		std::max(A.Max.Z, B.Max.Z));
	return out;
}

void FAABB::Union(const FAABB& Other)
{
	/*FAABB out;
	out.Min = FVector(
		std::min(Min.X, B.Min.X),
		std::min(A.Min.Y, B.Min.Y),
		std::min(A.Min.Z, B.Min.Z));
	out.Max = FVector(
		std::max(A.Max.X, B.Max.X),
		std::max(A.Max.Y, B.Max.Y),
		std::max(A.Max.Z, B.Max.Z));*/
	Min.X = std::min(Min.X, Other.Min.X);
	Min.Y = std::min(Min.Y, Other.Min.Y);
	Min.Z = std::min(Min.Z, Other.Min.Z);

	Max.X = std::max(Max.X, Other.Max.X);
	Max.Y = std::max(Max.Y, Other.Max.Y);
	Max.Z = std::max(Max.Z, Other.Max.Z);
}

FAABB FAABB::GetAABB(const TArray<FVector>& Points)
{
	if (!(Points.Num() > 0))
		return FAABB();
	FAABB Result( Points[0], Points[0]);

	for (int Index = 1; Index < Points.Num(); Index++)
	{
		Result.Union(FAABB(Points[Index], Points[Index]));
	}
	return Result;
}
FAABB FAABB::GetAABB(const FVector* Points)
{
	//Points가 8개라고 가정. 배열 처리를 어떻게 할 지 모르겠음
	if (!Points)
		return FAABB();
	FAABB Result(Points[0], Points[0]);

	for (int Index = 1; Index < 8; Index++)
	{
		Result.Union(FAABB(Points[Index], Points[Index]));
	}
	return Result;
}

// 어차피 View 좌표계로 변환하면서 임시 버퍼 생성해야해서 복사로 받음
FAABB FAABB::GetShadowAABBInView(TArray<FVector> PointsOnFrustum, TArray<FVector> PointsOnScene, const FMatrix& ViewMatrix)
{
	for (int Index = 0; Index < 8; Index++)
	{
		PointsOnFrustum[Index] = PointsOnFrustum[Index] * ViewMatrix;
		PointsOnScene[Index] = PointsOnScene[Index] * ViewMatrix;
	}


	FAABB SceneAABBLocal = FAABB::GetAABB(PointsOnScene);
	FAABB FrustumAABBLocal = FAABB::GetAABB(PointsOnFrustum);
	const FVector& SceneMin = SceneAABBLocal.Min;
	const FVector& SceneMax = SceneAABBLocal.Max;
	const FVector& FrustumMin = FrustumAABBLocal.Min;
	const FVector& FrustumMax = FrustumAABBLocal.Max;

	FAABB ShadowAABB;
	//FrustumMin.Z가 SceneMin.Z보다 더 큰 경우, FrustumMin.Z를 선택하면 카메라 뒤의 캐스터가 컬링됨.
	ShadowAABB.Min = FVector(std::max(SceneMin.X, FrustumMin.X), std::max(SceneMin.Y, FrustumMin.Y), SceneMin.Z);
	//ZFar의 경우 프러스텀과 씬 둘의 zFar중에서 더 작은 것을 선택해도 됨
	//FrustumMax > SceneMax -> 어차피 SceneMax이후에 캐스터가 없어서 컬링, FrustumMax < SceneMax -> 어차피 카메라가 컬링해서 캐스터의 역할을 못 함.
	ShadowAABB.Max = FVector(std::min(SceneMax.X, FrustumMax.X), std::min(SceneMax.Y, FrustumMax.Y), std::min(SceneMax.Z, FrustumMax.Z));

	return ShadowAABB;
}