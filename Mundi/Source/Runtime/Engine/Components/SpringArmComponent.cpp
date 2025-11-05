#include "pch.h"
#include "SpringArmComponent.h"

IMPLEMENT_CLASS(USpringArmComponent)

BEGIN_PROPERTIES(USpringArmComponent)
	MARK_AS_COMPONENT("스프링 암 컴포넌트", "자손을 부모로부터 고정된 거리에 유지하려고 하지만, 다른 물체와 겹치면 콜리젼이 되지 않는 거리까지 자손과의 거리를 줄입니다.")
END_PROPERTIES()


FTransform USpringArmComponent::GetSocketWorldTransform() const
{
	// GetWorldTransform()은 fallback
	return bSocketValid ? CachedSocketWorld : GetWorldTransform();
}

void USpringArmComponent::OnTransformUpdated()
{
    bSocketValid = false;  // 부모가 움직였거나 내 로컬이 바뀌면 소켓 캐시가 구버전이 됨

    EvaluateArm(0.0f);  // 간이 모드: 여기서 바로 갱신해도 됨(DeltaTime이 없으면 랙 미적용)

	Super::OnTransformUpdated();
}

void USpringArmComponent::EvaluateArm(float DeltaTime)
{
	// To be implemented
}