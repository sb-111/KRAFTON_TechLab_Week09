#pragma once
#include "UEContainer.h"
//Delegate의 안전성을 위해 멤버 함수 바인딩 할때 weakptr을 사용하는데, 스태틱 함수, 람다 함수랑 같이 묶어서 관리하려고 만든 클래스, 추후 사용할 예정

struct FBindingHandle
{
    uint32 ID = -1;

    FBindingHandle()
        :ID(-1) {
    }
    FBindingHandle(uint32 InID)
        :ID(InID) {
    }
    FBindingHandle(const FBindingHandle& InOther)
        :ID(InOther.ID){
    }
    bool operator==(const FBindingHandle& InOther)
    {
        return ID == InOther.ID;
    }

};
template<typename... Args>
class IDelegateBinding
{
public:
	//직접 호출 불가능, 자식 클래스가 호출
	IDelegateBinding(FBindingHandle InHandle)
		:Handle(InHandle) {
	}
	virtual ~IDelegateBinding() = default;
	virtual bool IsValid() const = 0;
    virtual bool IsBoundTo(const void* Instance) const = 0;
	virtual void Execute(Args... InArgs) const = 0;
	const FBindingHandle GetHandle() const {return Handle;}

private:
	FBindingHandle Handle;
};