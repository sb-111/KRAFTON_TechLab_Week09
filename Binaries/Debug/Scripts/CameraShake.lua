-- Camera Shake Example Script
-- 카메라 쉐이크 예제 스크립트
--
-- StartCameraShake 사용법:
-- 1. 기본 (Frequency=10, Pattern=SineWave):
--    pcm:StartCameraShake(Duration, LocAmp, RotAmp, FovAmp)
--
-- 2. Frequency 지정 (Pattern=SineWave):
--    pcm:StartCameraShake(Duration, LocAmp, RotAmp, FovAmp, Frequency)
--
-- 3. Frequency + Pattern 지정:
--    pcm:StartCameraShake(Duration, LocAmp, RotAmp, FovAmp, Frequency, ShakePattern.PerlinNoise)

local pcm = PlayerController:GetPlayerCameraManager()

function BeginPlay()
    print("[CameraShake] Script started")
    print("[CameraShake] Available patterns: SineWave=" .. ShakePattern.SineWave .. ", PerlinNoise=" .. ShakePattern.PerlinNoise)
end

function Tick(dt)
    -- 매 프레임 호출

    -- =============================================
    -- 기본 SineWave Shake (Frequency=10, 기본값)
    -- =============================================
    if Input:IsKeyPressed(Keys.Space) then
        print("[CameraShake] Space - Basic SineWave shake (freq=10)")
        pcm:StartCameraShake(
            0.5,                        -- 0.5초 지속
            Vector(15, 15, 8),         -- 위치 흔들림
            Vector(2, 2, 1),           -- 회전 흔들림 (도 단위)
            5.0                        -- FOV 흔들림
        )
    end

    -- =============================================
    -- 커스텀 Frequency SineWave (빠른 진동)
    -- =============================================
    if Input:IsKeyPressed(Keys.E) then
        print("[CameraShake] E - Fast SineWave shake (freq=20)")
        pcm:StartCameraShake(
            1.0,                        -- 1초 지속
            Vector(5, 5, 3),           -- 작은 위치 흔들림
            Vector(0.5, 0.5, 0.2),     -- 작은 회전 흔들림
            2.0,                       -- FOV 흔들림
            20.0                       -- Frequency: 빠른 진동
        )
    end

    -- =============================================
    -- Perlin Noise Shake (자연스러운 흔들림)
    -- =============================================
    if Input:IsKeyPressed(Keys.Q) then
        print("[CameraShake] Q - Perlin Noise shake (freq=8)")
        pcm:StartCameraShake(
            2.0,                        -- 2초 지속
            Vector(10, 10, 5),         -- 위치 흔들림
            Vector(1.5, 1.5, 0.8),     -- 회전 흔들림
            3.0,                       -- FOV 흔들림
            8.0,                       -- Frequency: 느린 변화
            ShakePattern.PerlinNoise   -- Pattern: Perlin Noise
        )
    end

    -- =============================================
    -- 걷기 효과 (Perlin Noise, 미세한 흔들림)
    -- =============================================
    if Input:IsKeyPressed(Keys.W) then
        print("[CameraShake] W - Walking shake (PerlinNoise, freq=5)")
        pcm:StartCameraShake(
            10.0,                      -- 10초 지속
            Vector(1, 1, 2),           -- 미세한 위치 흔들림
            Vector(0.1, 0.1, 0.05),    -- 미세한 회전 흔들림
            0.5,                       -- 미세한 FOV 흔들림
            5.0,                       -- Frequency: 느린 변화
            ShakePattern.PerlinNoise   -- Pattern: 자연스러운 노이즈
        )
    end

    -- =============================================
    -- 폭발 효과 (SineWave, 강한 흔들림)
    -- =============================================
    if Input:IsKeyPressed(Keys.R) then
        print("[CameraShake] R - EXPLOSION! (SineWave, freq=15)")
        pcm:StartCameraShake(
            0.8,                        -- 0.8초 지속
            Vector(30, 30, 15),        -- 강한 위치 흔들림
            Vector(3, 3, 1.5),         -- 강한 회전 흔들림
            8.0,                       -- 강한 FOV 흔들림
            15.0,                      -- Frequency: 중간 속도
            ShakePattern.SineWave      -- Pattern: 사인파
        )
    end

    -- =============================================
    -- 지진 효과 (Perlin Noise, 긴 지속 시간)
    -- =============================================
    if Input:IsKeyPressed(Keys.T) then
        print("[CameraShake] T - Earthquake! (PerlinNoise, freq=3)")
        pcm:StartCameraShake(
            5.0,                        -- 5초 지속
            Vector(8, 8, 4),           -- 중간 위치 흔들림
            Vector(1, 1, 0.5),         -- 중간 회전 흔들림
            2.0,                       -- FOV 흔들림
            3.0,                       -- Frequency: 느린 변화
            ShakePattern.PerlinNoise   -- Pattern: 불규칙한 노이즈
        )
    end
end

function EndPlay()
    print("[CameraShake] Script ended")
end

-- 커스텀 이벤트 (Overlap)
function OnOverlap(OtherActor)
    print("[CameraShake] Overlapped with: " .. tostring(OtherActor:GetName()))

    -- 충돌 시 쉐이크 효과 (Perlin Noise로 자연스럽게)
    pcm:StartCameraShake(
        0.4,                        -- 0.4초 지속
        Vector(12, 12, 6),         -- 위치 흔들림
        Vector(1.2, 1.2, 0.6),     -- 회전 흔들림
        3.0,                       -- FOV 흔들림
        12.0,                      -- Frequency
        ShakePattern.PerlinNoise   -- Pattern: 자연스러운 충돌
    )
end
