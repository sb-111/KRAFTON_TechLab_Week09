-- Auto-generated script for 스태틱 메시_0

function BeginPlay()
    print("[BeginPlay] " .. tostring(obj))
end

function Tick(dt)
    -- Update logic here
    GWorld:GetCameraActor():SetActorLocation(obj:GetActorLocation() + Vector(40, 0, 30))
end

function EndPlay()
    print("[EndPlay] " .. tostring(obj))
end
