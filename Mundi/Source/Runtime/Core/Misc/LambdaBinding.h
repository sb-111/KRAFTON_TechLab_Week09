#pragma once
#include "DelegateBinding.h"

template<typename ClassType, typename... Args>
class FLambdaBinding : public IDelegateBinding<Args...>
{

public:

	using HandlerType = std::function<void(Args...)>;

	FLambdaBinding(FBindingHandle InHandle, HandlerType InFunction)
		: IDelegateBinding<Args...>(InHandle),
		Function(InFunction) {
	}
	
	bool IsBoundTo(const void* Instance) const override { return false; }
	// 람다를 쓰는 경우 유효성 검사는 프로그래머가 직접 해줘야 함.
	// 객체 정보가 람다에 박혀있어서 방법이 없음
	bool IsValid() const override { return true; }
	void Execute(Args... InArgs) const override
	{
		Function(InArgs);
	}
private:
	HandlerType Function;
};