-- Template Lua Script
-- 이 파일은 새로운 스크립트를 생성할 때 복사
-- obj: 이 스크립트를 소유한 Actor 객체

local PCM = PlayerController:GetPlayerCameraManager()
function BeginPlay(Other)
    local ShapeComponent = obj:GetShapeComponent()
    ShapeComponent:RegisterBeginOverlapFunction(BeginOverlap)
        
end

function Tick(dt)

end

function EndPlay()
    print("[EndPlay] Script ended")
end

-- 커스텀 이벤트 (Overlap)
function BeginOverlap(Other)
    if Other:GetOwner():GetPlayerComponent() ~= nil then
        -- 트랜지션 (2초 동안 카메라 전환)
        PlayerController:GetPlayerCameraManager():SetViewTarget(obj, 2)

        -- 슬로모션 (50% 속도로 감속)
        GWorld:SetGlobalTimeDilation(0.5)

        -- 레터박스 (1초 동안 서서히 나타남)
        PCM:SetLetterboxColor(Color(0.0, 0.0, 0.0, 1.0))
        PCM:StartLetterboxBlend(0.0, 0.2, 1.0, true)
    end
end

function OnOverlap(OtherActor)
    print("[OnOverlap] Overlapped with: " .. tostring(OtherActor:GetName()))
end
