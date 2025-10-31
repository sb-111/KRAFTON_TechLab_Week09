#pragma once
#include "ActorComponent.h"
#include "sol/sol.hpp"
#include <string>

class AActor;

/**
 * @class UScriptComponent
 * @brief Lua 스크립트의 로드, 실행, 관리를 담당하는 액터 컴포넌트입니다.
 * @details 각 UScriptComponent 인스턴스는 독립적인 Lua 환경(sol::environment)을 가집니다.
 * 이는 여러 스크립트가 서로의 상태에 영향을 주지 않고 안전하게 실행되도록 보장합니다.
 * 액터의 생명주기(BeginPlay, Tick, EndPlay)와 연결되어 Lua 스크립트 내의 해당 함수들을 자동으로 호출합니다.
 * 스크립트의 동적 생성, 외부 편집기 연동, Hot Reload 기능을 지원합니다.
 */
class UScriptComponent : public UActorComponent
{
public:
	DECLARE_CLASS(UScriptComponent, UActorComponent)
	GENERATED_REFLECTION_BODY()
	/**
	 * @brief UScriptComponent의 기본 생성자입니다.
	 */
	UScriptComponent();
	virtual ~UScriptComponent() = default;

	// 생명주기 (UActorComponent 오버라이드)
	/**
	 * @brief 게임이 시작될 때 호출되는 컴포넌트의 생명주기 함수입니다.
	 * @details 스크립트 파일을 로드하고, 스크립트 내의 'BeginPlay' 함수를 호출합니다.
	 */
	void BeginPlay() override;

	/**
	 * @brief 매 프레임 호출되는 컴포넌트의 생명주기 함수입니다.
	 * @details 스크립트 내의 'Tick' 함수를 호출하며, 경과 시간(DeltaTime)을 인자로 전달합니다.
	 * @param DeltaTime 이전 프레임으로부터 경과한 시간 (초 단위)
	 */
	void TickComponent(float DeltaTime) override;

	/**
	 * @brief 게임이 종료될 때 호출되는 컴포넌트의 생명주기 함수입니다.
	 * @details 스크립트 내의 'EndPlay' 함수를 호출하고, 사용된 Lua 환경을 정리합니다.
	 * @param Reason 게임이 종료된 이유
	 */
	void EndPlay(EEndPlayReason Reason) override;

	// 스크립트 파일 관리
	/**
	 * @brief 새로운 Lua 스크립트 파일을 생성합니다.
	 * @details 'Scripts/template.lua' 파일을 복사하여 지정된 경로에 새 파일을 만듭니다.
	 * @param FilePath 생성할 스크립트의 전체 파일 경로.
	 */
	void CreateScript(const FString& FilePath);

	/**
	 * @brief 현재 스크립트 파일을 외부 편집기에서 엽니다.
	 * @details Windows의 경우, '.lua' 확장자에 연결된 기본 프로그램을 실행하여 스크립트를 엽니다.
	 * @warning 이 기능은 Windows 플랫폼에서만 지원됩니다.
	 */
	void EditScript();

	/**
	 * @brief 지정된 경로의 Lua 스크립트 파일을 로드합니다.
	 * @details 스크립트를 로드하기 전에 Lua 환경을 초기화하고, 파일을 실행하여 스크립트의 함수와 변수를 환경에 등록합니다.
	 * @param FilePath 로드할 스크립트의 파일 경로
	 */
	void LoadScript(const FString& FilePath);

	/**
	 * @brief 스크립트가 로드되었는지 확인합니다.
	 * @return 스크립트 파일이 설정되어 있으면 true
	 */
	bool HasScriptFile() const { return !ScriptFilePath.empty(); }

	/**
	 * @brief 현재 스크립트를 다시 로드합니다 (Hot Reload).
	 * @details 기존 Lua 환경을 초기화하고 스크립트 파일을 다시 읽어옵니다.
	 * 게임이 실행 중이라면 'BeginPlay' 함수를 다시 호출하여 변경 사항을 즉시 적용합니다.
	 */
	void ReloadScript();

protected:
	/**
	 * @brief 직렬화가 완료된 후 호출됩니다.
	 * @details 씬 로드 시 ScriptFilePath가 복원된 후 스크립트를 자동으로 로드합니다.
	 */
	void OnSerialized() override;

public:
	// Lua 함수 호출 헬퍼
	/**
	 * @brief 스크립트 내에 정의된 Lua 함수를 안전하게 호출합니다.
	 * @details 가변 인자 템플릿을 사용하여 모든 타입의 인자를 Lua 함수로 전달할 수 있습니다.
	 * 함수 실행 중 발생하는 오류를 감지하고 로그를 남깁니다.
	 * @tparam Args 함수에 전달할 인자들의 타입
	 * @param FuncName 호출할 Lua 함수의 이름
	 * @param args 함수에 전달할 인자들
	 */
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
				UE_LOG("[Lua Error] %s: %s", FuncName, Err.what());
			}
		}
	}

	/// @brief 현재 컴포넌트에 연결된 Lua 스크립트 파일의 경로입니다.
	FString ScriptFilePath;

private:
	/**
	 * @brief 스크립트 실행을 위한 독립적인 Lua 환경을 초기화합니다.
	 * @details `sol::environment`를 생성하고, 소유자 액터를 'obj' 변수로 바인딩하며, `print` 함수를 엔진 로그로 리다이렉션합니다.
	 * 이 함수는 스크립트가 처음 로드될 때 한 번만 호출됩니다.
	 */
	void InitializeEnvironment();

	/**
	 * @brief 사용된 Lua 환경을 정리하고 자원을 해제합니다.
	 * @details EndPlay가 호출되거나 스크립트가 리로드될 때 호출되어 `sol::environment`를 초기 상태로 되돌립니다.
	 */
	void CleanupEnvironment();

private:
	/// @brief 스크립트 격리를 위한 독립적인 Lua 실행 환경. 각 스크립트는 자신만의 환경을 가집니다.
	sol::environment Env;
	/// @brief 스크립트가 성공적으로 로드되었는지 여부를 나타내는 플래그.
	bool bScriptLoaded = false;
	/// @brief Lua 환경이 초기화되었는지 여부를 나타내는 플래그.
	bool bEnvironmentInitialized = false;
};
