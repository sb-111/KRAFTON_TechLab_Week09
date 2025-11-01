#include "pch.h"
#include "BoundingCapsule.h"
#include "Picking.h"

FBoundingCapsule::FBoundingCapsule()
    : Center(FVector(0.f, 0.f, 0.f))
    , Axis(FVector(0.f, 0.f, 1.f))
    , HalfHeight(0.0f)
    , Radius(0.0f)
{
}

FBoundingCapsule::FBoundingCapsule(const FVector& InCenter, const FVector& InAxis, float InHalfHeight, float InRadius)
    : Center(InCenter)
    , Axis(InAxis.GetSafeNormal())
    , HalfHeight(InHalfHeight)
    , Radius(InRadius)
{
    if(Axis.SizeSquared() < KINDA_SMALL_NUMBER)
    {
        UE_LOG("[FBoundingCapsule] 잘못된 축 설정 감지. 입력값 대신 기본 축(0, 0, 1)을 사용합니다.");
        Axis = FVector(0.f, 0.f, 1.f);
	}
}

bool FBoundingCapsule::Contains(const FVector& Point) const
{
    // 캡슐의 중심 축(선분)에서 점까지의 최단 거리 계산
    const FVector CenterToPoint = Point - Center;
    
    // 축 방향으로의 투영 거리
    const float AxisProjection = FVector::Dot(CenterToPoint, Axis);
    
    // 투영 거리를 [-HalfHeight, HalfHeight] 범위로 클램핑
    const float ClampedProjection = FMath::Clamp(AxisProjection, -HalfHeight, HalfHeight);
    
    // 축 위의 가장 가까운 점
    const FVector ClosestPointOnAxis = Center + Axis * ClampedProjection;
    
    // 가장 가까운 점에서 점까지의 거리
    const float DistSquared = (Point - ClosestPointOnAxis).SizeSquared();
    
    return DistSquared <= (Radius * Radius);
}

bool FBoundingCapsule::Intersects(const FBoundingCapsule& Other) const
{
    // 두 캡슐의 중심 축(선분)간 최단 거리 계산
    
    const FVector& P1 = Center;
    const FVector& D1 = Axis;
    const float H1 = HalfHeight;
    
    const FVector& P2 = Other.Center;
    const FVector& D2 = Other.Axis;
    const float H2 = Other.HalfHeight;
    
    // 두 선분의 끝점
    const FVector A = P1 - D1 * H1;
    const FVector B = P1 + D1 * H1;
    const FVector C = P2 - D2 * H2;
    const FVector D = P2 + D2 * H2;
    
    // 선분 AB와 CD의 방향 벡터
    const FVector AB = B - A;
    const FVector CD = D - C;
    const FVector AC = C - A;
    
    const float ABdotAB = FVector::Dot(AB, AB);
    const float CDdotCD = FVector::Dot(CD, CD);
    const float ABdotCD = FVector::Dot(AB, CD);
    const float ACdotAB = FVector::Dot(AC, AB);
    const float ACdotCD = FVector::Dot(AC, CD);
    
    const float Denom = ABdotAB * CDdotCD - ABdotCD * ABdotCD;
    
    float s, t;
    
    // 벡터 AB와 벡터 CD의 방향이 일치 => 평행
    if (Denom < 1e-6f)
    {
        s = 0.0f;
        t = ACdotCD / CDdotCD;
    }
    else
    {
        s = (ABdotCD * ACdotCD - CDdotCD * ACdotAB) / Denom;
        t = (ABdotAB * ACdotCD - ABdotCD * ACdotAB) / Denom;
    }
    
    // s와 t를 [0, 1] 범위로 클램핑
    s = FMath::Clamp(s, 0.0f, 1.0f);
    t = FMath::Clamp(t, 0.0f, 1.0f);
    
    // 가장 가까운 두 점
    const FVector ClosestPoint1 = A + AB * s;
    const FVector ClosestPoint2 = C + CD * t;
    
    // 두 점 사이의 거리
    const float DistSquared = (ClosestPoint2 - ClosestPoint1).SizeSquared();
    const float RadiusSum = Radius + Other.Radius;
    
    return DistSquared <= (RadiusSum * RadiusSum);
}

