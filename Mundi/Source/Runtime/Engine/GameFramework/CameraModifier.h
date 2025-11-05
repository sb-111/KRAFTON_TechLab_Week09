#pragma once
#include "Object.h"
#include "WeakPtr.h"

// 순환 include 방지를 위한 전방 선언
class APlayerCameraManager;

class UCameraModifier : public UObject
{

public:
    DECLARE_CLASS(UCameraModifier, UObject)
private:
    // 싱글 플레이라서 사실상 의미 없음
	TWeakPtr<APlayerCameraManager> CameraOwner;
    float AlphaInTime;
    float AlphaOutTime;
    float Alpha;
    uint32 bDisabled;
    uint8 Priority;
};