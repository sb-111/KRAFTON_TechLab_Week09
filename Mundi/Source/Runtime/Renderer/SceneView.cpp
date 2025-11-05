#include "pch.h"
#include "SceneView.h"
#include "CameraActor.h"
#include "FViewport.h"
#include "Frustum.h"
#include "math.h"
#include "PlayerCameraManager.h"

FSceneView::FSceneView(ACameraActor* InCamera, FViewport* InViewport, EViewModeIndex InViewMode)
{
    // --- 이 로직이 FSceneRenderer::PrepareView()에서 이동해 옴 ---
    float AspectRatio = 1.0f;
    if (InViewport->GetSizeY() > 0)
    {
        AspectRatio = (float)InViewport->GetSizeX() / (float)InViewport->GetSizeY();
    }

    if (GWorld->bPie)
    {
        APlayerCameraManager* PlayerCameraManager = GWorld->GetPlayerController()->GetPlayerCameraManager();
        if (!PlayerCameraManager)
        {
            return;
        }
        const FMinimalViewInfo& ViewInfo = PlayerCameraManager->GetCameraViewInfo();

        ViewLocation = ViewInfo.Location;
        ZNear = ViewInfo.ZNear;
        ZFar = ViewInfo.ZFar;

        ViewMatrix = FMatrix::GetViewMatrix(FTransform(ViewLocation, ViewInfo.Rotation, FVector(1,1,1)));
        ProjectionMatrix = FMatrix::PerspectiveFovLH(DegreesToRadians(ViewInfo.Fov), AspectRatio, ZNear, ZFar);
        ViewFrustum = CreateFrustum(ViewInfo.Location, ViewInfo.Rotation, ZNear, ZFar, ViewInfo.Fov, AspectRatio);
        ViewDirection = ViewInfo.Rotation.RotateVector(FVector(1, 0, 0));

        ProjectionMode = ViewInfo.ProjectionMode;
    }
    else
    {
        UCameraComponent* CamComp = InCamera->GetCameraComponent();
        if (!CamComp || !InViewport)
        {
            UE_LOG("[FSceneView::FSceneView()]: CamComp 또는 InViewport 가 없습니다.");
            return;
        }

        ViewMatrix = CamComp->GetViewMatrix();
        ProjectionMatrix = CamComp->GetProjectionMatrix(AspectRatio, InViewport);
        ViewFrustum = CreateFrustumFromCamera(*CamComp, AspectRatio);
        ViewLocation = CamComp->GetWorldLocation();
        ViewDirection = CamComp->GetForward();
        ZNear = CamComp->GetNearClip();
        ZFar = CamComp->GetFarClip();

        ProjectionMode = InCamera->GetCameraComponent()->GetProjectionMode();
    }
    ViewRect.MinX = InViewport->GetStartX();
    ViewRect.MinY = InViewport->GetStartY();
    ViewRect.MaxX = ViewRect.MinX + InViewport->GetSizeX();
    ViewRect.MaxY = ViewRect.MinY + InViewport->GetSizeY();
    // ViewMode는 World나 RenderSettings에서 가져와야 함 (임시)
    ViewMode = InViewMode;
}

TArray<FVector> FSceneView::GetPointsOnCameraFrustum() const
{
    TArray<FVector4> PointsOnNDC;
    TArray<FVector> PointsOnFrustum;
    PointsOnFrustum.resize(8);
    PointsOnNDC.resize(8);

    PointsOnNDC[0] = FVector4(-1.0f, -1.0f ,0.0001f, 1.0f);
    PointsOnNDC[1] = FVector4(-1.0f,  1.0f, 0.0001f, 1.0f);
    PointsOnNDC[2] = FVector4( 1.0f, -1.0f, 0.0001f, 1.0f);
    PointsOnNDC[3] = FVector4( 1.0f,  1.0f, 0.0001f, 1.0f);
    PointsOnNDC[4] = FVector4(-1.0f, -1.0f, 1.0f, 1.0f);
    PointsOnNDC[5] = FVector4(-1.0f,  1.0f, 1.0f, 1.0f);
    PointsOnNDC[6] = FVector4( 1.0f, -1.0f, 1.0f, 1.0f);
    PointsOnNDC[7] = FVector4( 1.0f,  1.0f, 1.0f, 1.0f);

    for (int Index = 0; Index < 8; Index++)
    {
        FVector4 View = PointsOnNDC[Index] * ProjectionMatrix.InversePerspectiveProjection();
        View /= View.W;
        PointsOnFrustum[Index] = (View * ViewMatrix.InverseAffine()).ToVector3();
    }
    
    
    return PointsOnFrustum;
}
