#include "pch.h"
#include <string>
#include "TargetActorTransformWidget.h"
#include "UIManager.h"
#include "ImGui/imgui.h"
#include "Vector.h"
#include "World.h"
#include "ResourceManager.h"
#include "SelectionManager.h"
#include "WorldPartitionManager.h"
#include "PropertyRenderer.h"

#include "Actor.h"
#include "Grid/GridActor.h"
#include "Gizmo/GizmoActor.h"
#include "StaticMeshActor.h"
#include "FakeSpotLightActor.h"

#include "StaticMeshComponent.h"
#include "TextRenderComponent.h"
#include "CameraComponent.h"
#include "BillboardComponent.h"
#include "DecalComponent.h"
#include "PerspectiveDecalComponent.h"
#include "HeightFogComponent.h"
#include "DirectionalLightComponent.h"
#include "AmbientLightComponent.h"
#include "PointLightComponent.h"
#include "SpotLightComponent.h"
#include "SceneComponent.h"
#include "ScriptComponent.h"
#include "Color.h"
#include "RenderManager.h"

#include "ShadowSystem.h"

#include "CameraActor.h"

using namespace std;

IMPLEMENT_CLASS(UTargetActorTransformWidget)

namespace
{
	struct FAddableComponentDescriptor
	{
		const char* Label;
		UClass* Class;
		const char* Description;
	};

	const TArray<FAddableComponentDescriptor>& GetAddableComponentDescriptors()
	{
		static TArray<FAddableComponentDescriptor> Options = []()
			{
				TArray<FAddableComponentDescriptor> Result;

				// 리플렉션 시스템을 통해 자동으로 컴포넌트 목록 가져오기
				TArray<UClass*> ComponentClasses = UClass::GetAllComponents();

				for (UClass* Class : ComponentClasses)
				{
					if (Class && Class->bIsComponent && Class->DisplayName)
					{
						Result.push_back({
							Class->DisplayName,
							Class,
							Class->Description ? Class->Description : ""
						});
					}
				}

				return Result;
			}();
		return Options;
	}

	bool TryAttachComponentToActor(AActor& Actor, UClass* ComponentClass, UActorComponent*& SelectedComponent)
	{
		if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
			return false;
		USceneComponent* SelectedSceneComponent = static_cast<USceneComponent*>(SelectedComponent);

		UObject* RawObject = ObjectFactory::NewObject(ComponentClass);
		if (!RawObject)
		{
			return false;
		}

		UActorComponent* NewComp = Cast<UActorComponent>(RawObject);
		if (!NewComp)
		{
			ObjectFactory::DeleteObject(RawObject);
			return false;
		}

		NewComp->SetOwner(&Actor);

		// 씬 컴포넌트라면 SelectedComponent에 붙임
		if (USceneComponent* SceneComp = Cast<USceneComponent>(NewComp))
		{
			if (SelectedSceneComponent)
			{
				SceneComp->SetupAttachment(SelectedSceneComponent, EAttachmentRule::KeepRelative);
			}
			// SelectedComponent가 없으면 루트에 붙이
			else if (USceneComponent* Root = Actor.GetRootComponent())
			{
				SceneComp->SetupAttachment(Root, EAttachmentRule::KeepRelative);
			}

			Actor.RegisterComponentTree(SceneComp, GWorld);
			SelectedComponent = SceneComp;
			GWorld->GetSelectionManager()->SelectComponent(SelectedComponent);
		}

		// AddOwnedComponent 경유 (Register/Initialize 포함)
		Actor.AddOwnedComponent(NewComp);
		
		// UStaticMeshComponent라면 World Partition에 추가. (null 체크는 Register 내부에서 수행)
		if (UWorld* ActorWorld = Actor.GetWorld())
		{
			if (UWorldPartitionManager* Partition = ActorWorld->GetPartitionManager())
			{
				Partition->Register(Cast<UStaticMeshComponent>(NewComp));
			}
		}
		return true;
	}

