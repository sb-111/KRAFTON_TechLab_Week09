-- Template Lua Script
-- 이 파일은 새로운 스크립트를 생성할 때 복사
-- obj: 이 스크립트를 소유한 Actor 객체

function BeginPlay()
    local pcm = PlayerController:GetPlayerCameraManager()
    pcm:SetVignetteIntensity(0.8)
    pcm:SetVignetteRadius(0.5)
    pcm:EnableVignetting(true)

end

function Tick(dt)
    -- 매 프레임 호출
    -- dt: Delta Time (초 단위)

    -- 예시: Actor를 위로 이동
    local velocity = Vector(0, 0, 10) -- 10 units/sec
    obj:AddActorWorldLocation(velocity * dt)
end

function EndPlay()
    print("[EndPlay] Script ended")
end

-- 커스텀 이벤트 (Overlap)
function OnOverlap(OtherActor)
    print("[OnOverlap] Overlapped with: " .. tostring(OtherActor:GetName()))
end
