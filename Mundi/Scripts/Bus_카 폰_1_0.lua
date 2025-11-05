-- Auto-generated script for 스태틱 메시_0
local Mass = 1.0
-- 공기저항
local AirResistance = 0.5
-- 차 엔진 힘(추진력, forward에 대한 벡터는 엑터로부터 얻으므로 float로 저장)
local MoveForce = 2000
local BreakForce = 50
local Velocity = Vector(0, 0, 0)
local AngularSpeed = 0

-- 1이면 w, -1이면 s 0이면 input X 
local ThrustInput = 0
-- 1이면 d, -1이면 a 0이면 input X 
local SteerInput = 0

local InitialPosition = Vector(0, 0, 0)
local InitialRotation = Vector(0, 0, 0)
local prevGameOver = false  -- 게임 재시작 감지용

-- 코루틴 에러때문에 boolean으로 처리
local CheckPointPass0 = false
local CheckPointPass1 = false
local CheckPointPass2 = false
local CheckPointPass3 = false

-- 카메라 쉐이크 변수
local CameraShakeTime = 0       -- 남은 쉐이크 시간
local CameraShakeDuration = 0.5 -- 쉐이크 지속 시간 (초)
local CameraShakeIntensity = 2.0 -- 쉐이크 강도

local Inertia = 1
-- z축만 회전할 거라서 flaot
local Torque = 500
-- 각속도에 따른 저항
local AngularRegistance = 5

function OnThrustInput(InValue)
    ThrustInput = InValue
end

function OnSteerInput(InValue)
    SteerInput = InValue
end

local audioComp = nil
local savedVolume = 100.0  -- 뮤트 해제 시 복원할 볼륨

-- M키: 뮤트/뮤트 해제 토글
function OnMuteInput()
    if not audioComp then
        audioComp = obj:GetAudioComponent()
        if not audioComp then
            print("AudioComponent not found")
            return
        end
    end

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
    local AudioComp = nil
    if not AudioComp then
        AudioComp = obj:GetAudioComponentByName("Booster")
        if not AudioComp then
            return
        end
    end

    AudioComp:Stop(true)
    AudioComp:Play(false)
    print("Boost started playing")
end

-- P키: 정지
function OnStopInput()
    if not audioComp then
        audioComp = obj:GetAudioComponent()
        if not audioComp then
            print("AudioComponent not found")
            return
        end
    end

    audioComp:Stop(true)
    print("Audio stopped")
end

-- 왼쪽 화살표: 5초 뒤로
function OnLeftArrowInput()
    if not audioComp then
        audioComp = obj:GetAudioComponent()
        if not audioComp then
            return
        end
    end

    audioComp:SeekRelative(-5.0)
    local newPos = audioComp:GetPlaybackPosition()
    print(string.format("Seeked backward to %.2f seconds", newPos))
end

-- 오른쪽 화살표: 5초 앞으로
function OnRightArrowInput()
    if not audioComp then
        audioComp = obj:GetAudioComponent()
        if not audioComp then
            return
        end
    end

    audioComp:SeekRelative(5.0)
    local newPos = audioComp:GetPlaybackPosition()
    print(string.format("Seeked forward to %.2f seconds", newPos))
end

-- 위쪽 화살표: 볼륨 증가
function OnUpArrowInput()
    if not audioComp then
        audioComp = obj:GetAudioComponent()
        if not audioComp then
            return
        end
    end

    local currentVolume = audioComp:GetVolume()
    local newVolume = math.min(currentVolume + 10.0, 100.0)
    audioComp:SetVolume(newVolume)
    savedVolume = newVolume  -- 뮤트 해제 시 복원할 볼륨도 업데이트
    print(string.format("Volume: %.0f", newVolume))
end

-- 아래쪽 화살표: 볼륨 감소
function OnDownArrowInput()
    if not audioComp then
        audioComp = obj:GetAudioComponent()
        if not audioComp then
            return
        end
    end

    local currentVolume = audioComp:GetVolume()
    local newVolume = math.max(currentVolume - 10.0, 0.0)
    audioComp:SetVolume(newVolume)
    if newVolume > 0.0 then
        savedVolume = newVolume  -- 뮤트 해제 시 복원할 볼륨도 업데이트
    end
    print(string.format("Volume: %.0f", newVolume))
