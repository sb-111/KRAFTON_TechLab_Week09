#pragma once


enum class ECameraProjectionMode
{
    Perspective,
    Orthographic
};
inline float BezierValue[5];

struct FMinimalViewInfo
{
    FVector Location;
    FQuat Rotation;
    // 이번 발제에서는 뷰포트 aspect를 그대로 쓸 것 같은데 일단 추가함
    float Aspect;
    float ZNear;
    float ZFar;
    float Fov;
    ECameraProjectionMode ProjectionMode = ECameraProjectionMode::Perspective;
};