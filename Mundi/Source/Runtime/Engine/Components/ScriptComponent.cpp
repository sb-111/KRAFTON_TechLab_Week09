#include "pch.h"
#include "ScriptComponent.h"
#include "Actor.h"
#include "World.h"
#include <filesystem>
#include "tchar.h"

IMPLEMENT_CLASS(UScriptComponent)

BEGIN_PROPERTIES(UScriptComponent)
	MARK_AS_COMPONENT("스크립트 컴포넌트", "액터에 Lua 스크립트를 연결하여 실행합니다.")
	ADD_PROPERTY(FString, ScriptFilePath, "Script", false, "Path to the Lua script file")
END_PROPERTIES()



UScriptComponent::UScriptComponent()
{
	bCanEverTick = true;
	bTickEnabled = true;
}

void UScriptComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ScriptFilePath.empty())
	{
		LoadScript(ScriptFilePath);
	}

	// Lua BeginPlay 호출
	CallLuaFunction("BeginPlay");
}

void UScriptComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	// Lua Tick 호출
	CallLuaFunction("Tick", DeltaTime);
}

void UScriptComponent::EndPlay(EEndPlayReason Reason)
{
	// Lua EndPlay 호출
	CallLuaFunction("EndPlay");

	CleanupEnvironment();

	Super::EndPlay(Reason);
}

void UScriptComponent::OnSerialized()
{
	Super::OnSerialized();

	// 씬 로드 후 스크립트 파일 경로가 복원되었으면 스크립트 로드
	if (!ScriptFilePath.empty() && !bScriptLoaded)
	{
		LoadScript(ScriptFilePath);
		UE_LOG("[ScriptComponent] Script loaded from serialization: %s", ScriptFilePath.c_str());
	}
}

void UScriptComponent::InitializeEnvironment()
{
	if (bEnvironmentInitialized) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	UWorld* World = Owner->GetWorld();
	if (!World) return;

	// World의 Lua State를 기반으로 격리된 환경 생성
	sol::state& LuaState = World->GetLuaState();
	Env = sol::environment(LuaState, sol::create, LuaState.globals());

	// 핵심: 각 스크립트마다 독립적인 'obj' 변수
	// 이 스크립트의 obj는 이 컴포넌트의 주인(Owner) Actor를 가리킴
	Env["obj"] = Owner;

	// 전역 print 함수를 UE_LOG로 리다이렉트
	Env["print"] = [&LuaState](sol::variadic_args va) {
		std::string output;
		for (auto v : va)
		{
			output += LuaState.globals()["tostring"](v).get<std::string>() + "\t";
		}
		UE_LOG("%s", output.c_str());
	};

	bEnvironmentInitialized = true;
}

void UScriptComponent::CleanupEnvironment()
{
	Env = sol::environment();
	bScriptLoaded = false;
	bEnvironmentInitialized = false;
}

void UScriptComponent::LoadScript(const FString& FilePath)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG("[ScriptComponent] Error: No owner actor");
		return;
	}

	UWorld* World = Owner->GetWorld();
	if (!World)
	{
		UE_LOG("[ScriptComponent] Error: No world");
		return;
	}

	// 환경 초기화
	InitializeEnvironment();

	// 스크립트 파일 로드
	sol::state& LuaState = World->GetLuaState();
	auto result = LuaState.safe_script_file(FilePath, Env);

	if (!result.valid())
	{
		sol::error Err = result;
		UE_LOG("[Lua Script Load Error] %s: %s", FilePath.c_str(), Err.what());
		bScriptLoaded = false;
		return;
	}

	ScriptFilePath = FilePath;
	bScriptLoaded = true;
	UE_LOG("[Lua Script Loaded] %s", FilePath.c_str());
}

void UScriptComponent::ReloadScript()
{
	if (ScriptFilePath.empty()) return;

	UE_LOG("[Lua Script Reloading] %s", ScriptFilePath.c_str());

	// 기존 환경 정리 및 재초기화
	CleanupEnvironment();
	LoadScript(ScriptFilePath);

	// BeginPlay 재호출 (Hot Reload 시)
	if (HasBegunPlay())
	{
		CallLuaFunction("BeginPlay");
	}
}

void UScriptComponent::CreateScript(const FString& FilePath)
{
	if (FilePath.empty())
	{
		UE_LOG("[ScriptComponent] CreateScript failed: FilePath is empty.");
		return;
	}

	// Scripts 디렉터리 생성 (없으면)
	std::filesystem::create_directories("Scripts");

	ScriptFilePath = FilePath;

	// template.lua 복사
	std::filesystem::path TemplatePath = "Scripts/template.lua";
	std::filesystem::path NewScriptPath = ScriptFilePath.c_str();

	if (std::filesystem::exists(TemplatePath))
	{
		std::filesystem::copy_file(TemplatePath, NewScriptPath, std::filesystem::copy_options::overwrite_existing);
		UE_LOG("[ScriptComponent] Script created: %s", ScriptFilePath.c_str());
	}
	else
	{
		UE_LOG("[ScriptComponent] Warning: template.lua not found, creating empty script");
		// template.lua가 없으면 기본 스크립트 생성
		std::ofstream ofs(ScriptFilePath);
		ofs << "-- Auto-generated script for " << GetOwner()->GetName().ToString() << "\n\n";
		ofs << "function BeginPlay()\n";
		ofs << "    print(\"[BeginPlay] \" .. tostring(obj))\n";
		ofs << "end\n\n";
		ofs << "function Tick(dt)\n";
		ofs << "    -- Update logic here\n";
		ofs << "end\n\n";
		ofs << "function EndPlay()\n";
		ofs << "    print(\"[EndPlay] \" .. tostring(obj))\n";
		ofs << "end\n";
		ofs.close();
	}

	// 생성 후 바로 로드
	LoadScript(ScriptFilePath);
}

void UScriptComponent::EditScript()
{
	if (ScriptFilePath.empty())
	{
		UE_LOG("[ScriptComponent] No script to edit");
		return;
	}

	// Windows 기본 프로그램으로 스크립트 파일 열기
#ifdef _WIN32
	// ShellExecute는 상대 경로보다 절대 경로에서 더 안정적으로 동작합니다.
	std::filesystem::path absolutePath = std::filesystem::absolute(ScriptFilePath.c_str());

	HINSTANCE hInst = ShellExecute(
		NULL,
		_T("open"),
		absolutePath.c_str(), // 절대 경로 사용
		NULL,
		NULL,
		SW_SHOWNORMAL
	);

	// ShellExecute는 성공 시 32보다 큰 값을 반환합니다.
	// 실패 시 32 이하의 값이 반환되므로 간단히 체크 가능
	if ((INT_PTR)hInst <= 32)
	{
		MessageBox(NULL, _T("파일 열기에 실패했습니다."), _T("Error"), MB_OK | MB_ICONERROR);
	}
#else
	UE_LOG("[ScriptComponent] EditScript is only supported on Windows");
#endif
}
