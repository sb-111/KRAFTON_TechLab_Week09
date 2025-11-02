-- Auto-generated script for 스태틱 메시_0
local Velocity = Vector(0, 0, 0)
local Acceleration = Vector(0, 0, -9.8)

function BeginPlay()
    print("[BeginPlay] " .. tostring(obj))

end

function Tick(dt)
    -- Update logic here
    local CameraActor = GWorld:GetCameraActor()

    local Forward = CameraActor:GetActorRight() * (-60)
    local RelativeLocation = Forward

    CameraActor:SetActorLocation(obj:GetActorLocation() + RelativeLocation)

    Velocity = Velocity + Acceleration * dt
    obj:AddActorWorldLocation(dt * Velocity)
end

function EndPlay()
    print("[EndPlay] " .. tostring(obj))
end
