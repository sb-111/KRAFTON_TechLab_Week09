-- Template Lua Script
-- 이 파일은 새로운 스크립트를 생성할 때 복사
-- obj: 이 스크립트를 소유한 Actor 객체

local pcm = PlayerController:GetPlayerCameraManager()
function BeginPlay()
    pcm:EnableVignetting(true)


    -- 시작값과 끝값을 모두 명시적으로 지정
    -- pcm:StartVignetteBlend(StartIntensity, StartRadius, TargetIntensity, TargetRadius, Duration, bEnable)
    pcm:StartVignetteBlend(0.0, 0.0, 1.0, 0.6, 2.0, true)


    -- 방법 2: 시작값과 끝값을 명시적으로 지정
    -- pcm:StartLetterboxBlend(StartSize, TargetSize, 1.0, true)
    pcm:StartLetterboxBlend(0.0, 0.3, 1.0, true)
end

function Tick(dt)
    -- 매 프레임 호출
    -- dt: Delta Time (초 단위)

    -- 즉시 값을 설정하려면 (보간 없이):
    --pcm:SetVignetteIntensity(1.0)
    --pcm:SetVignetteRadius(0.7)
end

function EndPlay()
    print("[EndPlay] Script ended")
end

-- 커스텀 이벤트 (Overlap)
function OnOverlap(OtherActor)
    print("[OnOverlap] Overlapped with: " .. tostring(OtherActor:GetName()))
end
