#pragma once
#include "UEContainer.h"

#define DECLARE_DELEGATE(Name, ...) TDelegate<__VA_ARGS__> Name
#define DECLARE_MULTICAST_DELEGATE(Name, ...) TMultiCastDelegate<__VA_ARGS__> Name


struct FBindingHandle
{
    uint32 ID = -1;

    FBindingHandle()
        :ID(-1) {
    }
    FBindingHandle(uint32 InID)
        :ID(InID) {
    }
    bool operator==(const FBindingHandle& InOther)
    {
        return ID == InOther.ID;
    }
};


template<typename... Args>
class TDelegate
{
public:
    using HandlerType = std::function<void(Args...)>;


    void Bind(const HandlerType& InHandler)
    {
        Handler = InHandler;
    }
   

    // 클래스 멤버 함수 바인딩
    template<typename T>
    void BindDynamic(T* Instance, void (T::* Func)(Args...))
    {
        Bind(HandlerType([Instance, Func](Args... InArgs) {
            (Instance->*Func)(InArgs...);
            }));
    }

    void UnBind()
    {
        Handler = nullptr;
    }

    void Execute(Args... InArgs)
    {
        if (Handler)
        {
            Handler(InArgs...);
        }
    }
private:
    HandlerType Handler = nullptr;

};

template<typename... Args>
class TMultiCastDelegate
{
public:
    using HandlerType = std::function<void(Args...)>;
    // 일반 함수나 람다 등록
    FBindingHandle Add(const HandlerType& Handler)
    {
        Handlers.Add(Handler);
        FBindingHandle Handle{ HandlerIndex++ };
        return Handle;
    }

    // 클래스 멤버 함수 바인딩
    template<typename T>
    FBindingHandle AddDynamic(T* Instance, void (T::* Func)(Args...))
    {
        Add(HandlerType([Instance, Func](Args... InArgs) {
            (Instance->*Func)(InArgs...);
            }));
        FBindingHandle Handle{ HandlerIndex++ };
        return Handle;
    }

    void UnBind(FBindingHandle Handle)
    {
        Handlers[Handle.ID] = nullptr;
    }

    void Broadcast(Args... InArgs)
    {
        for (HandlerType Handler : Handlers)
        {
            if (Handler)
            {
                Handler(InArgs...);
            }
        }
    }

private:
    TArray<HandlerType> Handlers;
    //핸들러 삭제를 고려하지 않음. MultiCastDelegateImproved에서 바인딩 객체를 만들고 핸들러 동적 관리할 것임.
    uint32 HandlerIndex = 0;

};