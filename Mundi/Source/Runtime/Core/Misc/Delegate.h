#pragma once
#include "DelegateBinding.h"
#include "StaticBinding.h"
#include "DynamicBinding.h"
#include "LambdaBinding.h"

#define DECLARE_DELEGATE(Name, ...) TDelegate<__VA_ARGS__> Name


template<typename... Args>
class TDelegate
{
public:
    using LambdaType = std::function<void(Args...)>;
    using StaticType = void(*)(Args...);

    ~TDelegate()
    {
        ClearBinding();
    }
    
    void BindStatic(StaticType InFunction)
    {
        ClearBinding();
        Binding = new FStaticBinding(FBindingHandle(0), InFunction);
    }
   
    void BindLambda(LambdaType InFunction)
    {
        ClearBinding();
        Binding = new FLambdaBinding(FBindingHandle(0), InFunction);
    }
    // 클래스 멤버 함수 바인딩
    template<typename T>
    void BindDynamic(T* Instance, void (T::* InFunction)(Args...) )
    {
        ClearBinding();
        Binding = new FDynamicBinding(FBindingHandle(0), Instance, InFunction);

    }

    void UnBind()
    {
        ClearBinding();
    }

    void Execute(Args... InArgs)
    {
        if (Binding && Binding->IsValid())
        {
            Binding->Execute(InArgs);
        }
    }
private:
    IDelegateBinding<Args...>* Binding = nullptr;

    void ClearBinding()
    {
        if (Binding)
        {
            delete Binding;
            Binding = nullptr;
        }
    }
};

//template<typename... Args>
//class TMultiCastDelegate
//{
//public:
//    using HandlerType = std::function<void(Args...)>;
//    // 일반 함수나 람다 등록
//    FBindingHandle Add(const HandlerType& Handler)
//    {
//        Handlers.Add(Handler);
//        FBindingHandle Handle{ HandlerIndex++ };
//        return Handle;
//    }
//
//    // 클래스 멤버 함수 바인딩
//    template<typename T>
//    FBindingHandle AddDynamic(T* Instance, void (T::* Func)(Args...))
//    {
//        Add(HandlerType([Instance, Func](Args... InArgs) {
//            (Instance->*Func)(InArgs...);
//            }));
//        FBindingHandle Handle{ HandlerIndex++ };
//        return Handle;
//    }
//
//    void UnBind(FBindingHandle Handle)
//    {
//        Handlers[Handle.ID] = nullptr;
//    }
//
//    void Broadcast(Args... InArgs)
//    {
//        for (HandlerType Handler : Handlers)
//        {
//            if (Handler)
//            {
//                Handler(InArgs...);
//            }
//        }
//    }
//
//private:
//    TArray<HandlerType> Handlers;
//    //핸들러 삭제를 고려하지 않음. MultiCastDelegateImproved에서 바인딩 객체를 만들고 핸들러 동적 관리할 것임.
//    uint32 HandlerIndex = 0;
//
//};