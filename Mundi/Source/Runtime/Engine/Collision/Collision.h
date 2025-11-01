#pragma once
struct FAABB;
struct FOBB;
struct FBoundingSphere;
struct FBoundingCapsule;

namespace Collision
{
    bool Intersects(const FAABB& Aabb, const FOBB& Obb);

    bool Intersects(const FAABB& Aabb, const FBoundingSphere& Sphere);

	bool Intersects(const FAABB& Aabb, const FBoundingCapsule& Capsule);

	bool Intersects(const FOBB& Obb, const FBoundingSphere& Sphere);

	bool Intersects(const FOBB& Obb, const FBoundingCapsule& Capsule);

	bool Intersects(const FBoundingSphere& Sphere, const FBoundingCapsule& Capsule);
}