#pragma once
// Delegate의 안전성을 위해 weakptr로 객체 참조하면서 사용하려고 만든 클래스, 추후 사용할 예정
//#include "DelegateBinding.h"
//
//template<typename ClassType, typename... Args>
//class FDynamicBinding : public IDelegateBinding<Args...>
//{
//
//public:
//	FDynamicBinding(FDelegateHandle InHandle, ClassType* InObject, void (ClassType::* InFunction)(Args...)
//		:IDelegateBinding(InHandle),
//		ObjectPtr(InObject),
//		Function(InFunction){
//	}
//
//	bool IsValid() const override { return ObjectPtr.IsValid(); }
//	void Execute(Args... InArgs) const override
//	{
//
//	}
//private:
//	TWeakPtr<ClassType> ObjectPtr = nullptr;
//	void (ClassType::* Function)(Args...);
//};