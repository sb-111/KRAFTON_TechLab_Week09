#pragma once
#include "Object.h"
#include "WeakPtr.h"

// 순환 include 방지를 위한 전방 선언
class APlayerCameraManager;
struct FPostProcessSettings;

class UCameraModifier : public UObject
{

public:
    DECLARE_CLASS(UCameraModifier, UObject)

     /** 
     * @brief 이 모디파이어가 최종 PP 설정에 '수정'을 가하는 함수
     * @param PostSettings 이 함수가 직접 '수정'할 '입출력' 파라미터
     */
    virtual void ModifyPostProcess(float DeltaTime, FPostProcessSettings& PostProcessSettings) {}

    bool IsDisabled() const { return bDisabled; }
    void SetDisabled(bool bInDisabled) { bDisabled = bInDisabled; }

private:
    // 싱글 플레이라서 사실상 의미 없음
	TWeakPtr<APlayerCameraManager> CameraOwner;
    float AlphaInTime;
    float AlphaOutTime;
    float Alpha;
    uint32 bDisabled;
    uint8 Priority;
};