bool FBoundingCapsule::IntersectsRay(const FRay& Ray, float& TEnter, float& TExit) const
{
    // 캡슐을 무한 원기둥과 두 개의 반구로 분해하여 교차 검사
    
    const FVector RayToCenter = Ray.Origin - Center;
    
    // 축 방향으로의 성분 제거 (원기둥의 2D 단면에서의 레이)
    const float RayAxisDot = FVector::Dot(Ray.Direction, Axis);
    const float OriginAxisDot = FVector::Dot(RayToCenter, Axis);
    
    const FVector RayDir2D = Ray.Direction - Axis * RayAxisDot;
    const FVector RayOrigin2D = RayToCenter - Axis * OriginAxisDot;
    
    // 원기둥 부분과의 교차 검사 (2D 원과 직선의 교차)
    const float A = FVector::Dot(RayDir2D, RayDir2D);
    const float B = 2.0f * FVector::Dot(RayOrigin2D, RayDir2D);
    const float C = FVector::Dot(RayOrigin2D, RayOrigin2D) - Radius * Radius;
    
    const float Discriminant = B * B - 4 * A * C;
    
    if (Discriminant < 0.0f)
    {
        return false; // 원기둥과 교차하지 않음
    }
    
    const float SqrtDisc = std::sqrt(Discriminant);
    float Tmin = (-B - SqrtDisc) / (2.0f * A);
    float Tmax = (-B + SqrtDisc) / (2.0f * A);
    
    if (Tmin > Tmax) std::swap(Tmin, Tmax);
    
    // 교차점이 캡슐의 높이 범위 내에 있는지 확인
    const float ZMin = OriginAxisDot + RayAxisDot * Tmin;
    const float ZMax = OriginAxisDot + RayAxisDot * Tmax;
    
    // 원기둥 부분의 유효한 교차점 찾기
    if (ZMin >= -HalfHeight && ZMin <= HalfHeight)
    {
        TEnter = Tmin;
    }
    else if (ZMax >= -HalfHeight && ZMax <= HalfHeight)
    {
        TEnter = Tmax;
    }
    else
    {
        // 원기둥 부분과 교차하지 않으므로 양쪽 반구와의 교차 검사
        TEnter = std::numeric_limits<float>::max();
    }
    
    // 두 반구와의 교차 검사
    const FVector TopCenter = Center + Axis * HalfHeight;
    const FVector BottomCenter = Center - Axis * HalfHeight;
    
    float SphereT1, SphereT2;
    bool HitTopSphere = false;
    bool HitBottomSphere = false;
    
    // 상단 반구
    {
        const FVector OC = Ray.Origin - TopCenter;
        const float A_sphere = FVector::Dot(Ray.Direction, Ray.Direction);
        const float B_sphere = 2.0f * FVector::Dot(OC, Ray.Direction);
        const float C_sphere = FVector::Dot(OC, OC) - Radius * Radius;
        
        const float Disc = B_sphere * B_sphere - 4 * A_sphere * C_sphere;
        if (Disc >= 0.0f)
        {
            const float SqrtD = std::sqrt(Disc);
            SphereT1 = (-B_sphere - SqrtD) / (2.0f * A_sphere);
            SphereT2 = (-B_sphere + SqrtD) / (2.0f * A_sphere);
            
            // 교차점이 반구의 위쪽에 있는지 확인
            const FVector Hit1 = Ray.Origin + Ray.Direction * SphereT1;
            const FVector Hit2 = Ray.Origin + Ray.Direction * SphereT2;
            
            if (FVector::Dot(Hit1 - TopCenter, Axis) >= 0.0f && SphereT1 >= 0.0f)
            {
                TEnter = FMath::Min(TEnter, SphereT1);
                HitTopSphere = true;
            }
            if (FVector::Dot(Hit2 - TopCenter, Axis) >= 0.0f && SphereT2 >= 0.0f)
            {
                TEnter = FMath::Min(TEnter, SphereT2);
                HitTopSphere = true;
            }
        }
    }
    
    // 하단 반구
    {
        const FVector OC = Ray.Origin - BottomCenter;
        const float A_sphere = FVector::Dot(Ray.Direction, Ray.Direction);
        const float B_sphere = 2.0f * FVector::Dot(OC, Ray.Direction);
        const float C_sphere = FVector::Dot(OC, OC) - Radius * Radius;
        
        const float Disc = B_sphere * B_sphere - 4 * A_sphere * C_sphere;
        if (Disc >= 0.0f)
        {
            const float SqrtD = std::sqrt(Disc);
            SphereT1 = (-B_sphere - SqrtD) / (2.0f * A_sphere);
            SphereT2 = (-B_sphere + SqrtD) / (2.0f * A_sphere);
            
            // 교차점이 반구의 아래쪽에 있는지 확인
            const FVector Hit1 = Ray.Origin + Ray.Direction * SphereT1;
            const FVector Hit2 = Ray.Origin + Ray.Direction * SphereT2;
            
            if (FVector::Dot(Hit1 - BottomCenter, Axis) <= 0.0f && SphereT1 >= 0.0f)
            {
                TEnter = FMath::Min(TEnter, SphereT1);
                HitBottomSphere = true;
            }
            if (FVector::Dot(Hit2 - BottomCenter, Axis) <= 0.0f && SphereT2 >= 0.0f)
            {
                TEnter = FMath::Min(TEnter, SphereT2);
                HitBottomSphere = true;
            }
        }
    }
    
    if (TEnter == std::numeric_limits<float>::max())
    {
        return false;
    }
    
    TExit = Tmax;
    return true;
}
