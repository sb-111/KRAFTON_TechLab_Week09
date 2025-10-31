#pragma once
#include "DelegateBinding.h"

template<typename ClassType, typename... Args>
class FStaticBinding : public IDelegateBinding<Args...>
{
public:
	FStaticBinding(FBindingHandle InHandle, void (*InFunction)(Args...) )
		: IDelegateBinding<Args...>(InHandle),
		Function(InFunction) {
	}

	bool IsBoundTo(const void* Instance) const override { return false; }
	bool IsValid() const override { return true; }
	void Execute(Args... InArgs) const override
	{
		(*Function)(InArgs...);
	}
private:
	void (ClassType::* Function)(Args...) ;
};