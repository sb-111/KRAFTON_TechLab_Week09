#include "pch.h"
#include "PlayerComponent.h"

IMPLEMENT_CLASS(UPlayerComponent)

BEGIN_PROPERTIES(UPlayerComponent)
    MARK_AS_COMPONENT("플레이어 컴포넌트", "플레이어 액터를 식별하기 위한 빈 컴포넌트입니다.")
END_PROPERTIES()

UPlayerComponent::UPlayerComponent()
{
    bCanEverTick = false;
    bTickEnabled = false;
}
