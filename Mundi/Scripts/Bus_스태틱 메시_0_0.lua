-- Auto-generated script for 스태틱 메시_0

function BeginPlay()
    print("[BeginPlay] " .. tostring(obj))
end

function Tick(dt)
    -- Update logic here
    local CameraActor = GWorld:GetCameraActor()

    local Forward = CameraActor:GetActorRight() * (-60)
    local RelativeLocation = Forward

    CameraActor:SetActorLocation(obj:GetActorLocation() + RelativeLocation)
end

function EndPlay()
    print("[EndPlay] " .. tostring(obj))
end
