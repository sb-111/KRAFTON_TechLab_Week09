#include "CoroutineManager.h"
#include "pch.h"
void FCoroutineManager::Initialize(sol::state* InLuaState)
{
	LuaStatePtr = InLuaState;
	UE_LOG("[FCoroutineManager] Initialized");
}

int32 FCoroutineManager::StartCoroutine(sol::function Func)
{
	// LuaState 포인터 확인
	if (!LuaStatePtr)
	{
		UE_LOG("[FCoroutineManager] Error: LuaState not initialized");
		return -1;
	}
	
	// 함수 유효성 확인
	if (!Func.valid())
	{
		UE_LOG("[FCoroutineManager] Error: Invalid function");
		return -1;
	}
	
	// 새 thraed 생성
	sol::thread NewThread = sol::thread::create(*LuaStatePtr);
	// thread 유효성 확인
	if (!NewThread.valid())
	{
		UE_LOG("[FCoroutineManager] Error: Failed to create thread");
		return -1;
	}

	// thread의 lua state 가져오기
	sol::state_view ThreadState = NewThread.state();

	// coroutine 생성 (state_view를 전달해야 함!)
	sol::coroutine NewCoroutine(ThreadState, Func);
	// coroutine 유효성 확인
	if (!NewCoroutine.valid())
	{
		UE_LOG("[FCoroutineManager] Error: Failed to create coroutine");
		return -1;
	}

	// FCoroutineHandle 생성
	FCoroutineHandle NewHandle;
	// 고유 ID 할당
	NewHandle.ID = NextID++;	// 고유 ID 할당
	NewHandle.Thread = NewThread;	// sol::thread는 RAII라 스코프 이탈 시 파괴 -> coroutine도 무효화 -> 따라서 저장해야 함.
	NewHandle.Coroutine = NewCoroutine;
	NewHandle.WaitTime = 0.0f;
	NewHandle.bWaitingForCondition = false;

	UE_LOG("[FCoroutineManager] Handle created with ID=%d", NewHandle.ID);

	// Coroutine 첫 실행 (BeginPlay까지)
	auto Result = NewCoroutine();

	// 실행 결과 확인 (에러 체크)
	if (!Result.valid())
	{
		sol::error Err = Result;
		UE_LOG("[FCoroutineManager] Error on first resume (ID=%d): %s",
			NewHandle.ID, Err.what());
		return -1;  // 실패 시 목록에 추가 안 함
	}

	// === yield 반환값 처리 (wait, wait_until 등) ===
	// 반환값이 있는지 확인
	if (Result.return_count() > 0)
	{
		// yield 타입 파싱 (wait, wait_until 등)
		ProcessYieldResult(NewHandle, Result);

		UE_LOG("[FCoroutineManager] Yield result processed (ID=%d)", NewHandle.ID);
	}

	// === 즉시 완료 확인 ===
	// Coroutine 상태 확인
	// call_status::ok = 완료, call_status::yielded = 일시 중지
	sol::call_status Status = NewCoroutine.status();
	if (Status == sol::call_status::ok)
	{
		// 첫 resume에서 바로 완료된 경우
		// 예: function() print("Done") end (yield 없이 즉시 종료)
		UE_LOG("[FCoroutineManager] Completed immediately (ID=%d)", NewHandle.ID);

		// 목록에 추가하지 않고 ID만 반환
		return NewHandle.ID;
	}
	// === 활성 코루틴 목록에 추가 ===
	ActiveCoroutines.push_back(NewHandle);
	UE_LOG("[FCoroutineManager] Added to active list (ID=%d, Total=%d)",
		NewHandle.ID, (int)ActiveCoroutines.size());

	return NewHandle.ID;
}

void FCoroutineManager::StopCoroutine(int32 ID)
{
	// ActiveCoroutines에서 ID로 찾기
	for (auto It = ActiveCoroutines.begin(); It != ActiveCoroutines.end(); ++It)
	{
		if (It->ID == ID)
		{
			UE_LOG("[FCoroutineManager] Stopped coroutine ID=%d", ID);
			ActiveCoroutines.erase(It);
			return;
		}
	}

	UE_LOG("[FCoroutineManager] Warning: Coroutine ID=%d not found", ID);
}

