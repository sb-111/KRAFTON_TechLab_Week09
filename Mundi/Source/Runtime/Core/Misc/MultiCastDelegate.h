#pragma once

#include "DelegateBinding.h"
#include "StaticBinding.h"
#include "DynamicBinding.h"
#include "LambdaBinding.h"
#define DECLARE_MULTICAST_DELEGATE(Name, ...) TMultiCastDelegate<__VA_ARGS__> Name

template<typename... Args>
class TMultiCastDelegate
{
public:
	using LambdaType = std::function<void(Args...)>;
	using StaticType = void(*)(Args...);

	~TMultiCastDelegate()
	{
		ClearBindings();
	}

	// Add가 호출되는 시점에 리스트에 빈 공간이 없어야함
	FBindingHandle AddStatic(StaticType InFunction)
	{
		FBindingHandle Handle{ FBindingHandle(NextID++) };

		Bindings.Add(new FStaticBinding(Handle, InFunction));
		return Handle;
	}

	// 람다를 쓰는 경우 유효성 검사는 프로그래머가 직접 해줘야 함.
	FBindingHandle AddLambda(LambdaType InFunction)
	{
		FBindingHandle Handle{ FBindingHandle(NextID++) };
		Bindings.Add(new FLambdaBinding(Handle, InFunction));
		return Handle;
	}

	template<typename T>
	FBindingHandle AddDynamic(T* Instance, void(T::* InFunction)(Args...) )
	{
		FBindingHandle Handle{ FBindingHandle(NextID++) };
		Bindings.Add(new FDynamicBinding(Handle, Instance, InFunction));
		return Handle;
	}

	void Remove(FBindingHandle Handle)
	{
		for (uint32 Index = 0; Index < Bindings.Num(); Index++)
		{
			if (Bindings[Index] && Bindings[Index]->GetHandle() == Handle)
			{
				delete Bindings[Index];
				
				Bindings.SwapAndPop(Index);
				break;
			}
		}
	}
	template<typename T>
	void Remove(T* Instance)
	{
		for (int32 Index = Bindings.Num()-1; Index >= 0; Index--)
		{
			if (Bindings[Index] && Bindings[Index]->IsBoundTo(Instance))
			{
				delete Bindings[Index];
				Bindings.SwapAndPop(Index);
			}
		}
	}
	void Broadcast(Args... InArgs)
	{
		for (IDelegateBinding<Args...>* Binding : Bindings)
		{
			if (Binding && Binding->IsValid())
			{
				Binding->Execute(InArgs...);
			}
		}
	}



private:

	uint32 NextID = 0;
	TArray<IDelegateBinding<Args...>*> Bindings;

	void ClearBindings()
	{
		for (IDelegateBinding<Args...>* Binding : Bindings)
		{
			if (Binding)
			{
				delete Binding;
			}
		}
		Bindings.clear();
	}
};