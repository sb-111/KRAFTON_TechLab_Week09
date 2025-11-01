#include "pch.h"
#include "Collision.h"
#include "AABB.h"
#include "OBB.h"
#include "BoundingSphere.h"
#include "BoundingCapsule.h"
#include "Picking.h"

namespace Collision
{
    bool Intersects(const FAABB& Aabb, const FOBB& Obb)
    {
        const FOBB ConvertedOBB(Aabb, FMatrix::Identity());
        return ConvertedOBB.Intersects(Obb);
    }

    bool Intersects(const FAABB& Aabb, const FBoundingSphere& Sphere)
    {
		// Real Time Rendering 4th, 22.13.2 Sphere/Box Intersection
        float Dist2 = 0.0f;

        for (int32 i = 0; i < 3; ++i)
        {
            if (Sphere.Center[i] < Aabb.Min[i])
            {
                float Delta = Sphere.Center[i] - Aabb.Min[i];
                if (Delta < -Sphere.GetRadius())
					return false;
				Dist2 += Delta * Delta;
            }
            else if (Sphere.Center[i] > Aabb.Max[i])
            {
                float Delta = Sphere.Center[i] - Aabb.Max[i];
                if (Delta > Sphere.GetRadius())
                    return false;
                Dist2 += Delta * Delta;
            }
        }
        return Dist2 <= (Sphere.Radius * Sphere.Radius);
	}

    bool Intersects(const FAABB& Aabb, const FBoundingCapsule& Capsule)
    {
        // 캡슐 중심선 세그먼트
        const FVector P0 = Capsule.Center - Capsule.Axis * Capsule.HalfHeight;
        const FVector P1 = Capsule.Center + Capsule.Axis * Capsule.HalfHeight;
        const float R = Capsule.Radius;
        const float R2 = R * R;

        // ───────────────────────────────────────────────────────────────
        // 1) 빠른 거절: 팽창된 AABB와 캡슐 중심의 AABB가 겹치지 않으면 충돌 불가
        // ───────────────────────────────────────────────────────────────
        const FVector CapsuleMin = FVector(
            FMath::Min(P0.X, P1.X) - R,
            FMath::Min(P0.Y, P1.Y) - R,
            FMath::Min(P0.Z, P1.Z) - R
        );
        const FVector CapsuleMax = FVector(
            FMath::Max(P0.X, P1.X) + R,
            FMath::Max(P0.Y, P1.Y) + R,
            FMath::Max(P0.Z, P1.Z) + R
        );

        if (CapsuleMax.X < Aabb.Min.X || CapsuleMin.X > Aabb.Max.X ||
            CapsuleMax.Y < Aabb.Min.Y || CapsuleMin.Y > Aabb.Max.Y ||
            CapsuleMax.Z < Aabb.Min.Z || CapsuleMin.Z > Aabb.Max.Z)
        {
            return false;
        }

        // ───────────────────────────────────────────────────────────────
        // 2) 세그먼트에서 AABB까지의 최단 거리 계산
        // ───────────────────────────────────────────────────────────────
        auto Clamp = [](float x, float a, float b) -> float {
            return (x < a) ? a : (x > b ? b : x);
            };

        auto ClosestBoxPoint = [&](const FVector& p) -> FVector {
            return FVector(
                Clamp(p.X, Aabb.Min.X, Aabb.Max.X),
                Clamp(p.Y, Aabb.Min.Y, Aabb.Max.Y),
                Clamp(p.Z, Aabb.Min.Z, Aabb.Max.Z)
            );
            };

        // 세그먼트 상의 점을 매개변수 t ∈ [0,1]로 표현: Point(t) = P0 + t * (P1 - P0)
        const FVector SegmentDir = P1 - P0;
        const float SegmentLengthSq = FVector::Dot(SegmentDir, SegmentDir);

        float BestDist2 = FLT_MAX;

        // 세그먼트의 끝점 테스트
        {
            const FVector B0 = ClosestBoxPoint(P0);
            const float D0_2 = (P0 - B0).SizeSquared();
            BestDist2 = FMath::Min(BestDist2, D0_2);

            const FVector B1 = ClosestBoxPoint(P1);
            const float D1_2 = (P1 - B1).SizeSquared();
            BestDist2 = FMath::Min(BestDist2, D1_2);
        }

        // AABB의 8개 꼭짓점에서 세그먼트까지의 최단 거리
        const TArray<FVector> Corners = Aabb.GetPoints();

        for (const FVector& Corner : Corners)
        {
            if (SegmentLengthSq > 1e-6f)
            {
                const FVector ToCorner = Corner - P0;
                float t = FVector::Dot(ToCorner, SegmentDir) / SegmentLengthSq;
                t = Clamp(t, 0.0f, 1.0f);

                const FVector ClosestOnSegment = P0 + SegmentDir * t;
                const float Dist2 = (Corner - ClosestOnSegment).SizeSquared();
                BestDist2 = FMath::Min(BestDist2, Dist2);
            }
            else
            {
                // 세그먼트가 점으로 축퇴된 경우
                const float Dist2 = (Corner - P0).SizeSquared();
                BestDist2 = FMath::Min(BestDist2, Dist2);
            }
        }

        // AABB의 12개 엣지와 세그먼트 간의 최단 거리 (선분-선분)
        // 간단하게 하려면 각 엣지의 중점과 끝점을 샘플링
        const int EdgeIndices[12][2] = {
            {0,1}, {2,3}, {4,5}, {6,7},  // Z축 평행
            {0,2}, {1,3}, {4,6}, {5,7},  // Y축 평행
            {0,4}, {1,5}, {2,6}, {3,7}   // X축 평행
        };

        for (const auto& EdgeIdx : EdgeIndices)
        {
            const FVector EdgeStart = Corners[EdgeIdx[0]];
            const FVector EdgeEnd = Corners[EdgeIdx[1]];
            const FVector EdgeDir = EdgeEnd - EdgeStart;
            const float EdgeLengthSq = FVector::Dot(EdgeDir, EdgeDir);

            if (EdgeLengthSq < 1e-6f) continue;

            // 두 선분 간 최단 거리 (간단한 근사: 엣지의 여러 샘플 점 테스트)
            for (int sample = 0; sample <= 2; ++sample)
            {
                const float s = sample * 0.5f; // 0.0, 0.5, 1.0
                const FVector EdgePoint = EdgeStart + EdgeDir * s;

                const FVector ToEdgePoint = EdgePoint - P0;
                float t = FVector::Dot(ToEdgePoint, SegmentDir) / SegmentLengthSq;
                t = Clamp(t, 0.0f, 1.0f);

                const FVector ClosestOnSegment = P0 + SegmentDir * t;
                const float Dist2 = (EdgePoint - ClosestOnSegment).SizeSquared();
                BestDist2 = FMath::Min(BestDist2, Dist2);
            }
        }

        return BestDist2 <= R2;
    }

