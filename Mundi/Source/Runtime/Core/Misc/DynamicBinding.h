#pragma once
#include "DelegateBinding.h"


template<typename... Args>
class FDynamicBinding : public IDelegateBinding<Args...>
{

};