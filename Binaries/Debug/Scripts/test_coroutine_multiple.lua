-- 여러 코루틴 동시 실행 테스트
-- 각 코루틴마다 독립적인 thread를 가지므로 안전하게 동시 실행 가능

function coroutine_a()
    print("[Coroutine A] Start")
    for i = 1, 3 do
        print("[Coroutine A] Step " .. i)
        wait(1.0)
    end
    print("[Coroutine A] Done")
end

function coroutine_b()
    print("[Coroutine B] Start")
    for i = 1, 5 do
        print("[Coroutine B] Step " .. i)
        wait(0.5)
    end
    print("[Coroutine B] Done")
end

function coroutine_c()
    print("[Coroutine C] Start")
    wait(2.5)
    print("[Coroutine C] After 2.5 seconds")
    wait(1.0)
    print("[Coroutine C] Done")
end

function BeginPlay()
    print("[BeginPlay] Starting multiple coroutines...")

    local id_a = start_coroutine(coroutine_a)
    local id_b = start_coroutine(coroutine_b)
    local id_c = start_coroutine(coroutine_c)

    print("[BeginPlay] Started 3 coroutines:")
    print("  - Coroutine A: ID " .. id_a)
    print("  - Coroutine B: ID " .. id_b)
    print("  - Coroutine C: ID " .. id_c)
end

function Tick(dt)
    -- 아무것도 안함
end

function EndPlay()
    print("[EndPlay] Multiple coroutines test finished")
end
