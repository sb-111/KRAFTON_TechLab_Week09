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

	// 객체가 소멸될 때 명시적으로 Remove를 호출해줘야함.
	// 안 해줘도 WeakPtr을 쓰기 때문에 문제가 생기진 않는데 쌓이면 쌓일 수록 성능 부담이 생길 수도 있음.
	// 가비지컬렉터처럼 일정 주기마다 알아서 리스트 관리를 해주면 좋을듯.
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
	// 현재 Duplicate에서 얕은복사하면서 NextID가 그대로 유지되고있음.
	// Duplicate포함 총 4,294,967,296개 이상의 객체를 바인딩 하는 경우 문제가 생김
	// Duplicate하거나 PIE에서 돌아올 때 초기화 해줘야 함.
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