    bool Intersects(const FOBB& Obb, const FBoundingSphere& Sphere)
    {
        // Real Time Rendering 4th, 22.13.2 Sphere/Box Intersection
        float Dist2 = 0.0f;
        // OBB의 로컬 좌표계로 구 중심점 변환
        FVector LocalCenter = Obb.Center;
        for (int32 i = 0; i < 3; ++i)
        {
            FVector Axis = Obb.Axes[i];
            float Offset = FVector::Dot(Sphere.Center - Obb.Center, Axis);
            if (Offset > Obb.HalfExtent[i])
                Offset = Obb.HalfExtent[i];
            else if (Offset < -Obb.HalfExtent[i])
                Offset = -Obb.HalfExtent[i];
            LocalCenter += Axis * Offset;
        }
        for (int i = 0; i < 3; ++i)
        {
            if (LocalCenter[i] < -Obb.HalfExtent[i])
            {
                float Delta = LocalCenter[i] + Obb.HalfExtent[i];
                if (Delta < -Sphere.GetRadius())
                    return false;
                Dist2 += Delta * Delta;
            }
            else if (LocalCenter[i] > Obb.HalfExtent[i])
            {
                float Delta = LocalCenter[i] - Obb.HalfExtent[i];
                if (Delta > Sphere.GetRadius())
                    return false;
                Dist2 += Delta * Delta;
            }
        }
        return Dist2 <= (Sphere.Radius * Sphere.Radius);
	}

