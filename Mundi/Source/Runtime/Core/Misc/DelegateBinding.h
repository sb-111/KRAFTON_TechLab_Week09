#pragma once


struct FBindingHandle
{
	uint64 Id;

	bool operator==(const FBindingHandle& InOther)
	{
		return Id == InOther.Id;
	}
};
template<typename... Args>
class IDelegateBinding
{
public:
	virtual ~IDelegateBinding() = default;
	virtual bool IsValid() const = 0;
	virtual void Execute(Args... InArgs) const = 0;
	const FBindingHandle GetHandle() const {return Handle;}

private:
	FBindingHandle Handle;
};