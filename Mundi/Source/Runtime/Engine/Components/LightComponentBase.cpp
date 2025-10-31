#include "pch.h"
#include "LightComponentBase.h"

IMPLEMENT_CLASS(ULightComponentBase)

BEGIN_PROPERTIES(ULightComponentBase)
    ADD_PROPERTY_RANGE(float, Intensity, "Light", 0.0f, 100.0f, true, "라이트의 강도입니다.")
    ADD_PROPERTY(FLinearColor, LightColor, "Light", true, "라이트의 색상입니다.")
    // 에디터 컴포넌트 섹션에서 on/off 할 수 있도록 카테고리를 "컴포넌트"로 배치
    ADD_PROPERTY(bool, bCastShadows, "컴포넌트", true, "그림자를 그릴지 결정합니다")
END_PROPERTIES()

ULightComponentBase::ULightComponentBase()
{
	bWantsOnUpdateTransform = true;
	Intensity = 1.0f;
	LightColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

ULightComponentBase::~ULightComponentBase()
{
}

void ULightComponentBase::UpdateLightData()
{
	// 자식 클래스에서 오버라이드
}

void ULightComponentBase::OnSerialized()
{
	Super::OnSerialized();

}

void ULightComponentBase::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}
