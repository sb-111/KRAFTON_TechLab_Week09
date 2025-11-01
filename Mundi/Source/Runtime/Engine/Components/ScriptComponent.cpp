#include "pch.h"
#include "ScriptComponent.h"
#include "Actor.h"
#include "World.h"
#include <filesystem>
#include <fstream>
#include "tchar.h"

// UTF-8 string을 Wide string으로 변환 (Windows 한글 경로 지원)
static std::wstring Utf8ToWide(const std::string& utf8str)
{
	if (utf8str.empty()) return std::wstring();

	int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), (int)utf8str.size(), NULL, 0);
	if (size_needed <= 0) return std::wstring();

	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), (int)utf8str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

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

	UE_LOG("[ScriptComponent] BeginPlay called for %s", GetOwner() ? GetOwner()->GetName().ToString().c_str() : "Unknown");

	if (!ScriptFilePath.empty())
	{
		UE_LOG("[ScriptComponent] Loading script from BeginPlay: %s", ScriptFilePath.c_str());
		LoadScript(ScriptFilePath);
	}
	else
	{
		UE_LOG("[ScriptComponent] No script file path set");
	}

	// Lua BeginPlay 호출
	UE_LOG("[ScriptComponent] Calling Lua BeginPlay...");
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

void UScriptComponent::PostDuplicate()
{
	Super::PostDuplicate();

	// PIE 복제 시 EditorWorld의 Lua 상태가 복사되므로 초기화
	// 이렇게 하면 PIEWorld의 새 LuaState를 사용하게 됨
	CleanupEnvironment();

	UE_LOG("[ScriptComponent] PostDuplicate: Lua environment reset for PIE");

	// 스크립트 파일이 있으면 PIEWorld에서 다시 로드
	// (BeginPlay에서 자동으로 로드되므로 여기서는 초기화만 수행)
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
	// Owner를 캡처하여 실행 시점에 LuaState를 얻어옴 (댕글링 참조 방지)
	Env["print"] = [Owner](sol::variadic_args va) {
		if (!Owner)
		{
			UE_LOG("[Lua Print] Error: Owner is null");
			return;
		}

		UWorld* World = Owner->GetWorld();
		if (!World)
		{
			UE_LOG("[Lua Print] Error: World is null");
			return;
		}

		sol::state& LuaState = World->GetLuaState();
		std::string output;
		int i = 1;
		for (auto v : va)
		{
			sol::protected_function_result tostring_result = LuaState.globals()["tostring"](v);
			if (tostring_result.valid() && tostring_result.get_type() == sol::type::string)
			{
				output += tostring_result.get<std::string>() + "\t";
			}
			else
			{
				std::string type_name = sol::type_name(LuaState, v.get_type());
				if (!tostring_result.valid())
				{
					sol::error err = tostring_result;
					UE_LOG("[Lua Print Error] 'tostring' call failed for arg %d (%s). Reason: %s", i, type_name.c_str(), err.what());
				}
				else
				{
					UE_LOG("[Lua Print Error] Failed to convert arg %d to string. Type was '%s' but tostring returned %s.", i, type_name.c_str(), sol::type_name(LuaState, tostring_result.get_type()));
				}
				output += "[CONV_ERROR] ";
			}
			i++;
		}

		// 마지막 탭 제거
		if (!output.empty() && output.back() == '\t') {
			output.pop_back();
		}

		UE_LOG("[Lua Print] %s", output.c_str());
	};

	bEnvironmentInitialized = true;
	UE_LOG("[ScriptComponent] Environment initialized for %s", Owner->GetName().ToString().c_str());
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

	// 스크립트 파일 로드 (한글 경로 지원을 위해 직접 읽기)
	sol::state& LuaState = World->GetLuaState();

	// Wide string으로 파일 열기
	std::wstring wFilePath = Utf8ToWide(FilePath);
	std::ifstream file(wFilePath, std::ios::binary);

	if (!file.is_open())
	{
		UE_LOG("[Lua Script Load Error] Cannot open file: %s", FilePath.c_str());
		bScriptLoaded = false;
		return;
	}

	// 파일 내용을 문자열로 읽기
	std::string script_content((std::istreambuf_iterator<char>(file)),
	                            std::istreambuf_iterator<char>());
	file.close();

	// UTF-8 BOM 제거 (EF BB BF)
	if (script_content.size() >= 3 &&
	    static_cast<unsigned char>(script_content[0]) == 0xEF &&
	    static_cast<unsigned char>(script_content[1]) == 0xBB &&
	    static_cast<unsigned char>(script_content[2]) == 0xBF)
	{
		script_content.erase(0, 3);
		UE_LOG("[ScriptComponent] UTF-8 BOM removed from script");
	}

	// 디버깅: 스크립트 첫 100자 출력
	UE_LOG("[ScriptComponent] Executing script, size: %d bytes", (int)script_content.size());
	if (script_content.size() > 0)
	{
		std::string preview = script_content.substr(0, std::min<size_t>(100, script_content.size()));
		UE_LOG("[ScriptComponent] Script preview: %s...", preview.c_str());
	}

	// 스크립트 실행
	auto result = LuaState.safe_script(script_content, Env, FilePath);

	if (!result.valid())
	{
		sol::error Err = result;
		UE_LOG("[Lua Script Load Error] %s: %s", FilePath.c_str(), Err.what());
		bScriptLoaded = false;
		return;
	}

	ScriptFilePath = FilePath;
	bScriptLoaded = true;
	UE_LOG("[Lua Script Loaded] %s (bScriptLoaded=true)", FilePath.c_str());

	// 환경에 BeginPlay 함수가 있는지 확인
	sol::object beginPlayObj = Env["BeginPlay"];
	if (beginPlayObj.valid() && beginPlayObj.is<sol::function>())
	{
		UE_LOG("[ScriptComponent] BeginPlay function found in script");
	}
	else
	{
		UE_LOG("[ScriptComponent] WARNING: BeginPlay function NOT found in script!");
	}
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
	std::filesystem::create_directories(L"Scripts");

	ScriptFilePath = FilePath;

	// template.lua 복사 (한글 경로 지원을 위해 wstring 사용)
	std::filesystem::path TemplatePath = L"Scripts/template.lua";
	std::filesystem::path NewScriptPath = Utf8ToWide(ScriptFilePath);

	if (std::filesystem::exists(TemplatePath))
	{
		std::filesystem::copy_file(TemplatePath, NewScriptPath, std::filesystem::copy_options::overwrite_existing);
		UE_LOG("[ScriptComponent] Script created: %s", ScriptFilePath.c_str());
	}
	else
	{
		UE_LOG("[ScriptComponent] Warning: template.lua not found, creating empty script");
		// template.lua가 없으면 기본 스크립트 생성 (한글 경로 지원)
		std::wstring wScriptPath = Utf8ToWide(ScriptFilePath);
		std::ofstream ofs(wScriptPath);
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

	// C++의 BeginPlay가 이미 호출된 후 스크립트를 생성했다면, Lua의 BeginPlay를 명시적으로 호출
	if (HasBegunPlay())
	{
		CallLuaFunction("BeginPlay");
	}
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
	std::filesystem::path absolutePath = std::filesystem::absolute(Utf8ToWide(ScriptFilePath));

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

void UScriptComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// ScriptComponent는 별도의 서브 오브젝트가 없음
	// ScriptFilePath는 자동으로 복사되며, Lua 환경은 PostDuplicate()에서 초기화됨
}