end
-- 코루틴 에러때문에 boolean으로 처리
function UpdateScore()
    local CurrentLocation = obj:GetActorLocation().x
    local CheckPoint0 = -350
    local CheckPoint1 = -100
    local CheckPoint2 = 100
    local EndPoint = 360
    if(not CheckPointPass0 and CurrentLocation > CheckPoint0 ) then
        -- 남은 시간에 비례해서 스코어 증가
        local CurrentPlayTime = UI:GetPlayTime()
        
        UI:AddScore(math.floor(CurrentPlayTime) * 50)
        -- 다음 체크포인트까지 도달할 시간 추가
        UI:AddPlayTime(5)
        CheckPointPass0 = true
    end

    if(not CheckPointPass1 and CurrentLocation > CheckPoint1 ) then
        local CurrentPlayTime = UI:GetPlayTime()
        
        UI:AddScore(math.floor(CurrentPlayTime) * 50)
        UI:AddPlayTime(10)
        CheckPointPass1 = true
    end
    if(not CheckPointPass2 and CurrentLocation > CheckPoint2 ) then
        local CurrentPlayTime = UI:GetPlayTime()
        
        UI:AddScore(math.floor(CurrentPlayTime) * 50)

        UI:AddPlayTime(10)
        CheckPointPass2 = true
    end
    if(CurrentLocation > EndPoint ) then
        local CurrentPlayTime = UI:GetPlayTime()
        
        UI:AddScore(math.floor(CurrentPlayTime) * 50)
        UI:SetGameOver(true)
    end
end

function BeginOverlap(Other)
    Velocity = Velocity * (-1.0)


    obj:SetActorLocation(obj:GetActorLocation() + Velocity:GetNormalized())
    UI:SetAfterCollisionTime(3)
    UI:AddScore(-30)

    -- 카메라 쉐이크 시작
    CameraShakeTime = CameraShakeDuration
    print("[Car] Collision! Camera shake triggered")

    local AudioComp = nil
    if not AudioComp then
        AudioComp = obj:GetAudioComponentByName("CarCrush")
        if not AudioComp then
            return
        end
    end

    AudioComp:Stop(true)
    AudioComp:Play(false)
    print("Car Crush started playing")

end
function EndOverlap(Other)

end
-- 게임 재시작 위함
function ResetCar()
    -- 위치와 회전 초기화
    obj:SetActorLocation(InitialPosition)
    obj:SetActorRotation(InitialRotation)

    -- 속도 초기화
    Velocity = Vector(0, 0, 0)
    AngularSpeed = 0

    -- 체크포인트 초기화
    CheckPointPass0 = false
    CheckPointPass1 = false
    CheckPointPass2 = false
    CheckPointPass3 = false
    UI:SetAfterCollisionTime(0)

    -- 카메라 쉐이크 초기화
    CameraShakeTime = 0

    print("[Car] Reset to initial state")
end

function BeginPlay()
    print("[BeginPlay] Car" .. tostring(obj))


    -- 초기 위치와 회전 저장
    InitialPosition = obj:GetActorLocation()
    InitialRotation = obj:GetActorRotation()
    PlayerController:Possess(Pawn)
    PlayerController:GetPlayerCameraManager():SetViewTarget(obj, 0)
    PlayerController:GetPlayerCameraManager():StartFadeOut(0, Color(1,0,0,1))
    PlayerController:GetPlayerCameraManager():StartFadeIn(2)
    print("[PlayerController] Possess Pawn")

    local ShapeComponent = obj:GetShapeComponent()
    ShapeComponent:RegisterBeginOverlapFunction(BeginOverlap)
    ShapeComponent:RegisterEndOverlapFunction(EndOverlap)

    -- AudioComponent 확인
    audioComp = obj:GetAudioComponent()
    if audioComp then
        print("[Audio] AudioComponent found! File: " .. tostring(audioComp:GetAudioFile()))
    else
        print("[Audio] WARNING: AudioComponent NOT FOUND! Please add AudioComponent to this actor.")
    end
end

