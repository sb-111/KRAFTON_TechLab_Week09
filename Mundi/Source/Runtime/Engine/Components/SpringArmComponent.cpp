#include "pch.h"
#include "SpringArmComponent.h"
#include "World.h"
#include "WorldPartitionManager.h"
#include "BVHierarchy.h"
#include "Picking.h"
#include "Actor.h"
#include "Renderer.h"

IMPLEMENT_CLASS(USpringArmComponent)

BEGIN_PROPERTIES(USpringArmComponent)
	MARK_AS_COMPONENT("스프링 암 컴포넌트", "자손을 부모로부터 고정된 거리에 유지하려고 하지만, 다른 물체와 겹치면 콜리젼이 되지 않는 거리까지 자손과의 거리를 줄입니다.")
	ADD_PROPERTY_RANGE(float, TargetArmLength, "SpringArm", 0.0f, 100.0f, true, "목표 암 길이입니다.")
	ADD_PROPERTY(FVector, SocketOffset, "SpringArm", true, "소켓 오프셋입니다.")
	ADD_PROPERTY(FVector, TargetOffset, "SpringArm", true, "피벗 보정 오프셋입니다.")
	ADD_PROPERTY_RANGE(float, BackoffEpsilon, "SpringArm", 0.0f, 5.0f, true, "충돌 시 떨림을 완화하기 위해 추가로 당기는 오프셋입니다.")
	ADD_PROPERTY(bool, bUseControllerRotation, "SpringArm", true, "플레이어 컨트롤러의 회전값을 적용합니다")
END_PROPERTIES()

USpringArmComponent::USpringArmComponent()
{
	bCanEverTick = true;
	bTickEnabled = true;
	bTickInEditor = true;
}

void USpringArmComponent::OnRegister(UWorld* InWorld)
{
	Super_t::OnRegister(InWorld);
	if(Owner)
		Owner->SetTickInEditor(true);
}

void USpringArmComponent::BeginPlay()
{
	InitialRotation = GetRelativeRotation();
}

void USpringArmComponent::TickComponent(float DeltaTime)
{
	if (!IsComponentTickEnabled()) return;

	Super_t::TickComponent(DeltaTime);

	APlayerController* PlayerController = GWorld->GetPlayerController();
	if (bUseControllerRotation && PlayerController)
	{
		SetWorldRotation(PlayerController->GetControlRotation());
	}
	EvaluateArm(DeltaTime);
}

FTransform USpringArmComponent::GetSocketWorldTransform() const
{
	if (!bSocketValid)
	{
		// const 함수 내에서 멤버 업데이트 하기 위해 const_cast 사용
		const_cast<USpringArmComponent*>(this)->EvaluateArm(0.0f);
	}
	return CachedSocketWorld;
}

void USpringArmComponent::RenderDebugVolume(URenderer* Renderer) const
{
	const FLinearColor LineColor(1.0f, 0.0f, 0.0f, 1.0f);

	FVector Start = GetWorldTransform().TransformPosition(TargetOffset);
	FVector End = GetSocketWorldTransform().Translation;
	Renderer->AddLine(Start, End, LineColor.ToFVector4());
}

void USpringArmComponent::EvaluateArm(float DeltaTime)
{
	const FTransform worldTransform = GetWorldTransform();
	const FQuat worldRotation = worldTransform.Rotation;
	const FVector worldLocation = worldTransform.Translation;

	
	// 피벗 위치: 부모 기준 TargetOffset 적용
	const FVector pivotWorld = worldLocation + worldRotation.RotateVector(TargetOffset);

	// 암이 뻗어 나갈 방향 (기본은 -X 축)
	FVector armDirection = worldRotation.RotateVector(FVector(-1.0f, 0.0f, 0.0f)).GetSafeNormal();
	if (armDirection.IsZero())
	{
		armDirection = FVector(-1.0f, 0.0f, 0.0f);
	}

	const float desiredLength = FMath::Max(0.0f, TargetArmLength);
	float resolvedLength = desiredLength;

	// 정밀 레이캐스트로 충돌 검사
	if (desiredLength > KINDA_SMALL_NUMBER)
	{
		UWorld* world = GetWorld();
		if (!world)
		{
			world = GWorld;
		}

		if (world)
		{
			if (UWorldPartitionManager* partition = world->GetPartitionManager())
			{
				if (FBVHierarchy* bvh = partition->GetBVH())
				{
					bvh->FlushRebuild();

					FRay ray;
					ray.Origin = pivotWorld;
					ray.Direction = armDirection;

					float bestT = desiredLength;
					AActor* hitActor = nullptr;
					bvh->QueryRayClosestStrict(ray, hitActor, bestT);

					if (hitActor && hitActor != GetOwner() && bestT < resolvedLength)
					{
						resolvedLength = FMath::Max(0.0f, bestT - BackoffEpsilon);
					}
				}
			}
		}
	}

	const FVector socketWorldLocation =
		pivotWorld +
		armDirection * resolvedLength +
		worldRotation.RotateVector(SocketOffset);

	CachedSocketWorld = FTransform(socketWorldLocation, worldRotation, worldTransform.Scale3D);
	bSocketValid = true;
}

void USpringArmComponent::DuplicateSubObjects()
{
	Super_t::DuplicateSubObjects();
}