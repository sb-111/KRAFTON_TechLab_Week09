-- Auto-generated script for 스태틱 메시_0
local Mass = 1.0
-- 공기저항
local AirResistance = 0.5
-- 차 엔진 힘(추진력, forward에 대한 벡터는 엑터로부터 얻으므로 float로 저장)
local MoveForce = 2000
local BreakForce = 50
local Velocity = Vector(0, 0, 0)
local AngularSpeed = 0



local Inertia = 1
-- z축만 회전할 거라서 flaot
local Torque = 500
-- 각속도에 따른 저항
local AngularRegistance = 5


function BeginOverlap(Other)
    print("[Car] BeginOverlap In Car")
    Velocity = Velocity * (-1.0)
    obj:SetActorLocation(obj:GetActorLocation() + Velocity:GetNormalized())
end
function EndOverlap(Other)

end
-- 스코어를 관리할 이벤트 시스템을 쓰면 좋겠지만
-- 시간관계상 UI를 게임 시스템처럼 사용함
function ScoreCoroutine()
    wait_until(function()
        local CheckPoint0 = -350.0
        return obj:GetActorLocation().x > CheckPoint0
    end)
    
    -- 게임 오버 상태 체크
    if not UI then
        print("[Car] Coroutine terminated: UI invalid")
        return
    end
    
    -- 남은 시간에 비례해서 스코어 증가
    local CurrentPlayTime = UI:GetPlayTime()
    
    UI:AddScore(math.floor(CurrentPlayTime) * 50)
    -- 다음 체크포인트까지 도달할 시간 추가
    UI:AddPlayTime(5)
    print("[Car] Coroutine Entered")

end

function BeginPlay()
    print("[BeginPlay] Car" .. tostring(obj))
    local ShapeComponent = obj:GetShapeComponent()
    ShapeComponent:RegisterBeginOverlapFunction(BeginOverlap)
    ShapeComponent:RegisterEndOverlapFunction(EndOverlap)

    start_coroutine(ScoreCoroutine)
end

function SetCameraPos()
    local CameraActor = GWorld:GetCameraActor()

    local Forward = CameraActor:GetActorRight() * (-10)
    local RelativeLocation = Forward

    CameraActor:SetActorLocation(obj:GetActorLocation() + RelativeLocation)
end

function Tick(dt)
    -- Update logic here

    local NetForce = Vector(0,0,0)
    local NetTorque = 0
    --Velocity = Velocity + Acceleration * dt
    local ForwardVector = Vector(0,0,0) - obj:GetActorRight()
    local ForwardSpeed = Vector.Dot(Velocity, ForwardVector)
    local LinearSpeed = Velocity:Size()
    local MaxMoveForceSpeed = 100
    local MoveForceFactor = math.min(LinearSpeed / MaxMoveForceSpeed, 1.0)
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
  

    SetCameraPos()
end

function EndPlay()
    print("[EndPlay] " .. tostring(obj))
end
