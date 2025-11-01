#pragma once
#include "sol/sol.hpp"
/**
 * @class FCoroutineManager
 * @brief World 레벨에서 모든 Lua 코루틴을 관리하는 매니저
 * @details
 * - 코루틴 시작/중지/업데이트
 * - 시간 기반 대기 (wait)
 * - 조건 기반 대기 (wait_until)
 * - 자동 완료 감지 및 정리
 */
class FCoroutineManager
{
public:
	/**
	* @brief 코루틴 핸들 structure
	*/
	struct FCoroutineHandle
	{
		int32 ID = -1;						// 고유 ID
		sol::thread Thread;					// 컨텍스트 스레드
		sol::coroutine Coroutine;			// Lua 코루틴 객체
		float WaitTime = 0.0f;				// 남은 대기 시간
		bool bWaitingForCondition = false;	// 조건 대기 중 여부
		sol::protected_function Condition;	// 대기 조건 함수
		/**
		 * @brief 대기 중인지 확인
		 */
		bool IsWaiting() const
		{
			return WaitTime > 0.0f || bWaitingForCondition;
		}
	};
	FCoroutineManager() = default;
	~FCoroutineManager() = default;

	/**
	 * @brief 새 코루틴 시작
	 * @param Coro sol::coroutine 객체
	 *  
	*/

	void Initialize(sol::state* InLuaState);	// World에서 호출

	int32 StartCoroutine(sol::function Func);	// 코루틴 시작
	void StopCoroutine(int32 ID);				// 특정 코루틴 중지

	void Update(float DeltaTime);				// 매 프레임 호출

	bool IsRunning(int32 ID) const;				// 실행 여부 확인

private:
	/**
	 * @brief coroutine.yield()에서 반환된 값을 처리
	 * @param Handle 코루틴 핸들
	 * @param Result yield의 반환값
	 */
	void ProcessYieldResult(FCoroutineHandle& Handle, const sol::protected_function_result& Result);

	TArray<FCoroutineHandle> ActiveCoroutines;	// 실행 중 코루틴 리스트
	int32 NextID = 0;							// 고유 ID 생성용
	sol::state* LuaStatePtr = nullptr;			// World의 LuaState 참조
};