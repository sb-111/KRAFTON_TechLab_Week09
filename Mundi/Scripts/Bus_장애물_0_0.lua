-- Template Lua Script
-- 이 파일은 새로운 스크립트를 생성할 때 복사
-- obj: 이 스크립트를 소유한 Actor 객체

local minY = -45.0
local maxY = 45.0
local direction = 1.0
local moveSpeed = 40.0

local function ResetSpeed()
    moveSpeed = 20.0 + math.random() * 15.0
end

function BeginPlay()
    local shape = obj:GetShapeComponent()
    shape:RegisterBeginOverlapFunction(BeginOverlap)
    shape:RegisterEndOverlapFunction(EndOverlap)

    local location = obj:GetActorLocation()
    if location.y > maxY then
        direction = -1.0
    else
        direction = 1.0
    end

    ResetSpeed()
end

function Tick(dt)
    local location = obj:GetActorLocation()
    local baseX = location.x
    local baseZ = location.z
    local baseY = location.y

    if baseY > maxY then
        baseY = maxY
    elseif baseY < minY then
        baseY = minY
    end

    obj:SetActorLocation(Vector(baseX, baseY, baseZ))

    local current = obj:GetActorLocation()
    local newY = current.y + direction * moveSpeed * dt

    if newY > maxY then
        newY = maxY
        direction = -1.0
        ResetSpeed()
    elseif newY < minY then
        newY = minY
        direction = 1.0
        ResetSpeed()
    end

    obj:SetActorLocation(Vector(baseX, newY, baseZ))
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
