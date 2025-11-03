-- Template Lua Script
-- 이 파일은 새로운 스크립트를 생성할 때 복사
-- obj: 이 스크립트를 소유한 Actor 객체

function BeginPlay()
        local shape = obj:GetShapeComponent()
        shape:RegisterBeginOverlapFunction(BeginOverlap)
        shape:RegisterEndOverlapFunction(EndOverlap)
    end

function Tick(dt)
    local velocity = Vector(-20, 0, 0);
    obj:AddActorWorldLocation(velocity * dt)
end

function EndPlay()
    print("[EndPlay] Script ended")
end

function BeginOverlap(otherShape)
    local owner = otherShape:GetOwner()
    if owner == nil then return end
    
    local playerComp = owner:GetPlayerComponent()
    if playerComp ~= nil then
        if type(OnPlayerDeath) == "function" then
            OnPlayerDeath()
        end
    end
end

function EndOverlap(OtherShape)

end
