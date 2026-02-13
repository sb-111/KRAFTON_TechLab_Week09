-- ============================================
-- AudioComponent 사용 예제
-- ============================================
-- 이 스크립트는 AudioComponent의 모든 기능을 보여줍니다.
-- Actor에 AudioComponent가 추가되어 있어야 합니다.

local audioComp = nil
local elapsedTime = 0.0

function BeginPlay()
    print("=== AudioComponent Example Started ===")

    -- AudioComponent 가져오기
    audioComp = Actor:GetComponentByClass("UAudioComponent")

    if audioComp then
        print("AudioComponent found!")

        -- 오디오 파일 설정 (Data/Sound 폴더의 wav 파일)
        audioComp:SetAudioFile("Data/Sound/BGM.wav")

        -- 볼륨 설정 (0 ~ 100)
        audioComp:SetVolume(10.0)
        print("Volume set to: " .. audioComp:GetVolume())

        -- 재생 속도 설정 (0.25 ~ 2.0)
        audioComp:SetPlaybackSpeed(1.0)
        print("Playback speed: " .. audioComp:GetPlaybackSpeed())

        -- 루프 설정
        audioComp:SetLoop(false)
        print("Loop enabled: " .. tostring(audioComp:IsLoop()))

        -- 재생 시작
        audioComp:Play(false)
        print("Audio started playing")
    else
        print("ERROR: AudioComponent not found! Please add AudioComponent to this Actor.")
    end
end

function Tick(DeltaTime)
    if not audioComp then
        return
    end

    elapsedTime = elapsedTime + DeltaTime

    -- 1초마다 재생 정보 출력
    if elapsedTime > 1.0 then
        elapsedTime = 0.0

        if audioComp:IsPlaying() then
            local currentTime = audioComp:GetPlaybackPosition()
            local duration = audioComp:GetDuration()
            print(string.format("Playing: %.2f / %.2f seconds", currentTime, duration))
        end
    end
end

function EndPlay()
    if audioComp then
        -- 재생 중지
        audioComp:Stop(true)
        print("Audio stopped")
    end
    print("=== AudioComponent Example Ended ===")
end

-- ============================================
-- 키 입력 예제 (PlayerController → Pawn → ScriptComponent 경로로 자동 호출)
-- ============================================

local savedVolume = 10.0  -- 뮤트 해제 시 복원할 볼륨

-- M키: 뮤트/뮤트 해제 토글
function OnMuteInput()
    if not audioComp then return end

    local currentVolume = audioComp:GetVolume()
    if currentVolume > 0.0 then
        -- 뮤트: 현재 볼륨 저장하고 0으로 설정
        savedVolume = currentVolume
        audioComp:SetVolume(0.0)
        print("Audio muted")
    else
        -- 뮤트 해제: 저장된 볼륨으로 복원
        audioComp:SetVolume(savedVolume)
        print("Audio unmuted (Volume: " .. savedVolume .. ")")
    end
end

-- 스페이스바: 재생/일시정지 토글
function OnPlayPauseInput()
    if not audioComp then return end

    if audioComp:IsPlaying() then
        audioComp:Pause(true)
        print("Audio paused")
    else
        -- 재생 위치가 0이면 처음부터, 아니면 이어서 재생
        if audioComp:GetPlaybackPosition() > 0.0 then
            audioComp:Resume()
            print("Audio resumed")
        else
            audioComp:Play(false)
            print("Audio started playing")
        end
    end
end

-- P키: 정지
function OnStopInput()
    if not audioComp then return end

    audioComp:Stop(true)
    print("Audio stopped")
end

-- ============================================
-- 아래 키 입력은 더 이상 사용되지 않습니다 (참고용으로 보존)
-- M/Space/P 키는 위의 함수들로 자동 호출됩니다
-- ============================================

-- 스페이스바: 재생/일시정지 토글 (구버전)
function OnSpacePressed()
    if not audioComp then return end

    if audioComp:IsPlaying() then
        audioComp:Pause(true)
        print("Audio paused")
    else
        audioComp:Resume()
        print("Audio resumed")
    end
end

-- S키: 정지
function OnSPressed()
    if not audioComp then return end

    audioComp:Stop(true)
    print("Audio stopped")
end

-- 왼쪽 화살표: 5초 뒤로
function OnLeftArrowPressed()
    if not audioComp then return end

    audioComp:SeekRelative(-5.0)
    local newPos = audioComp:GetPlaybackPosition()
    print(string.format("Seeked backward to %.2f seconds", newPos))
end

-- 오른쪽 화살표: 5초 앞으로
function OnRightArrowPressed()
    if not audioComp then return end

    audioComp:SeekRelative(5.0)
    local newPos = audioComp:GetPlaybackPosition()
    print(string.format("Seeked forward to %.2f seconds", newPos))
end

-- 위쪽 화살표: 볼륨 증가
function OnUpArrowPressed()
    if not audioComp then return end

    local currentVolume = audioComp:GetVolume()
    local newVolume = math.min(currentVolume + 10.0, 100.0)
    audioComp:SetVolume(newVolume)
    print(string.format("Volume: %.0f", newVolume))
end

-- 아래쪽 화살표: 볼륨 감소
function OnDownArrowPressed()
    if not audioComp then return end

    local currentVolume = audioComp:GetVolume()
    local newVolume = math.max(currentVolume - 10.0, 0.0)
    audioComp:SetVolume(newVolume)
    print(string.format("Volume: %.0f", newVolume))
end

-- ============================================
-- 사용 가능한 AudioComponent 함수 목록
-- ============================================
--[[
재생 제어:
- Play(bLoopOverride)        : 재생 시작
- Stop(bImmediate)            : 정지
- Pause(bSavePosition)        : 일시정지 (기본값: 위치 저장)
- Resume()                    : 재개
- IsPlaying()                 : 재생 중 여부 확인

위치 제어:
- GetPlaybackPosition()       : 현재 재생 시간 (초) 가져오기
- SetPlaybackPosition(seconds): 재생 위치 설정
- GetDuration()               : 전체 길이 (초) 가져오기
- SeekRelative(seconds)       : 상대 위치 이동 (±초)

볼륨/속도 제어:
- SetVolume(volume)           : 볼륨 설정 (0 ~ 100)
- GetVolume()                 : 현재 볼륨 가져오기
- SetPlaybackSpeed(speed)     : 재생 속도 설정 (0.25 ~ 2.0)
- GetPlaybackSpeed()          : 현재 재생 속도 가져오기

설정:
- SetLoop(bLoop)              : 루프 설정
- IsLoop()                    : 루프 설정 여부 확인
- SetAudioFile(path)          : 오디오 파일 경로 설정
- GetAudioFile()              : 현재 오디오 파일 경로 가져오기
--]]
