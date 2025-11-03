#include "pch.h"
#include "ShapeComponent.h"
#include "World.h"
#include "WorldPhysics.h"
#include "JsonSerializer.h"

IMPLEMENT_CLASS(UShapeComponent)

void UShapeComponent::Destroy()
{
    UWorld* World = GetWorld();
    UWorldPhysics* Physics = World ? World->GetWorldPhysics() : nullptr;

	//월드가 사라질 때 델리게이트가 남지 않도록 먼저 해제
    UnbindCollisionDelegates();

    if (Physics)
    {
		//Physics 캐시에 남아 있는 충돌 정보도 정리
        Physics->UnregisterCollision(this);
    }

    Super::Destroy();
}

void UShapeComponent::OnRegister(UWorld* InWorld)
{
	Super::OnRegister(InWorld);
	UpdateBound();
	//에디터/PIE 모두에서 새 월드 기준으로 다시 바인딩
	BindCollisionDelegates();
}

void UShapeComponent::OnUnregister()
{
	//월드에서 빠질 때 이벤트 연결 해제
	UnbindCollisionDelegates();
	Super::OnUnregister();
}

void UShapeComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
	//PIE 복제 시 기존 핸들 ID가 복사되지 않도록 초기화
	BeginOverlapHandle = FBindingHandle();
	EndOverlapHandle = FBindingHandle();
}

void UShapeComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FVector4 ColorVec = ShapeColor.ToFVector4();
		if (FJsonSerializer::ReadVector4(InOutHandle, "ShapeColor", ColorVec, ColorVec, false))
		{
			ShapeColor = FLinearColor(ColorVec);
		}

		FJsonSerializer::ReadBool(InOutHandle, "bDrawOnlyIfSelected", bDrawOnlyIfSelected, bDrawOnlyIfSelected, false);
	}
	else
	{
		InOutHandle["ShapeColor"] = FJsonSerializer::Vector4ToJson(ShapeColor.ToFVector4());
		InOutHandle["bDrawOnlyIfSelected"] = bDrawOnlyIfSelected;
	}
}

void UShapeComponent::OnSerialized()
{
    Super::OnSerialized();
}

bool UShapeComponent::Intersects(const UShapeComponent* A, const UShapeComponent* B)
{
	if (A->GetShapeType() == EShapeType::None || B->GetShapeType() == EShapeType::None) return false;

	return A->Intersects(B);
}

void UShapeComponent::BindCollisionDelegates()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		//월드가 없으면 이후에 안전하게 재바인딩할 수 있도록 초기화
		BeginOverlapHandle = FBindingHandle();
		EndOverlapHandle = FBindingHandle();
		return;
	}

	if (UWorldPhysics* Physics = World->GetWorldPhysics())
	{
		//이전 월드에서 사용하던 핸들을 정리
		if (BeginOverlapHandle.ID != static_cast<uint32>(-1))
		{
			Physics->OnShapeBeginOverlap().Remove(BeginOverlapHandle);
			BeginOverlapHandle = FBindingHandle();
		}
		if (EndOverlapHandle.ID != static_cast<uint32>(-1))
		{
			Physics->OnShapeEndOverlap().Remove(EndOverlapHandle);
			EndOverlapHandle = FBindingHandle();
		}

		BeginOverlapHandle = Physics->OnShapeBeginOverlap().AddDynamic(this, &UShapeComponent::HandleBeginOverlap);
		EndOverlapHandle = Physics->OnShapeEndOverlap().AddDynamic(this, &UShapeComponent::HandleEndOverlap);
	}
}

void UShapeComponent::UnbindCollisionDelegates()
{
	UWorld* World = GetWorld();
	UWorldPhysics* Physics = World ? World->GetWorldPhysics() : nullptr;

	if (Physics)
	{
		//이미 등록된 핸들이 있다면 제거
		if (BeginOverlapHandle.ID != static_cast<uint32>(-1))
		{
			Physics->OnShapeBeginOverlap().Remove(BeginOverlapHandle);
		}
		if (EndOverlapHandle.ID != static_cast<uint32>(-1))
		{
			Physics->OnShapeEndOverlap().Remove(EndOverlapHandle);
		}
	}

	BeginOverlapHandle = FBindingHandle();
	EndOverlapHandle = FBindingHandle();
}

void UShapeComponent::HandleBeginOverlap(UShapeComponent* A, UShapeComponent* B)
{
	if (A == this && B && B != this)
	{
		OnCollisionBegin(B);
	}
	else if (B == this && A && A != this)
	{
		OnCollisionBegin(A);
	}
}

void UShapeComponent::HandleEndOverlap(UShapeComponent* A, UShapeComponent* B)
{
	if (A == this && B && B != this)
	{
		OnCollisionEnd(B);
	}
	else if (B == this && A && A != this)
	{
		OnCollisionEnd(A);
	}
}
