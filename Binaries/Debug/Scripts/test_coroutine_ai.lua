-- 학습자료 예시 구현: EnemyAI 패트롤 시뮬레이션
-- 조건 기반 대기 포함

-- 전역 변수
move_done = false
move_target_x = 0
move_target_y = 0

function move_to(x, y)
    print(string.format("[AI] Moving to (%.1f, %.1f)", x, y))
    move_target_x = x
    move_target_y = y
    move_done = false

    -- 실제로는 액터를 이동시키는 코드가 여기에 들어감
    -- obj:SetActorLocation(Vector(x, y, 0))
end

function wait_until_move_done()
    print("[AI] Waiting until move is done...")
    wait_until(function()
        return move_done
    end)
    print("[AI] Move completed!")
end

function patrol_anim()
    print("[AI] Playing patrol animation")
end

function EnemyAI_start()
    print("[AI] Patrol start")

    for i = 1, 3 do
        move_to(10 * i, 0)
        wait_until_move_done()
        patrol_anim()
        wait(1.0)
    end

    print("[AI] Done")
end

-- 이동 완료 시뮬레이션 (2초 후 자동 완료)
local move_timer = 0
local is_moving = false

function BeginPlay()
    print("[BeginPlay] Starting Enemy AI coroutine...")
    start_coroutine(EnemyAI_start)
end

function Tick(dt)
    -- 이동 시뮬레이션: move_to 호출 후 2초 뒤 완료
    if not move_done and is_moving then
        move_timer = move_timer + dt
        if move_timer >= 2.0 then
            print("[AI] (Simulated move completion)")
            move_done = true
            move_timer = 0
            is_moving = false
        end
    end

    -- move_to가 호출되면 is_moving = true
    if not is_moving and not move_done and (move_target_x ~= 0 or move_target_y ~= 0) then
        is_moving = true
    end
end

function EndPlay()
    print("[EndPlay] AI script finished")
end