    bool Intersects(const FOBB& Obb, const FBoundingCapsule& Capsule)
    {
        // ───────────────────────────────────────────────────────────────
        // 1) 빠른 거절: 캡슐과 OBB의 대략적인 영역 체크
        //    캡슐의 AABB를 구하고 OBB의 AABB와 겹치는지 확인
        // ───────────────────────────────────────────────────────────────
        const FVector P0 = Capsule.Center - Capsule.Axis * Capsule.HalfHeight;
        const FVector P1 = Capsule.Center + Capsule.Axis * Capsule.HalfHeight;
        const float R = Capsule.Radius;

        const FVector CapsuleMin = FVector(
            FMath::Min(P0.X, P1.X) - R,
            FMath::Min(P0.Y, P1.Y) - R,
            FMath::Min(P0.Z, P1.Z) - R
        );
        const FVector CapsuleMax = FVector(
            FMath::Max(P0.X, P1.X) + R,
            FMath::Max(P0.Y, P1.Y) + R,
            FMath::Max(P0.Z, P1.Z) + R
        );

        // OBB의 8개 꼭짓점으로부터 AABB 계산
        const TArray<FVector> ObbCorners = Obb.GetCorners();
        FVector ObbMin = ObbCorners[0];
        FVector ObbMax = ObbCorners[0];
        for (const FVector& Corner : ObbCorners)
        {
            ObbMin = ObbMin.ComponentMin(Corner);
            ObbMax = ObbMax.ComponentMax(Corner);
        }

        // 대략적인 AABB 겹침 체크
        if (CapsuleMax.X < ObbMin.X || CapsuleMin.X > ObbMax.X ||
            CapsuleMax.Y < ObbMin.Y || CapsuleMin.Y > ObbMax.Y ||
            CapsuleMax.Z < ObbMin.Z || CapsuleMin.Z > ObbMax.Z)
        {
            return false;
        }

        // ───────────────────────────────────────────────────────────────
        // 2) OBB의 로컬 좌표계로 변환하여 AABB-Capsule 충돌 검사 활용
        // ───────────────────────────────────────────────────────────────
        
        // 월드 공간의 점을 OBB 로컬 공간으로 변환하는 람다
        auto ToLocalSpace = [&](const FVector& WorldPoint) -> FVector {
            const FVector Offset = WorldPoint - Obb.Center;
            return FVector(
                FVector::Dot(Offset, Obb.Axes[0]),
                FVector::Dot(Offset, Obb.Axes[1]),
                FVector::Dot(Offset, Obb.Axes[2])
            );
        };

        // 캡슐의 중심과 축을 로컬 공간으로 변환
        const FVector LocalCapsuleCenter = ToLocalSpace(Capsule.Center);
        
        // 방향 벡터는 내적으로 변환 (평행이동 제외)
        const FVector LocalCapsuleAxis = FVector(
            FVector::Dot(Capsule.Axis, Obb.Axes[0]),
            FVector::Dot(Capsule.Axis, Obb.Axes[1]),
            FVector::Dot(Capsule.Axis, Obb.Axes[2])
        ).GetSafeNormal();

        // 로컬 공간의 캡슐 생성
        const FBoundingCapsule LocalCapsule(
            LocalCapsuleCenter,
            LocalCapsuleAxis,
            Capsule.HalfHeight,
            Capsule.Radius
        );

        // OBB의 로컬 공간에서 AABB 생성 (중심이 원점, HalfExtent가 범위)
        const FAABB LocalObbAsAabb(-Obb.HalfExtent, Obb.HalfExtent);

        // 기존 AABB-Capsule 충돌 검사 함수 활용
        return Intersects(LocalObbAsAabb, LocalCapsule);
    }

    bool Intersects(const FBoundingSphere& Sphere, const FBoundingCapsule& Capsule)
    {
        // 구의 중심에서 캡슐 축까지의 최단 거리 계산
        const FVector CenterToSphere = Sphere.Center - Capsule.Center;

        // 축 방향으로의 투영 거리
        const float AxisProjection = FVector::Dot(CenterToSphere, Capsule.Axis);

        // 투영 거리를 [-HalfHeight, HalfHeight] 범위로 클램핑
        const float ClampedProjection = FMath::Clamp(AxisProjection, -Capsule.HalfHeight, Capsule.HalfHeight);

        // 축 위의 가장 가까운 점
        const FVector ClosestPointOnAxis = Capsule.Center + Capsule.Axis * ClampedProjection;

        // 가장 가까운 점에서 점까지의 거리
        const float DistSquared = (Sphere.Center - ClosestPointOnAxis).SizeSquared();

        const float RadiusSum = Sphere.Radius + Capsule.Radius;
        return DistSquared <= (RadiusSum * RadiusSum);
	}
}
