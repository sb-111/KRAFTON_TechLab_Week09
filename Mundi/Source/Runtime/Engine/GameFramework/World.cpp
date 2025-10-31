#include "pch.h"
#include "SelectionManager.h"
#include "Picking.h"
#include "CameraActor.h"
#include "StaticMeshActor.h"
#include "CameraComponent.h"
#include "ObjectFactory.h"
#include "TextRenderComponent.h"
#include "FViewport.h"
#include "Windows/SViewportWindow.h"
#include "USlateManager.h"
#include "StaticMesh.h"
#include "ObjManager.h"
#include "WorldPartitionManager.h"
#include "PrimitiveComponent.h"
#include "Octree.h"
#include "BVHierarchy.h"
#include "Frustum.h"
#include "Occlusion.h"
#include "Gizmo/GizmoActor.h"
#include "Grid/GridActor.h"
#include "StaticMeshComponent.h"
#include "DirectionalLightActor.h"
#include "Frustum.h"
#include "Level.h"
#include "LightManager.h"

IMPLEMENT_CLASS(UWorld)

UWorld::UWorld()
	: Partition(new UWorldPartitionManager())
{
	SelectionMgr = std::make_unique<USelectionManager>();
	//PIE의 경우 Initalize 없이 빈 Level 생성만 해야함
	Level = std::make_unique<ULevel>();
	LightManager = std::make_unique<FLightManager>();

}

UWorld::~UWorld()
{
	if (Level)
	{
		for (AActor* Actor : Level->GetActors())
		{
			ObjectFactory::DeleteObject(Actor);
		}
		Level->Clear();
	}
	for (AActor* Actor : EditorActors)
	{
		ObjectFactory::DeleteObject(Actor);
	}
	EditorActors.clear();

	GridActor = nullptr;
	GizmoActor = nullptr;
}

void UWorld::Initialize()
{
	CreateLevel();

	InitializeGrid();
	InitializeGizmo();
	InitializeLuaState();
}

void UWorld::InitializeGrid()
{
	GridActor = NewObject<AGridActor>();
	GridActor->SetWorld(this);
	GridActor->Initialize();

	EditorActors.push_back(GridActor);
}

void UWorld::InitializeGizmo()
{
	GizmoActor = NewObject<AGizmoActor>();
	GizmoActor->SetWorld(this);
	GizmoActor->SetActorTransform(FTransform(FVector{ 0, 0, 0 }, FQuat::MakeFromEulerZYX(FVector{ 0, -90, 0 }),
		FVector{ 1, 1, 1 }));

	EditorActors.push_back(GizmoActor);
}

