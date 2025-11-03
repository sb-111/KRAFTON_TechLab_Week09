#pragma once
#include "ActorComponent.h"

class UPlayerComponent : public UActorComponent
{
public:
    DECLARE_CLASS(UPlayerComponent, UActorComponent)
    GENERATED_REFLECTION_BODY()

    UPlayerComponent();
    ~UPlayerComponent() override = default;
};
