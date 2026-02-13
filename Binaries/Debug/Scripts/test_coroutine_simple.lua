-- 간단한 코루틴 테스트 스크립트
-- 시간 기반 대기만 테스트

function simple_coroutine_test()
    print("[Coroutine] Start!")
    print("[Coroutine] Waiting 1 second...")
    wait(1.0)

    print("[Coroutine] After 1 second")
    print("[Coroutine] Waiting 2 seconds...")
    wait(2.0)

    print("[Coroutine] After 3 seconds total")
    print("[Coroutine] Done!")
end

function BeginPlay()
    print("[BeginPlay] Starting simple coroutine test...")
    local co_id = start_coroutine(simple_coroutine_test)
    print("[BeginPlay] Coroutine started with ID: " .. co_id)
end

function Tick(dt)
    -- 아무것도 안함
end

function EndPlay()
    print("[EndPlay] Script finished")
end