	void MarkComponentSubtreeVisited(USceneComponent* Component, TSet<USceneComponent*>& Visited)
	{
		if (!Component || Visited.count(Component))
		{
			return;
		}

		Visited.insert(Component);
		for (USceneComponent* Child : Component->GetAttachChildren())
		{
			MarkComponentSubtreeVisited(Child, Visited);
		}
	}

	void RenderActorComponent(
		AActor* SelectedActor,
		UActorComponent* SelectedComponent,
		UActorComponent* ComponentPendingRemoval
	)
	{
		for (UActorComponent* Component : SelectedActor->GetOwnedComponents())
		{
			if (Cast<USceneComponent>(Component))
			{
				continue;
			}

			ImGuiTreeNodeFlags NodeFlags =
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_Leaf | 
				ImGuiTreeNodeFlags_NoTreePushOnOpen;

			// 선택 하이라이트: 현재 선택된 컴포넌트와 같으면 Selected 플래그
			if (Component == SelectedComponent && !GWorld->GetSelectionManager()->IsActorMode())
			{
				NodeFlags |= ImGuiTreeNodeFlags_Selected;
			}

			FString Label = Component->GetClass() ? Component->GetName() : "Unknown Component";

			ImGui::PushID(Component);
			const bool bNodeOpen = ImGui::TreeNodeEx(Component, NodeFlags, "%s", Label.c_str());
			// 좌클릭 시 컴포넌트 선택으로 전환(액터 Row 선택 해제)
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				GWorld->GetSelectionManager()->SelectComponent(Component);
			}

			if (ImGui::BeginPopupContextItem("ComponentContext"))
			{
				const bool bCanRemove = !Component->IsNative();
				if (ImGui::MenuItem("삭제", "Delete", false, bCanRemove))
				{
					ComponentPendingRemoval = Component;
				}
				ImGui::EndPopup();
			}
			
			ImGui::PopID();
		}
	}
	void RenderSceneComponentTree(
		USceneComponent* Component,
		AActor& Actor,
		UActorComponent*& SelectedComponent,
		UActorComponent*& ComponentPendingRemoval,
		TSet<USceneComponent*>& Visited)
	{
		if (!Component)
			return;

		// 에디터 전용 컴포넌트는 계층구조에 표시하지 않음
		// (CREATE_EDITOR_COMPONENT로 생성된 DirectionGizmo, SpriteComponent 등)
		if (!Component->IsEditable())
		{
			return;
		}

		Visited.insert(Component);
		const TArray<USceneComponent*>& Children = Component->GetAttachChildren();
		const bool bHasChildren = !Children.IsEmpty();

		ImGuiTreeNodeFlags NodeFlags =
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_DefaultOpen;

		if (!bHasChildren)
		{
			NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}
		// 선택 하이라이트: 현재 선택된 컴포넌트와 같으면 Selected 플래그
		if (Component == SelectedComponent && !GWorld->GetSelectionManager()->IsActorMode())
		{
			NodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		FString Label = Component->GetClass() ? Component->GetName() : "Unknown Component";
		if (Component == Actor.GetRootComponent())
		{
			Label += " (Root)";
		}

		// 트리 노드 그리기 직전에 ID 푸시
		ImGui::PushID(Component);
		const bool bNodeOpen = ImGui::TreeNodeEx(Component, NodeFlags, "%s", Label.c_str());
		// 좌클릭 시 컴포넌트 선택으로 전환(액터 Row 선택 해제)
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			GWorld->GetSelectionManager()->SelectComponent(Component);
		}

		if (ImGui::BeginPopupContextItem("ComponentContext"))
		{
			const bool bCanRemove = !Component->IsNative();
			if (ImGui::MenuItem("삭제", "Delete", false, bCanRemove))
			{
				ComponentPendingRemoval = Component;
			}
			ImGui::EndPopup();
		}

		if (bNodeOpen && bHasChildren)
		{
			for (USceneComponent* Child : Children)
			{
				RenderSceneComponentTree(Child, Actor, SelectedComponent, ComponentPendingRemoval, Visited);
			}
			ImGui::TreePop();
		}
		else if (!bNodeOpen && bHasChildren)
		{
			for (USceneComponent* Child : Children)
			{
				MarkComponentSubtreeVisited(Child, Visited);
			}
		}
		// 항목 처리가 끝나면 반드시 PopID
		ImGui::PopID();
	}
}

