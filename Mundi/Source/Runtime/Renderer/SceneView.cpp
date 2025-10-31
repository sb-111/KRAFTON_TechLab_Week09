#include "pch.h"
#include "SceneView.h"
#include "CameraActor.h"
#include "FViewport.h"
#include "Frustum.h"
#include "math.h"

FSceneView::FSceneView(ACameraActor* InCamera, FViewport* InViewport, EViewModeIndex InViewMode)
{
    UCameraComponent* CamComp = InCamera->GetCameraComponent();
    if (!CamComp || !InViewport)
    {
        UE_LOG("[FSceneView::FSceneView()]: CamComp 또는 InViewport 가 없습니다.");
        return;
    }

    // --- 이 로직이 FSceneRenderer::PrepareView()에서 이동해 옴 ---

    float AspectRatio = 1.0f;
    if (InViewport->GetSizeY() > 0)
    {
        AspectRatio = (float)InViewport->GetSizeX() / (float)InViewport->GetSizeY();
    }

    ViewMatrix = CamComp->GetViewMatrix();
    ProjectionMatrix = CamComp->GetProjectionMatrix(AspectRatio, InViewport);
    ViewFrustum = CreateFrustumFromCamera(*CamComp, AspectRatio);
    ViewLocation = CamComp->GetWorldLocation();
    ViewDirection = CamComp->GetForward();
    ZNear = CamComp->GetNearClip();
    ZFar = CamComp->GetFarClip();

    ViewRect.MinX = InViewport->GetStartX();
    ViewRect.MinY = InViewport->GetStartY();
    ViewRect.MaxX = ViewRect.MinX + InViewport->GetSizeX();
    ViewRect.MaxY = ViewRect.MinY + InViewport->GetSizeY();

    // ViewMode는 World나 RenderSettings에서 가져와야 함 (임시)
    ViewMode = InViewMode;
    ProjectionMode = InCamera->GetCameraComponent()->GetProjectionMode();
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