void UWorld::InitializeLuaState()
{
	// Lua 표준 라이브러리 로드
	LuaState.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::coroutine);

	// FVector 바인딩 (설계도 등록)
	LuaState.new_usertype<FVector>("Vector", // Lua에서 사용할 타입 이름
		// 생성자 바인딩: Lua에서 Vector(1, 2, 3) 처럼 호출 가능
		sol::constructors<FVector(), FVector(float, float, float)>(),
		// 멤버 변수 바인딩: Lua에서 vec.x = 10처럼 접근 가능
		"x", &FVector::X,
		"y", &FVector::Y,
		"z", &FVector::Z,
		// 연산자 오버로딩: Lua에서 vec1 +- vec2 가능
		sol::meta_function::addition, [](const FVector& a, const FVector& b) {
			return FVector(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
		},
		sol::meta_function::subtraction, [](const FVector& a, const FVector& b) {
			return FVector(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
		},
		// 곱셈 오버로딩 (벡터 * 스칼라, 스칼라 * 벡터 둘 다 지원)
		sol::meta_function::multiplication, sol::overload(
			[](const FVector& v, float f) { return v * f; },
			[](float f, const FVector& v) { return v * f; }
		),
		sol::meta_function::division, [](const FVector& v, float f) {
			return v / f;
		}
	);

	// FTransform 바인딩
	LuaState.new_usertype<FTransform>("Transform",
		sol::constructors<FTransform()>(),
		// 멤버 변수 바인딩 (간단한 구조체라 Get/Set 대신 직접 접근)
		"Location", &FTransform::Translation,
		"Rotation", &FTransform::Rotation,
		"Scale", &FTransform::Scale3D
	);

	// AActor 바인딩
	LuaState.new_usertype<AActor>("Actor",
		// 함수 바인딩: Lua의 obj:GetActorLocation() 호출이 C++ Actor::GetActorLocation 실행
		"GetActorLocation", &AActor::GetActorLocation,
		"SetActorLocation", &AActor::SetActorLocation,
		"GetActorRotation", &AActor::GetActorRotation,
		// 함수 오버로딩: SetActorRotation이 FVector, FQuat 두 버전을 받으므로
		// sol::overload를 사용해 Lua에 둘 다 등록
		"SetActorRotation", sol::overload(
			static_cast<void(AActor::*)(const FVector&)>(&AActor::SetActorRotation),
			static_cast<void(AActor::*)(const FQuat&)>(&AActor::SetActorRotation)
		),
		"GetActorScale", &AActor::GetActorScale,
		"SetActorScale", &AActor::SetActorScale,
		"GetActorTransform", &AActor::GetActorTransform,
		"SetActorTransform", &AActor::SetActorTransform,

		// 유틸리티 함수들
		"GetActorForward", &AActor::GetActorForward,
		"GetActorRight", &AActor::GetActorRight,
		"GetActorUp", &AActor::GetActorUp,
		"AddActorWorldLocation", &AActor::AddActorWorldLocation,
		"AddActorLocalLocation", &AActor::AddActorLocalLocation,
		// Lua에서 C++ 액터를 소멸시킬 수 있게 함(주의)
		"Destroy", &AActor::Destroy,
		// Lua에서 액터 이름을 가져올 수 있게 함(디버깅 유용)
		"GetName", &AActor::GetName
	);

	UE_LOG("Lua State initialized successfully");
}

void UWorld::Tick(float DeltaSeconds)
{
	Partition->Update(DeltaSeconds, /*budget*/256);

//순서 바꾸면 안댐
	if (Level)
	{
		for (AActor* Actor : Level->GetActors())
		{
			if (Actor && (Actor->CanTickInEditor() || bPie))
			{
				Actor->Tick(DeltaSeconds);
			}
		}
	}
	for (AActor* EditorActor : EditorActors)
	{
		if (EditorActor && !bPie) EditorActor->Tick(DeltaSeconds);
	}
}

UWorld* UWorld::DuplicateWorldForPIE(UWorld* InEditorWorld)
{
	// 레벨 새로 생성
	// 월드 카피 및 월드에 이 새로운 레벨 할당
	// 월드 컨텍스트 새로 생성(월드타입, 카피한 월드)
	// 월드의 레벨에 원래 Actor들 다 복사
	// 해당 월드의 Initialize 호출?

	//ULevel* NewLevel = ULevelService::CreateNewLevel();
	UWorld* PIEWorld = NewObject<UWorld>(); // 레벨도 새로 생성됨
	PIEWorld->bPie = true;
	
	FWorldContext PIEWorldContext = FWorldContext(PIEWorld, EWorldType::Game);
	GEngine.AddWorldContext(PIEWorldContext);
	

	const TArray<AActor*>& SourceActors = InEditorWorld->GetLevel()->GetActors();
	for (AActor* SourceActor : SourceActors)
	{
		if (!SourceActor)
		{
			UE_LOG("Duplicate failed: SourceActor is nullptr");
			continue;
		}

		AActor* NewActor = SourceActor->Duplicate();

		if (!NewActor)
		{
			UE_LOG("Duplicate failed: NewActor is nullptr");
			continue;
		}
		PIEWorld->AddActorToLevel(NewActor);
		NewActor->SetWorld(PIEWorld);
		
	}

	return PIEWorld;
}

FString UWorld::GenerateUniqueActorName(const FString& ActorType)
{
	// GetInstance current count for this type
	int32& CurrentCount = ObjectTypeCounts[ActorType];
	FString UniqueName = ActorType + "_" + std::to_string(CurrentCount);
	CurrentCount++;
	return UniqueName;
}

//
// 액터 제거
//
bool UWorld::DestroyActor(AActor* Actor)
{
	if (!Actor) return false;

	// 재진입 가드
	if (Actor->IsPendingDestroy()) return false;
	Actor->MarkPendingDestroy();

	// 선택/UI 해제
	if (SelectionMgr) SelectionMgr->DeselectActor(Actor);

	// 게임 수명 종료
	Actor->EndPlay(EEndPlayReason::Destroyed);

	// 컴포넌트 정리 (등록 해제 → 파괴)
	TArray<USceneComponent*> Components = Actor->GetSceneComponents();
	for(USceneComponent* Comp : Components)
	{
		if (Comp)
		{
			Comp->SetOwner(nullptr); // 소유자 해제
		}
	}

	// 월드 자료구조에서 소유한 컴포넌트 내리기
	OnActorDestroyed(Actor);

	Actor->UnregisterAllComponents(/*bCallEndPlayOnBegun=*/true);
	Actor->DestroyAllComponents();
	Actor->ClearSceneComponentCaches();

// 레벨에서 제거 시도
	if (Level && Level->RemoveActor(Actor))
	{
		// 옥트리에서 제거
		OnActorDestroyed(Actor);

		// 메모리 해제
		ObjectFactory::DeleteObject(Actor);

		// 삭제된 액터 정리
		if (SelectionMgr)
		{
			SelectionMgr->CleanupInvalidActors();
			SelectionMgr->ClearSelection();
		}

		return true; // 성공적으로 삭제
	}

	return false; // 레벨에 없는 액터
}

void UWorld::OnActorSpawned(AActor* Actor)
{
	if (!Actor) return;

	// Level에 Actor 추가 (SceneManagerWidget에 표시되도록)
	AddActorToLevel(Actor);
}

void UWorld::OnActorDestroyed(AActor* Actor)
{
	if (Actor)
	{
		Partition->Unregister(Actor);
	}
}

inline FString RemoveObjExtension(const FString& FileName)
{
	const FString Extension = ".obj";

	// 마지막 경로 구분자 위치 탐색 (POSIX/Windows 모두 지원)
	const uint64 Sep = FileName.find_last_of("/\\");
	const uint64 Start = (Sep == FString::npos) ? 0 : Sep + 1;

	// 확장자 제거 위치 결정
	uint64 End = FileName.size();
	if (End >= Extension.size() &&
		FileName.compare(End - Extension.size(), Extension.size(), Extension) == 0)
	{
		End -= Extension.size();
	}

	// 베이스 이름(확장자 없는 파일명) 반환
	if (Start <= End)
		return FileName.substr(Start, End - Start);

	// 비정상 입력 시 원본 반환 (안전장치)
	return FileName;
}

void UWorld::CreateLevel()
{
	if (SelectionMgr) SelectionMgr->ClearSelection();
	
	SetLevel(ULevelService::CreateNewLevel());
	// 이름 카운터 초기화: 씬을 새로 시작할 때 각 BaseName 별 suffix를 0부터 다시 시작
	ObjectTypeCounts.clear();
}

//어느 레벨이든 기본적으로 존재하는 엑터(디렉셔널 라이트) 생성
void UWorld::SpawnDefaultActors()
{
	SpawnActor<ADirectionalLightActor>();
}
void UWorld::SetLevel(std::unique_ptr<ULevel> InLevel)
{
    // Make UI/selection safe before destroying previous actors
    if (SelectionMgr) SelectionMgr->ClearSelection();

    // Cleanup current
    if (Level)
    {
        for (AActor* Actor : Level->GetActors())
        {
            ObjectFactory::DeleteObject(Actor);
        }
        Level->Clear();
    }
    // Clear spatial indices
    Partition->Clear();

    Level = std::move(InLevel);

    // Adopt actors: set world and register
    if (Level)
    {
		Partition->BulkRegister(Level->GetActors());
        for (AActor* A : Level->GetActors())
        {
            if (!A) continue;
            A->SetWorld(this);
        }
    }

    // Clean any dangling selection references just in case
    if (SelectionMgr) SelectionMgr->CleanupInvalidActors();
}

void UWorld::AddActorToLevel(AActor* Actor)
{
	if (Level) 
	{
		Level->AddActor(Actor);
		Partition->Register(Actor);
	}
}

AActor* UWorld::SpawnActor(UClass* Class, const FTransform& Transform)
{
	// 유효성 검사: Class가 유효하고 AActor를 상속했는지 확인
	if (!Class || !Class->IsChildOf(AActor::StaticClass()))
	{
		UE_LOG("SpawnActor failed: Invalid class provided.");
		return nullptr;
	}

	// ObjectFactory를 통해 UClass*로부터 객체 인스턴스 생성
	AActor* NewActor = Cast<AActor>(ObjectFactory::NewObject(Class));
	if (!NewActor)
	{
		UE_LOG("SpawnActor failed: ObjectFactory could not create an instance of");
		return nullptr;
	}

	// 초기 트랜스폼 적용
	NewActor->SetActorTransform(Transform);

	// 월드 참조 설정
	NewActor->SetWorld(this);

	// 현재 레벨에 액터 등록
	AddActorToLevel(NewActor);

	return NewActor;
}

AActor* UWorld::SpawnActor(UClass* Class)
{
	// 기본 Transform(원점)으로 스폰하는 메인 함수를 호출합니다.
	return SpawnActor(Class, FTransform());
}
