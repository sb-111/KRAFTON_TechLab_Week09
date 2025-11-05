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
local CameraShakeIntensity = 7.0 -- 쉐이크 강도
local CollisionVignetteDuration = 0

local Inertia = 1
-- z축만 회전할 거라서 flaot
local Torque = 500
-- 각속도에 따른 저항
local AngularRegistance = 5

-- 부스터 관련 변수
local BoosterActive = false
local BoosterForce = 5000  -- 부스터 추가 힘
local BoosterDuration = 2.0  -- 부스터 지속 시간 (초)
local BoosterTimeRemaining = 0.0
local BoosterCooldown = 5.0  -- 부스터 재사용 대기 시간 (초)
local BoosterCooldownRemaining = 0.0

local PlayerComponent = obj:GetPlayerComponent()

function OnThrustInput(InValue)
    ThrustInput = InValue
end

function OnSteerInput(InValue)
    SteerInput = InValue
end

-- 부스터
function OnBoosterInput()
    -- 쿨다운이 끝나고 부스터가 비활성 상태일 때만 활성화 가능
    if BoosterCooldownRemaining <= 0 and not BoosterActive then
        CollisionVignetteDuration = 0
        PlayerController:GetPlayerCameraManager():SetVignetteColor(Color(0,0,1,1))
        PlayerController:GetPlayerCameraManager():EnableVignetting(true)

        BoosterActive = true
        BoosterTimeRemaining = BoosterDuration
        BoosterCooldownRemaining = BoosterCooldown

        print(string.format("Booster activated! (Duration: %.1fs, Cooldown: %.1fs)", BoosterDuration, BoosterCooldown))

        -- 소리 재생
        local AudioComp = obj:GetAudioComponentByName("Booster")
        if AudioComp then
            AudioComp:Stop(true)
            AudioComp:Play(false)
        end
    else
        if BoosterActive then
            print("Booster already active!")
        else
            print(string.format("Booster on cooldown: %.1fs remaining", BoosterCooldownRemaining))
        end
    end
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
    if obj:GetActorLocation().x > 350 then
        return
    end
    -- 충돌 법선 = 진행 방향의 반대 (실제 충돌 접점 방향과 유사)
    local CollisionNormal = Velocity:GetNormalized() * (-1.0)

    -- Z축 제거 (XY 평면에서만 충돌 처리)
    CollisionNormal.z = 0
    if CollisionNormal:Size() > 0 then
        CollisionNormal = CollisionNormal:GetNormalized()
    else
        -- 속도가 0이면 오브젝트 중심 방향 사용
        local MyPos = obj:GetActorLocation()
        local OtherPos = Other:GetOwner():GetActorLocation()
        CollisionNormal = (MyPos - OtherPos)
        CollisionNormal.z = 0
        CollisionNormal = CollisionNormal:GetNormalized()
    end

    -- 충돌 법선 방향으로 밀어냄 (겹침 해제)
    local SeparationDistance = 2.0
    local MyPos = obj:GetActorLocation()
    obj:SetActorLocation(MyPos + CollisionNormal * SeparationDistance)

    -- 속도를 충돌 면에 반사 (법선 방향으로 튕김)
    local VelocityDotNormal = Vector.Dot(Velocity, CollisionNormal)
    if VelocityDotNormal < 0 then
        -- 충돌 면으로 향하고 있을 때만 반사
        Velocity = Velocity - CollisionNormal * (VelocityDotNormal * 2.0)
    end

    -- Z축 속도 제거 (XY 평면에서만 움직임)
    Velocity.z = 0

    UI:SetAfterCollisionTime(3)
    UI:AddScore(-30)

    

    -- 부스터 비활성화
    if BoosterActive then
        PlayerController:GetPlayerCameraManager():EnableVignetting(false)
        BoosterActive = false
        BoosterTimeRemaining = 0.0

        -- 부스터 오디오 중단
        local BoosterAudioComp = obj:GetAudioComponentByName("Booster")
        if BoosterAudioComp then
            BoosterAudioComp:Stop(true)
        end

        print("[Car] Booster disabled due to collision")
    end

    -- 카메라 쉐이크 시작
    CameraShakeTime = CameraShakeDuration

    PlayerController:GetPlayerCameraManager():SetVignetteColor(Color(1,0,0,1))
    PlayerController:GetPlayerCameraManager():EnableVignetting(true)
    CollisionVignetteDuration = 0.5
    PlayerController:GetPlayerCameraManager():StartCameraShake(
        CameraShakeTime,
        Vector(1,1,1),
        Vector(1,1,1),
        2.0,
        CameraShakeIntensity,
        ShakePattern.SineWave
    )

    -- 충돌 소리 재생
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
    local PCM = PlayerController:GetPlayerCameraManager()
    PCM:SetLetterboxSize(0.0)
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

    -- 부스터 초기화
    PlayerController:GetPlayerCameraManager():EnableVignetting(false)
    PlayerController:GetPlayerCameraManager():SetViewTarget(obj, 2)
    BoosterActive = false
    BoosterTimeRemaining = 0.0
    BoosterCooldownRemaining = 0.0

    print("[Car] Reset to initial state")
end

function BeginPlay()
    print("[BeginPlay] Car" .. tostring(obj))


    -- 초기 위치와 회전 저장
    InitialPosition = obj:GetActorLocation()
    InitialRotation = obj:GetActorRotation()
    PlayerController:Possess(Pawn)
    PlayerController:GetPlayerCameraManager():SetViewTarget(obj, 0)
    PlayerController:GetPlayerCameraManager():StartFadeOut(0, Color(0.1,0.1,0.1,1))
    PlayerController:GetPlayerCameraManager():StartFadeIn(3)
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

    if CollisionVignetteDuration > 0 then
        CollisionVignetteDuration = CollisionVignetteDuration - dt
        if CollisionVignetteDuration < 0 then
            CollisionVignetteDuration = 0
            PlayerController:GetPlayerCameraManager():EnableVignetting(false)
        end
    end
    -- 카메라 쉐이크 시간 감소
    if CameraShakeTime > 0 then
        CameraShakeTime = CameraShakeTime - dt
        if CameraShakeTime < 0 then
            CameraShakeTime = 0
        end
    end

    -- 부스터 타이머 업데이트
    if BoosterActive then
        BoosterTimeRemaining = BoosterTimeRemaining - dt
        if BoosterTimeRemaining <= 0 then
            PlayerController:GetPlayerCameraManager():EnableVignetting(false)
            BoosterActive = false
            BoosterTimeRemaining = 0
            print("Booster deactivated!")
        end
    end

    -- 부스터 쿨다운 업데이트
    if BoosterCooldownRemaining > 0 then
        BoosterCooldownRemaining = BoosterCooldownRemaining - dt
        if BoosterCooldownRemaining < 0 then
            BoosterCooldownRemaining = 0
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

    -- 부스터 힘 적용 (속도 제한 무시, 최대 속도 돌파 가능)
    if BoosterActive then
        NetForce = NetForce + ForwardVector * BoosterForce
    end

    local AirResistanceForce = Vector(0,0,0)
    AirResistanceForce = Velocity:GetNormalized() * AirResistance * Velocity:SizeSquared()
    NetForce = NetForce - AirResistanceForce

    local Acceleration = NetForce / Mass
    Velocity = Velocity + Acceleration * dt

    -- Z축 속도 제거 (XY 평면에서만 움직임)
    Velocity.z = 0

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
