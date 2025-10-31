#pragma once
#include "ActorComponent.h"
#include "sol/sol.hpp"
#include <string>

class AActor;

/**
 * UScriptComponent
 * Lua 스크립트를 실행하고 관리하는 컴포넌트
 * 각 인스턴스는 독립적인 Lua 환경(sol::environment)을 가져 스크립트 간 격리를 보장합니다.
 */
class UScriptComponent : public UActorComponent
{
public:
	DECLARE_CLASS(UScriptComponent, UActorComponent)

	UScriptComponent();
	virtual ~UScriptComponent() = default;

	// 생명주기 (UActorComponent 오버라이드)
	void BeginPlay() override;
	void TickComponent(float DeltaTime) override;
	void EndPlay(EEndPlayReason Reason) override;

	// ====== 스크립트 파일 관리 ==========
	void CreateScript();
	void EditScript();
	void LoadScript(const FString& FilePath);
	void ReloadScript(); // Hot Reload 지원

	// Lua 함수 호출 헬퍼
	template<typename... Args>
	void CallLuaFunction(const char* FuncName, Args... args)
	{
		if (!bScriptLoaded) return;

		sol::protected_function LuaFunc = Env[FuncName];
		if (LuaFunc.valid())
		{
			auto Result = LuaFunc(args...);
			if (!Result.valid())
			{
				sol::error Err = Result;
				ULog("[Lua Error] %s: %s", FuncName, Err.what());
			}
		}
	}

	// 스크립트 파일 경로
	FString ScriptFilePath;

private:
	void InitializeEnvironment();
	void CleanupEnvironment();

private:
	sol::environment Env;       // 스크립트 격리용 환경
	bool bScriptLoaded = false; // 스크립트 로드 여부
	bool bEnvironmentInitialized = false;
};