function SetCameraPos()
    local CameraActor = GWorld:GetCameraActor()

    local Forward = CameraActor:GetActorRight() * (-10)
    local RelativeLocation = Forward

    -- 카메라 쉐이크 오프셋 추가
    local ShakeOffset = Vector(0, 0, 0)
    if CameraShakeTime > 0 then
        -- 시간이 지날수록 강도 감소 (감쇠)
        local DecayFactor = CameraShakeTime / CameraShakeDuration
        local CurrentIntensity = CameraShakeIntensity * DecayFactor

        -- 랜덤 오프셋 생성 (-1 ~ 1 범위)
        local RandomX = (math.random() * 2 - 1) * CurrentIntensity
        local RandomY = (math.random() * 2 - 1) * CurrentIntensity
        local RandomZ = (math.random() * 2 - 1) * CurrentIntensity * 0.5 -- Z축은 덜 흔들리게

        ShakeOffset = Vector(RandomX, RandomY, RandomZ)
    end

    CameraActor:SetActorLocation(obj:GetActorLocation() + RelativeLocation + ShakeOffset)
end

function Tick(dt)
    -- 게임 재시작 감지 (게임 오버 상태에서 플레이 상태로 전환)
    local currentGameOver = UI and UI:IsGameOver() or false
    if prevGameOver and not currentGameOver then
        ResetCar()
    end
    prevGameOver = currentGameOver

    -- 게임 오버 시 차량 움직임 정지 (UIManager를 통해 확인)
    if UI and UI:IsGameOver() then
        -- print("게임 오버 상태: 차량 정지")
        return
    end

    -- 카메라 쉐이크 시간 감소
    if CameraShakeTime > 0 then
        CameraShakeTime = CameraShakeTime - dt
        if CameraShakeTime < 0 then
            CameraShakeTime = 0
        end
    end

    -- Update logic here

    local NetForce = Vector(0,0,0)
    local NetTorque = 0
    --Velocity = Velocity + Acceleration * dt
    local ForwardVector = Vector(0,0,0) - obj:GetActorRight()
    local ForwardSpeed = Vector.Dot(Velocity, ForwardVector)
    local LinearSpeed = Velocity:Size()
    local MaxMoveForceSpeed = 100
    local MoveForceFactor = math.min(LinearSpeed / MaxMoveForceSpeed, 1.0)
    if UI:GetAfterCollisionTime() > 0 then
        -- 충돌 이후 저항 계수(작을수록 저항 큼)
        local AfterCollisionResistance = 0.3
        MoveForceFactor = MoveForceFactor * AfterCollisionResistance
        UI:AddAfterCollisionTime(-dt)
    end
    if ThrustInput > 0 then
       
        if ForwardSpeed > 0 then
            NetForce = NetForce + ForwardVector * (BreakForce + MoveForce*MoveForceFactor)
        else
            NetForce = NetForce + ForwardVector * BreakForce
        end
    end
    if ThrustInput < 0 then
        if ForwardSpeed > 0 then
            NetForce = NetForce - ForwardVector * BreakForce
        else
            NetForce = NetForce - ForwardVector * (BreakForce + MoveForce*MoveForceFactor)
        end
    end

    local AirResistanceForce = Vector(0,0,0)
    AirResistanceForce = Velocity:GetNormalized() * AirResistance * Velocity:SizeSquared()
    NetForce = NetForce - AirResistanceForce

    local Acceleration = NetForce / Mass
    Velocity = Velocity + Acceleration * dt
    obj:AddActorWorldLocation(Velocity * dt)


    if SteerInput < 0 then
        if ForwardSpeed > 0 then
            NetTorque = NetTorque - Torque
        else
            NetTorque = NetTorque + Torque
        end
    end
    if SteerInput > 0 then
        if ForwardSpeed > 0 then
            NetTorque = NetTorque + Torque
        else
            NetTorque = NetTorque - Torque
        end
    end
    
    -- 이 속도에 다다르면 각속도 최대
    local MaxSteeringSpeed = 40
   
    local SteeringFactor = math.min(LinearSpeed / MaxSteeringSpeed, 1.0)
    NetTorque = NetTorque * SteeringFactor

    
    local RegistanceTorque = -AngularRegistance * AngularSpeed
    NetTorque = NetTorque + RegistanceTorque
    
    local AngularAcceleration = NetTorque / Inertia 
    AngularSpeed = AngularSpeed + AngularAcceleration * dt
    local AngularVelocity = Vector(0,0, AngularSpeed * dt) 
    obj:AddActorWorldRotation(AngularVelocity)
  
    UpdateScore()
    --SetCameraPos()
end

function EndPlay()
    print("[EndPlay] " .. tostring(obj))
end
