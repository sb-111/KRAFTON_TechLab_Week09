-- Auto-generated script for 스태틱 메시_0
local Mass = 1.0
-- 공기저항
local AirResistance = 0.5
-- 차 엔진 힘(추진력, forward에 대한 벡터는 엑터로부터 얻으므로 float로 저장)
local MoveForce = 2000
local BreakForce = 50
local Velocity = Vector(0, 0, 0)
local AngularSpeed = 0

local InitialPosition = Vector(0, 0, 0)
local InitialRotation = Vector(0, 0, 0)
local prevGameOver = false  -- 게임 재시작 감지용

-- 코루틴 에러때문에 boolean으로 처리
local CheckPointPass0 = false
local CheckPointPass1 = false
local CheckPointPass2 = false
local CheckPointPass3 = false

local Inertia = 1
-- z축만 회전할 거라서 flaot
local Torque = 500
-- 각속도에 따른 저항
local AngularRegistance = 5


-- 코루틴 에러때문에 boolean으로 처리
function UpdateScore()
    local CurrentLocation = obj:GetActorLocation().x
    local CheckPoint0 = -350
    local CheckPoint1 = -100
    local CheckPoint2 = 100
    local CheckPoint3 = 360
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
    if(not CheckPointPass3 and CurrentLocation > CheckPoint3 ) then
        local CurrentPlayTime = UI:GetPlayTime()
        
        UI:AddScore(math.floor(CurrentPlayTime) * 50)
        CheckPointPass3 = true
    end
end

function BeginOverlap(Other)
    Velocity = Velocity * (-1.0)
    obj:SetActorLocation(obj:GetActorLocation() + Velocity:GetNormalized())
    UI:SetAfterCollisionTime(3)
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

    -- 스코어 코루틴 재시작
    start_coroutine(ScoreCoroutine)

    print("[Car] Reset to initial state")
end

function BeginPlay()
    print("[BeginPlay] Car" .. tostring(obj))


    -- 초기 위치와 회전 저장
    InitialPosition = obj:GetActorLocation()
    InitialRotation = obj:GetActorRotation()
    start_coroutine(ScoreCoroutine)

    local ShapeComponent = obj:GetShapeComponent()
    ShapeComponent:RegisterBeginOverlapFunction(BeginOverlap)
    ShapeComponent:RegisterEndOverlapFunction(EndOverlap)

end

function SetCameraPos()
    local CameraActor = GWorld:GetCameraActor()

    local Forward = CameraActor:GetActorRight() * (-10)
    local RelativeLocation = Forward

    CameraActor:SetActorLocation(obj:GetActorLocation() + RelativeLocation)
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
    if Input:IsKeyDown(Keys.W) then
        if ForwardSpeed > 0 then
            NetForce = NetForce + ForwardVector * (BreakForce + MoveForce*MoveForceFactor)
        else
            NetForce = NetForce + ForwardVector * BreakForce
        end
    end
    if Input:IsKeyDown(Keys.S) then
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


    if Input:IsKeyDown(Keys.A) then
        if ForwardSpeed > 0 then
            NetTorque = NetTorque - Torque
        else
            NetTorque = NetTorque + Torque
        end
    end
    if Input:IsKeyDown(Keys.D) then
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
    SetCameraPos()
end

function EndPlay()
    print("[EndPlay] " .. tostring(obj))
end