// 파일명 스템(Cube 등) 추출 + .obj 확장자 제거
static inline FString GetBaseNameNoExt(const FString& Path)
{
	const size_t sep = Path.find_last_of("/\\");
	const size_t start = (sep == FString::npos) ? 0 : sep + 1;

	const FString ext = ".obj";
	size_t end = Path.size();

	if (end >= ext.size())
	{
		FString PathExt = Path.substr(end - ext.size());

		std::transform(PathExt.begin(), PathExt.end(), PathExt.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (PathExt == ext)
		{
			end -= ext.size();
		}
	}

	if (start <= end)
	{
		return Path.substr(start, end - start);
	}

	return Path.substr(start);
}

UTargetActorTransformWidget::UTargetActorTransformWidget()
	: UWidget("Target Actor Transform Widget")
	, UIManager(&UUIManager::GetInstance())
{

}

UTargetActorTransformWidget::~UTargetActorTransformWidget() = default;

void UTargetActorTransformWidget::OnSelectedActorCleared()
{
	ResetChangeFlags();
}

void UTargetActorTransformWidget::Initialize()
{
	// UIManager 참조 확보
	UIManager = &UUIManager::GetInstance();

	// Transform 위젯을 UIManager에 등록하여 선택 해제 브로드캐스트를 받을 수 있게 함
	if (UIManager)
	{
		UIManager->RegisterTargetTransformWidget(this);
	}
}


void UTargetActorTransformWidget::Update()
{
	USceneComponent* SelectedComponent = GWorld->GetSelectionManager()->GetSelectedComponent();
	if (SelectedComponent)
	{
		if (!bRotationEditing)
		{
			UpdateTransformFromComponent(SelectedComponent);
		}
	}
}

void UTargetActorTransformWidget::RenderWidget()
{
	AActor* SelectedActor = GWorld->GetSelectionManager()->GetSelectedActor();
	UActorComponent* SelectedComponent = GWorld->GetSelectionManager()->GetSelectedActorComponent();
	if (!SelectedActor)
	{
		return;
	}

	// 1. 헤더 (액터 이름, "+추가" 버튼) 렌더링
	RenderHeader(SelectedActor, SelectedComponent);

	// 2. 컴포넌트 계층 구조 렌더링
	RenderComponentHierarchy(SelectedActor, SelectedComponent);
	// 위 함수에서 SelectedComponent를 Delete하는데 아래 함수에서 SelectedComponent를 그대로 인자로 사용하고 있었음.
	// 댕글링 포인터 참조를 막기 위해 다시 한번 SelectionManager에서 Component를 얻어옴
	// 기존에는 DestroyComponent에서 DeleteObject를 호출하지도 않았음. Delete를 실제로 진행하면서 발견된 버그.
	
	// 3. 선택된 컴포넌트, 엑터의 상세 정보 렌더링 (Transform 포함)
	if (GWorld->GetSelectionManager()->IsActorMode())
	{
		RenderSelectedActorDetails(GWorld->GetSelectionManager()->GetSelectedActor());
	}
	else
	{
		RenderSelectedComponentDetails(GWorld->GetSelectionManager()->GetSelectedActorComponent());
	}
}

void UTargetActorTransformWidget::RenderHeader(AActor* SelectedActor, UActorComponent* SelectedComponent)
{
	ImGui::Text(SelectedActor->GetName().ToString().c_str());
	ImGui::SameLine();

	const float ButtonWidth = 60.0f;
	const float ButtonHeight = 25.0f;
	float Avail = ImGui::GetContentRegionAvail().x;
	float NewX = ImGui::GetCursorPosX() + std::max(0.0f, Avail - ButtonWidth);
	ImGui::SetCursorPosX(NewX);

	if (ImGui::Button("+ 추가", ImVec2(ButtonWidth, ButtonHeight)))
	{
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		ImGui::TextUnformatted("Add Component");
		ImGui::Separator();

		for (const FAddableComponentDescriptor& Descriptor : GetAddableComponentDescriptors())
		{
			ImGui::PushID(Descriptor.Label);
			if (ImGui::Selectable(Descriptor.Label))
			{
				if (TryAttachComponentToActor(*SelectedActor, Descriptor.Class, SelectedComponent))
				{
					ImGui::CloseCurrentPopup();
				}
			}
			if (Descriptor.Description && ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", Descriptor.Description);
			}
			ImGui::PopID();
		}
		ImGui::EndPopup();
	}
	ImGui::Spacing();
}

void UTargetActorTransformWidget::RenderComponentHierarchy(AActor* SelectedActor, UActorComponent* SelectedComponent)
{
	AActor* ActorPendingRemoval = nullptr;
	UActorComponent* ComponentPendingRemoval = nullptr;

	// 컴포넌트 트리 박스 크기 관련
	static float PaneHeight = 120.0f;
	const float SplitterThickness = 4.0f;
	const float MinTop = 1.0f;
	const float MinBottom = 0.0f;
	const float WindowAvailY = ImGui::GetContentRegionAvail().y;
	PaneHeight = std::clamp(PaneHeight, MinTop, std::max(MinTop, WindowAvailY - (MinBottom + SplitterThickness)));

	ImGui::BeginChild("ComponentBox", ImVec2(0, PaneHeight), true);

	// 액터 행 렌더링
	ImGui::PushID("ActorDisplay");
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));

	const bool bActorSelected = GWorld->GetSelectionManager()->IsActorMode();
	if (ImGui::Selectable(SelectedActor->GetName().ToString().c_str(), bActorSelected, ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_SpanAvailWidth))
	{
		GWorld->GetSelectionManager()->SelectActor(SelectedActor);
	}
	ImGui::PopStyleColor();
	if (ImGui::BeginPopupContextItem("ActorContextMenu"))
	{
		if (ImGui::MenuItem("삭제", "Delete", false, true))
		{
			ActorPendingRemoval = SelectedActor;
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();

	// 컴포넌트 트리 렌더링
	USceneComponent* RootComponent = SelectedActor->GetRootComponent();
	if (!RootComponent)
	{
		ImGui::BulletText("Root component not found.");
	}
	else
	{
		TSet<USceneComponent*> VisitedComponents;
		RenderSceneComponentTree(RootComponent, *SelectedActor, SelectedComponent, ComponentPendingRemoval, VisitedComponents);
		// ... (루트에 붙지 않은 씬 컴포넌트 렌더링 로직은 생략 가능성 있음, 엔진 설계에 따라)
		ImGui::Separator();
		RenderActorComponent(SelectedActor, SelectedComponent, ComponentPendingRemoval);
	}

	// 삭제 입력 처리
	const bool bDeletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete);
	if (bDeletePressed)
	{
		if (bActorSelected) ActorPendingRemoval = SelectedActor;
		else if (SelectedComponent && !SelectedComponent->IsNative()) ComponentPendingRemoval = SelectedComponent;
	}

	// 컴포넌트 삭제 실행
	if (ComponentPendingRemoval)
	{

		USceneComponent* NewSelection = SelectedActor->GetRootComponent();
		// SelectionManager를 통해 선택 해제
		GWorld->GetSelectionManager()->ClearSelection();

		// 컴포넌트 삭제
		SelectedActor->RemoveOwnedComponent(ComponentPendingRemoval);
		ComponentPendingRemoval = nullptr;

		// 삭제 후 새로운 컴포넌트 선택
		if (NewSelection)
		{
			GWorld->GetSelectionManager()->SelectComponent(NewSelection);
		}
	}

	// 액터 삭제 실행
	if (ActorPendingRemoval)
	{
		// 삭제 전에 선택 해제 (dangling pointer 방지)
		GWorld->GetSelectionManager()->ClearSelection();

		// 액터 삭제
		if (UWorld* World = ActorPendingRemoval->GetWorld()) World->DestroyActor(ActorPendingRemoval);
		else ActorPendingRemoval->Destroy();

		OnSelectedActorCleared();
	}

	ImGui::EndChild();

	// 스플리터 렌더링
	ImGui::InvisibleButton("VerticalSplitter", ImVec2(-FLT_MIN, SplitterThickness));
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	if (ImGui::IsItemActive()) PaneHeight += ImGui::GetIO().MouseDelta.y;
	ImVec2 Min = ImGui::GetItemRectMin(), Max = ImGui::GetItemRectMax();
	ImGui::GetWindowDrawList()->AddLine(ImVec2(Min.x, (Min.y + Max.y) * 0.5f), ImVec2(Max.x, (Min.y + Max.y) * 0.5f), ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
	ImGui::Spacing();
}

void UTargetActorTransformWidget::RenderSelectedActorDetails(AActor* SelectedActor)
{
	if (!SelectedActor)
	{
		return;
	}
	USceneComponent* RootComponent = SelectedActor->GetRootComponent();
	const TArray<FProperty>& Properties = USceneComponent::StaticClass()->GetProperties();
	

	UPropertyRenderer::RenderProperties(Properties, RootComponent);

	bool bActorHiddenInGame = SelectedActor->GetActorHiddenInGame();
	if (ImGui::Checkbox("bActorHiddendInGame", &bActorHiddenInGame))
	{
		SelectedActor->SetActorHiddenInGame(bActorHiddenInGame);
	}

	// ======== Lua Script UI ======== 
	ImGui::Spacing();
	ImGui::Separator();
	
	// Actor의 모든 ScriptComponent 찾기
	TArray<UScriptComponent*> ScriptComponents;
	for (UActorComponent* Comp : SelectedActor->GetOwnedComponents())
	{
		if (UScriptComponent* Script = Cast<UScriptComponent>(Comp))
		{
			ScriptComponents.push_back(Script);
		}
	}
	
	if (!ScriptComponents.IsEmpty())
	{
		ImGui::Text("[Lua Scripts]");
		ImGui::Spacing();
	
		int ScriptIndex = 0;
		for (UScriptComponent* ScriptComp : ScriptComponents)
		{
			ImGui::PushID(ScriptComp); // 각 컴포넌트 UI에 고유 ID 부여
	
			bool bFileExists = !ScriptComp->ScriptFilePath.empty() && std::filesystem::exists(ScriptComp->ScriptFilePath.c_str());
	
			ImGui::Text("Script File:");
			ImGui::SameLine();
	
			FString DisplayPath;
			if (bFileExists)
			{
				DisplayPath = ScriptComp->ScriptFilePath;
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", DisplayPath.c_str());
			}
			else
			{
				// 파일이 없으면 제안 경로를 생성
				FString SceneName = SelectedActor->GetWorld() ? SelectedActor->GetWorld()->GetName() : "Scene";
				FString ActorName = SelectedActor->GetName().ToString();
				DisplayPath = "Scripts/" + SceneName + "_" + ActorName + "_" + std::to_string(ScriptIndex) + ".lua";
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s (proposed)", DisplayPath.c_str());
			}
	
			ImGui::Spacing();
	
			if (bFileExists)
			{
				// Edit Script 버튼 (스크립트가 존재할 때만 표시)
				if (ImGui::Button("Edit Script", ImVec2(120, 0)))
				{
					ScriptComp->EditScript();
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Open the script file in your default text editor");
				}
	
				ImGui::SameLine();
	
				// Reload Script 버튼 (Hot Reload)
				if (ImGui::Button("Reload Script", ImVec2(120, 0)))
				{
					ScriptComp->ReloadScript();
					UE_LOG("[UI] Script reloaded: %s", ScriptComp->ScriptFilePath.c_str());
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Hot reload the script without restarting the game");
				}
			}
			else
			{
				// Create Script 버튼
				if (ImGui::Button("Create Script", ImVec2(150, 0)))
				{
					// 제안된 경로로 스크립트 생성
					ScriptComp->CreateScript(DisplayPath);
					UE_LOG("[UI] Script created: %s", ScriptComp->ScriptFilePath.c_str());
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Create a new Lua script from template.lua");
				}
			}
	
			ImGui::Separator();
			ImGui::PopID();
			ScriptIndex++;
		}
	}
}

void UTargetActorTransformWidget::RenderSelectedComponentDetails(UActorComponent* SelectedComponent)
{

	ImGui::Spacing();
	ImGui::Separator();

	if (!SelectedComponent) return;

	// 리플렉션이 적용된 컴포넌트는 자동으로 UI 생성
	if (SelectedComponent)
	{
		ImGui::Separator();
		ImGui::Text("[Reflected Properties]");
		UPropertyRenderer::RenderAllPropertiesWithInheritance(SelectedComponent);
	}

	if(ULightComponent* LightComp = Cast<ULightComponent>(SelectedComponent))
	{
		if(ImGui::Button("Reset Shadow Values To Default Values"))
		{
			LightComp->SetShadowValuesToDefault();
		}
	}

	if (Cast<UDirectionalLightComponent>(SelectedComponent))
	{
		if (GWorld->GetRenderSettings().GetDirectionaliShadowMode() == EDirectionalShadowMode::CSM)
		{
			FCSM* CsmSystem = URenderManager::GetInstance().GetRenderer()->GetCSMSystem();

			bool bBlend = CsmSystem->GetBlending();
			if (ImGui::Checkbox(" Blend", &bBlend)) { CsmSystem->SetBlending(bBlend); }
			if (ImGui::IsItemHovered())  ImGui::SetTooltip("경계 적용 여부");

			bool bDebugging = CsmSystem->GetDebugging();
			if (ImGui::Checkbox(" Debug", &bDebugging)) { CsmSystem->SetDebugging(bDebugging); }
			if (ImGui::IsItemHovered())  ImGui::SetTooltip("디버깅 적용 여부");

			/*bool bAABBScene = CsmSystem->GetSceneAABB();
			if (ImGui::Checkbox(" SceneAABB", &bAABBScene)) { CsmSystem->SetSceneAABB(bAABBScene); }
			if (ImGui::IsItemHovered())  ImGui::SetTooltip("Cutoff 보완 여부");*/
			
			float bLambda = CsmSystem->GetLambda();
			if (ImGui::SliderFloat("Lambda", &bLambda, 0.0f, 1.0f)) 
			{ 
				CsmSystem->SetLambda(bLambda); 
			}
			if (ImGui::IsItemHovered())  ImGui::SetTooltip("클수록 근거리 그림자 선명해짐");

			int bCascadeNum = CsmSystem->GetCascadeNum();
			if (ImGui::SliderInt("Cascade Number", &bCascadeNum, 1.0f, 8.0f)) 
			{ 
				CsmSystem->SetCascadeNum(bCascadeNum); 
			}
			if (ImGui::IsItemHovered())  ImGui::SetTooltip("Sub Frustum 개수");
			
			float bPadding = CsmSystem->GetPaddingRadius();
			if (ImGui::SliderFloat("Padding", &bPadding, 0.0f, 100.0f))
			{
				CsmSystem->SetPaddingRadius(bPadding);
			}
			if (ImGui::IsItemHovered())  ImGui::SetTooltip("AABB Padding");
			
			TArray<ID3D11ShaderResourceView*> CSMDebugSRV = CsmSystem->GetDebugSRV();
			const ImVec2 Size(256, 256);
			for (int i=0; i<CSMDebugSRV.Num(); i++)
			{
				ImGui::Image((ImTextureID)CSMDebugSRV[i], Size);
				if (i%2==0) ImGui::SameLine();
			}

			
		}
		else
		{
			ImGui::Image(URenderManager::GetInstance().GetRenderer()->GetShadowSystem()->GetDirectionalShadowMapSRV(), ImVec2(512, 512));	
		}
	}

	ImGui::Separator();

	// Shadow map을 UI에 렌더링
	auto* ShadowSystem = URenderManager::GetInstance().GetRenderer()->GetShadowSystem();

	if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(SelectedComponent))
	{
		ImGui::Text("[Spot Light Shadow Maps]");

		// Spot Light의 Shadow Index 가져오기
		FSpotLightInfo LightInfo = SpotLightComp->GetLightInfo();
		int32 ShadowIndex = LightInfo.ShadowIndex;

		if (ShadowIndex >= 0)
		{
			// 해당 SpotLight의 Shadow Map SRV 가져오기
			ID3D11ShaderResourceView* SpotShadowSRV = ShadowSystem->GetSpotShadowMapSnapShotSRV();
			if (SpotShadowSRV)
			{
				ImGui::Image(SpotShadowSRV, ImVec2(512, 512));
			}
			else
			{
				ImGui::Text("No shadow map available for this light (ShadowIndex: %d)", ShadowIndex);
			}
		}
		else
		{
			ImGui::Text("No shadow cast by this light (ShadowIndex: %d)", ShadowIndex);
		}
	}
	else if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(SelectedComponent))
	{
		ImGui::Text("[Point Light Shadow Maps]");

		// Point Light의 Shadow Index 가져오기
		FPointLightInfo LightInfo = PointLightComp->GetLightInfo();
		int32 ShadowIndex = LightInfo.ShadowIndex;

		if (ShadowIndex >= 0)
		{
			// 큐브맵의 6개 면에 대한 SRV 가져오기
			const char* FaceNames[6] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };

			const float ImageSize = 200.0f;
			const float Spacing = 15.0f;

			for (int FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
			{
				// 짝수 인덱스일 때만 FaceName 출력 (한 줄에 2개씩)
				if (FaceIndex % 2 == 0)
				{
					ImGui::Text("%s", FaceNames[FaceIndex]);
					ImGui::SameLine();
					ImGui::SetCursorPosX(ImageSize + Spacing);
					ImGui::Text("%s", FaceNames[FaceIndex + 1]);
				}

				// FSceneRenderer::RenderShadowMap에서, 선택된 Pointlight에 해당하는 cubemap을 스냅샷으로 캡처하는데, 그렇게 캡처된 스냅샷을 SRV로 가져옴
				ID3D11ShaderResourceView* FaceSRV = ShadowSystem->GetPointShadowCubeMapSnapShotSRV(FaceIndex);

				if (FaceSRV)
				{
					ImGui::Image(FaceSRV, ImVec2(ImageSize, ImageSize));
				}
				else
				{
					ImGui::Text("%s - No Shadow", FaceNames[FaceIndex]);
				}

				// 짝수 인덱스(0, 2, 4)이고 마지막 요소가 아니면 SameLine 호출
				if (FaceIndex % 2 == 0 && FaceIndex < 5)
				{
					ImGui::SameLine();
				}
			}
		}
		else
		{
			ImGui::Text("No shadow cast by this light (ShadowIndex: %d)", ShadowIndex);
		}
	}

}

void UTargetActorTransformWidget::UpdateTransformFromComponent(USceneComponent* SelectedComponent)
{
	if (SelectedComponent)
	{
		EditLocation = SelectedComponent->GetRelativeLocation();
		EditRotation = SelectedComponent->GetRelativeRotation().ToEulerZYXDeg();
		EditScale = SelectedComponent->GetRelativeScale();
	}

	ResetChangeFlags();
}

void UTargetActorTransformWidget::ResetChangeFlags()
{
	bPositionChanged = false;
	bRotationChanged = false;
	bScaleChanged = false;
}
