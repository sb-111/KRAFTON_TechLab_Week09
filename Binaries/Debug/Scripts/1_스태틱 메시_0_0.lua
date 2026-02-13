 -- Input Binding Test Script

 -- 이동 및 회전 속도 설정
 local moveSpeed = 200.0
 local rotationSpeed = 90.0

 -- BeginPlay: 스크립트가 처음 시작될 때 한 번 호출됩니다.
 function BeginPlay()
     print("Input test script loaded for object: " .. obj:GetName())
 end

 -- Tick: 매 프레임 호출됩니다. (dt는 이전 프레임과의 시간 간격)
 function Tick(dt)
     -- ==================================
     --      키보드 이동 (누르고 있는 동안)
     -- ==================================

     -- W/S: 앞/뒤로 이동
     if Input:IsKeyDown(Keys.W) then
         local forwardVector = obj:GetActorForward()
         obj:AddActorWorldLocation(forwardVector * moveSpeed * dt)
         
     end
     if Input:IsKeyDown(Keys.S) then
         local forwardVector = obj:GetActorForward()
         obj:AddActorWorldLocation(forwardVector * -moveSpeed * dt)
     end
     
     -- A/D: 좌/우로 이동
     if Input:IsKeyDown(Keys.A) then
         local rightVector = obj:GetActorRight()
         obj:AddActorWorldLocation(rightVector * -moveSpeed * dt)
     end
     if Input:IsKeyDown(Keys.D) then
         local rightVector = obj:GetActorRight()
         obj:AddActorWorldLocation(rightVector * moveSpeed * dt)
     end
     
     -- ==================================
     --      키보드 회전 (누르고 있는 동안)
     -- ==================================
     
     -- Q/E: 좌/우로 회전 (Yaw)
     if Input:IsKeyDown(Keys.Q) then
         obj:AddActorWorldRotation(Vector.new(0, -rotationSpeed * dt, 0))
     end
     if Input:IsKeyDown(Keys.E) then
         obj:AddActorWorldRotation(Vector.new(0, rotationSpeed * dt, 0))
     end
     
     -- ==================================
     --      마우스 입력 (한 번 클릭)
     -- ==================================
     
     -- 왼쪽 마우스 버튼을 "처음 눌렀을 때" 마우스 좌표 출력
     if Input:IsMouseButtonPressed(0) then -- 0: 왼쪽, 1: 오른쪽, 2: 가운데
         local mousePos = Input:GetMousePosition()
         print("Left mouse button pressed at: " .. mousePos.x .. ", " .. mousePos.y)
     end
     
     -- 오른쪽 마우스 버튼을 "뗐을 때" 메시지 출력
     if Input:IsMouseButtonReleased(1) then
         print("Right mouse button was released!")
     end
 end

 -- EndPlay: 스크립트가 종료될 때 한 번 호출됩니다.
 function EndPlay()
     print("Input test script finished for object: " .. obj:GetName())
 end