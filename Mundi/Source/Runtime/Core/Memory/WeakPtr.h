#pragma once
// Delegate의 안전성을 위해 필요한 weakptr, 추후 사용할 예정
//
//template<typename T>
//class TWeakPtr
//{
//public:
//	TWeakPtr() = default;
//
//	TWeakPtr(UObject* InObject)
//	{
//		if (InObject)
//		{
//			Index = InObject->InternalIndex;
//		}
//	}
//
//	TWeakPtr& operator=(UObject* InObject)
//	{
//		if (InObject)
//		{
//			Index = InObject->InternalIndex;
//		}
//		else
//		{
//			Index = -1;
//		}
//	}
//
//	bool IsValid() const { return UObject::GetObjectFromIndex(Index); }
//
//	T* Get() const { return static_cast<T*>(UObject::GetObjectFromIndex(Index); }
//
//	T* operator->() { return Get(); }
//	T& operator*() { return *Get(); }
//
//private:
//	// 인덱스 재활용 안 한다고 가정
//	uint32 Index = -1;
//};