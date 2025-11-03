-- Game Manager Lua Script
-- 게임의 시작, 종료, 점수 관리를 담당하는 스크립트
-- 이 스크립트는 PIE 시작 버튼을 누르면 실행

-- ═══════════════════════════════════════════════════════
-- 게임 상태 변수
-- ═══════════════════════════════════════════════════════
local playTime = 10.0           -- 제한 시간 (초)
local maxPlayTime = 10.0        -- 최대 제한 시간
local currentScore = 0          -- 현재 점수
local bIsGameOver = false             -- 게임 종료 여부 
local gameCheckCoroutineId = -1 -- 게임 종료 체크 코루틴 ID

-- ═══════════════════════════════════════════════════════
-- 게임 시작 (PIE 시작 버튼을 누르면 호출)
-- ═══════════════════════════════════════════════════════
function BeginPlay()
    print("[GameManager] Game Starting...")

    -- 게임 초기 설정
    OnGameRestart()

    -- 재시작 버튼 콜백 등록
    UI:SetRestartCallback(OnGameRestart)

    -- 게임 종료 체크 코루틴 시작
    --gameCheckCoroutineId = start_coroutine(GameEndCheckCoroutine)

    print("[GameManager] Game Started Successfully!")
end

-- ═══════════════════════════════════════════════════════
-- 매 프레임 업데이트
-- ═══════════════════════════════════════════════════════
function Tick(dt)
    if bIsGameOver then
        return
    end
    
    -- 시간 카운트 다운
    playTime = UI:GetPlayTime() - dt
    UI:UpdateTime(playTime)

    -- 예제: 임의로 점수를 증가시킵니다 (실제로는 게임 로직에 따라 변경)
    -- 실제 게임에서는 플레이어가 무언가를 획득하거나 적을 처치할 때 점수 증가
    --  if math.random() < 0.01 then  -- 1% 확률로 점수 증가
    --    AddScore(10)
    -- end
end

-- ═══════════════════════════════════════════════════════
-- 게임 종료 시 호출
-- ═══════════════════════════════════════════════════════
function EndPlay()
    print("[GameManager] Game Ending...")

    -- 코루틴 중지
    if gameCheckCoroutineId >= 0 then
        stop_coroutine(gameCheckCoroutineId)
    end

    print("[GameManager] Game Ended")
end

-- ═══════════════════════════════════════════════════════
-- 게임 재시작 (재시작 버튼을 누르면 호출)
-- ═══════════════════════════════════════════════════════
function OnGameRestart()
    print("[GameManager] Restarting Game...")

    -- 게임 상태 초기화
    UI:UpdateTime(maxPlayTime)
    currentScore = 0
    bIsGameOver = false
    UI:SetGameOver(false)  -- UIManager를 통해 게임 오버 상태 해제

    -- Game Over UI 숨기기
    UI:SetGameOverUIVisibility(false)

    -- In Game UI 활성화
    UI:SetInGameUIVisibility(true)

    -- 점수 초기화
    UI:UpdateScore(0)

    -- 게임 종료 체크 코루틴 재시작
    if gameCheckCoroutineId >= 0 then
        stop_coroutine(gameCheckCoroutineId)
    end
    gameCheckCoroutineId = start_coroutine(GameEndCheckCoroutine)

    print("[GameManager] Game Restarted!")
end

-- ═══════════════════════════════════════════════════════
-- 게임 종료 처리
-- ═══════════════════════════════════════════════════════
function GameOver()
    if bIsGameOver then
        print("[GameManager] GameOver 이미 호출됨 (중복 호출 방지)")
        return
    end
    UI:SetGameOver(true)
    bIsGameOver = true
end

-- ═══════════════════════════════════════════════════════
-- 게임 종료 조건 체크 코루틴
-- 제한 시간이 지나거나 플레이어가 죽으면 게임 종료
-- ═══════════════════════════════════════════════════════
function GameEndCheckCoroutine()
    print("[GameManager] Game End Check Coroutine Started")

    -- 제한 시간이 0이 될 때까지 대기
    wait_until(function()
        -- UI가 유효한지 체크 (게임 종료 시 무효화될 수 있음)
        return UI:GetPlayTime() <= 0 or bIsGameOver
    end)

    -- playTime이 0이 되어서 종료조건을 만족하면 resume되고 GameOver를 통해 bIsGameOver를 true로 만들어줌
    if not bIsGameOver then
        print("[GameManager] Time Over!")
        GameOver()
    end

    print("[GameManager] Game End Check Coroutine Ended")
end

-- ═══════════════════════════════════════════════════════
-- 플레이어 사망 처리 (외부에서 호출 가능)
-- 예: 장애물에 부딪혔을 때 다른 스크립트에서 이 함수를 호출
-- ═══════════════════════════════════════════════════════
function OnPlayerDeath()
    print("[GameManager] Player Died!")
    GameOver()
end

-- ═══════════════════════════════════════════════════════
-- 게임 플레이 중인지 확인
-- ═══════════════════════════════════════════════════════
function IsPlaying()
    return not bIsGameOver
end