void FCoroutineManager::Update(float DeltaTime)
{
	if (ActiveCoroutines.empty())
		return;

	// 참조가 아닌 반복자 사용 (erase를 위해)
	for (auto It = ActiveCoroutines.begin(); It != ActiveCoroutines.end();)
	{
		FCoroutineHandle& Handle = *It;

		// 1. 시간 기반 대기 처리
		if (Handle.WaitTime > 0.0f)
		{
			Handle.WaitTime -= DeltaTime;

			if (Handle.WaitTime > 0.0f)
			{
				++It;
				continue;  // 아직 대기 중
			}

			Handle.WaitTime = 0.0f;  // 대기 완료
		}

		// 2. 조건 기반 대기 처리
		if (Handle.bWaitingForCondition)
		{
			if (Handle.Condition.valid())
			{
				// 조건 함수 실행
				auto CondResult = Handle.Condition();

				if (!CondResult.valid())
				{
					sol::error Err = CondResult;
					UE_LOG("[FCoroutineManager] Error in condition (ID=%d): %s",
						Handle.ID, Err.what());
					It = ActiveCoroutines.erase(It);
					continue;
				}

				// 조건 결과 확인 (true면 대기 종료)
				bool bConditionMet = false;
				if (CondResult.return_count() > 0)
				{
					sol::object resultObj = CondResult[0];
					if (resultObj.is<bool>())
					{
						bConditionMet = resultObj.as<bool>();
					}
				}

				if (!bConditionMet)
				{
					++It;
					continue;  // 조건 미충족, 계속 대기
				}
			}

			Handle.bWaitingForCondition = false;  // 조건 충족
		}

		// 3. 대기가 끝났으면 코루틴 재개 (resume)
		if (!Handle.IsWaiting())
		{
			auto Result = Handle.Coroutine();

			// 에러 체크
			if (!Result.valid())
			{
				sol::error Err = Result;
				UE_LOG("[FCoroutineManager] Error during resume (ID=%d): %s",
					Handle.ID, Err.what());
				It = ActiveCoroutines.erase(It);
				continue;
			}

			// yield 반환값 처리
			if (Result.return_count() > 0)
			{
				ProcessYieldResult(Handle, Result);
			}

			// 코루틴 완료 확인
			// call_status::ok = 완료, call_status::yielded = 일시 중지
			sol::call_status Status = Handle.Coroutine.status();
			if (Status == sol::call_status::ok)
			{
				UE_LOG("[FCoroutineManager] Coroutine finished (ID=%d)", Handle.ID);
				It = ActiveCoroutines.erase(It);
				continue;
			}
		}

		++It;
	}
}

bool FCoroutineManager::IsRunning(int32 ID) const
{
	// ActiveCoroutines에서 ID가 존재하는지 확인
	for (const FCoroutineHandle& Handle : ActiveCoroutines)
	{
		if (Handle.ID == ID)
		{
			return true;
		}
	}
	return false;
}

void FCoroutineManager::ProcessYieldResult(FCoroutineHandle& Handle, const sol::protected_function_result& Result)
{
	// yield에서 반환값이 없으면 무시
	if (Result.return_count() == 0)
	{
		return;
	}

	// 첫 번째 반환값 확인
	sol::object FirstArg = Result[0];

	// 문자열인 경우: "wait", "wait_until" 같은 명령어
	if (FirstArg.is<std::string>())
	{
		std::string Command = FirstArg.as<std::string>();

		// === wait 명령: yield("wait", seconds) ===
		if (Command == "wait" && Result.return_count() > 1)
		{
			sol::object SecondArg = Result[1];
			if (SecondArg.is<float>())
			{
				float Seconds = SecondArg.as<float>();
				Handle.WaitTime = Seconds;
				UE_LOG("[FCoroutineManager] Coroutine ID=%d waiting for %.2f seconds",
					Handle.ID, Seconds);
			}
			else if (SecondArg.is<double>())
			{
				double Seconds = SecondArg.as<double>();
				Handle.WaitTime = static_cast<float>(Seconds);
				UE_LOG("[FCoroutineManager] Coroutine ID=%d waiting for %.2f seconds",
					Handle.ID, Handle.WaitTime);
			}
			else
			{
				UE_LOG("[FCoroutineManager] Warning: 'wait' command requires numeric argument (ID=%d)",
					Handle.ID);
			}
		}
		// === wait_until 명령: yield("wait_until", function) ===
		else if (Command == "wait_until" && Result.return_count() > 1)
		{
			sol::object SecondArg = Result[1];
			if (SecondArg.is<sol::function>())
			{
				Handle.Condition = SecondArg.as<sol::protected_function>();
				Handle.bWaitingForCondition = true;
				UE_LOG("[FCoroutineManager] Coroutine ID=%d waiting for condition",
					Handle.ID);
			}
			else
			{
				UE_LOG("[FCoroutineManager] Warning: 'wait_until' command requires function argument (ID=%d)",
					Handle.ID);
			}
		}
		else
		{
			UE_LOG("[FCoroutineManager] Warning: Unknown yield command '%s' (ID=%d)",
				Command.c_str(), Handle.ID);
		}
	}
	// 숫자가 직접 반환된 경우: yield(1.0) → wait(1.0)으로 간주
	else if (FirstArg.is<float>() || FirstArg.is<double>())
	{
		float Seconds = FirstArg.is<float>() ? FirstArg.as<float>() : static_cast<float>(FirstArg.as<double>());
		Handle.WaitTime = Seconds;
		UE_LOG("[FCoroutineManager] Coroutine ID=%d waiting for %.2f seconds (direct yield)",
			Handle.ID, Seconds);
	}
	// 함수가 직접 반환된 경우: yield(function) → wait_until(function)으로 간주
	else if (FirstArg.is<sol::function>())
	{
		Handle.Condition = FirstArg.as<sol::protected_function>();
		Handle.bWaitingForCondition = true;
		UE_LOG("[FCoroutineManager] Coroutine ID=%d waiting for condition (direct yield)",
			Handle.ID);
	}
}
