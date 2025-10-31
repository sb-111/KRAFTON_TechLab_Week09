#pragma once
// Delegate의 안전성을 위해 weakptr로 객체 참조하면서 사용하려고 만든 클래스, 추후 사용할 예정
#include "DelegateBinding.h"

template<typename ClassType, typename... Args>
class FDynamicBinding : public IDelegateBinding<Args...>
{

public:
	FDynamicBinding(FBindingHandle InHandle, ClassType* InObject, void (ClassType::* InFunction)(Args...) )
		:IDelegateBinding<Args...>(InHandle),
		ObjectPtr(InObject),
		Function(InFunction){
	}

	bool IsBoundTo(const void* Instance) const override { return ObjectPtr.Get() == Instance; }
	bool IsValid() const override { return ObjectPtr.IsValid(); }
	void Execute(Args... InArgs) const override
	{
		if (ObjectPtr.IsValid())
		{
			(ObjectPtr.Get()->*Function)(InArgs...);
		}
	}
private:
	TWeakPtr<ClassType> ObjectPtr = nullptr;
	void (ClassType::* Function)(Args...) ;
};