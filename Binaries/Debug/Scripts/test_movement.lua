-- Test Script: Actor Movement
-- 이 스크립트는 Actor를 계속 회전시킵니다.

local rotationSpeed = 45  -- degrees per second
local moveSpeed = 50      -- units per second
local elapsedTime = 0

function BeginPlay()
    print("=== [Lua Script Test] ===")
    print("Actor: " .. tostring(obj:GetName()))

    local startPos = obj:GetActorLocation()
    print(string.format("Start Position: (%.2f, %.2f, %.2f)", startPos.x, startPos.y, startPos.z))

    print("This actor will rotate and move forward!")
end

function Tick(dt)
    elapsedTime = elapsedTime + dt

    -- Rotate around Z-axis
    local rotation = Vector(0, 0, rotationSpeed * dt)
    obj:AddActorLocalRotation(rotation)

    -- Move forward
    local forward = obj:GetActorForward()
    local movement = forward * moveSpeed * dt
    obj:AddActorWorldLocation(movement)

    -- Print position every second
    if math.floor(elapsedTime) > math.floor(elapsedTime - dt) then
        local pos = obj:GetActorLocation()
        print(string.format("[%.1fs] Position: (%.2f, %.2f, %.2f)",
            elapsedTime, pos.x, pos.y, pos.z))
    end
end

function EndPlay()
    print("=== [Script Ended] ===")
    print(string.format("Total time: %.2f seconds", elapsedTime))
end
