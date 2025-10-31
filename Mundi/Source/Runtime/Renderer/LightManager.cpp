#include "pch.h"
#include "LightManager.h"
#include "AmbientLightComponent.h"
#include "DirectionalLightComponent.h"
#include "SpotLightComponent.h"
#include "PointLightComponent.h"
#include "D3D11RHI.h"


FLightManager::~FLightManager()
{
	Release();
}
void FLightManager::Initialize(D3D11RHI* RHIDevice)
{
	if (!PointLightBuffer)
	{
		RHIDevice->CreateStructuredBuffer(sizeof(FPointLightInfo), NUM_POINT_LIGHT_MAX, nullptr, &PointLightBuffer);
		RHIDevice->CreateStructuredBufferSRV(PointLightBuffer, &PointLightBufferSRV);
	}
	if (!SpotLightBuffer)
	{
		RHIDevice->CreateStructuredBuffer(sizeof(FSpotLightInfo), NUM_SPOT_LIGHT_MAX, nullptr, &SpotLightBuffer);
		RHIDevice->CreateStructuredBufferSRV(SpotLightBuffer, &SpotLightBufferSRV);
	}
}
void FLightManager::Release()
{
	if (PointLightBuffer)
	{
		PointLightBuffer->Release();
		PointLightBuffer = nullptr;
	}
	if (PointLightBufferSRV)
	{
		PointLightBufferSRV->Release();
		PointLightBufferSRV = nullptr;
	}

	if (SpotLightBuffer)
	{
		SpotLightBuffer->Release();
		SpotLightBuffer = nullptr;
	}
	if (SpotLightBufferSRV)
	{
		SpotLightBufferSRV->Release();
		SpotLightBufferSRV = nullptr;
	}
}

void FLightManager::UpdateLightBuffer(D3D11RHI* RHIDevice)
{
	TArray<FPointLightInfo> PointLightInfoList;
	TArray<FSpotLightInfo> SpotLightInfoList;
	PointLightInfoList.Reserve(PointLightList.Num());
	SpotLightInfoList.Reserve(SpotLightList.Num());

	if (!PointLightBuffer)
	{
		Initialize(RHIDevice);
	}
	FLightBufferType LightBuffer{};

	if (AmbientLightList.Num() > 0)
	{
		if (AmbientLightList[0]->IsVisible()&&
			AmbientLightList[0]->GetOwner()->IsActorVisible())
		{
			LightBuffer.AmbientLight = AmbientLightList[0]->GetLightInfo();
		}
	}

	if (DirectionalLightList.Num() > 0)
	{
		if (DirectionalLightList[0]->IsVisible() &&
			DirectionalLightList[0]->GetOwner()->IsActorVisible())
		{
			LightBuffer.DirectionalLight = DirectionalLightList[0]->GetLightInfo();
		}
	}
	
	for (int Index = 0; Index < PointLightList.Num(); Index++)
	{
		if (PointLightList[Index]->IsVisible() &&
			PointLightList[Index]->GetOwner()->IsActorVisible())
		{
			PointLightInfoList.Add(PointLightList[Index]->GetLightInfo());
		}
	}

	for (int Index = 0; Index < SpotLightList.Num(); Index++)
	{
		if (SpotLightList[Index]->IsVisible() &&
			SpotLightList[Index]->GetOwner()->IsActorVisible())
		{
			SpotLightInfoList.Add(SpotLightList[Index]->GetLightInfo());
		}
	}

    RHIDevice->UpdateStructuredBuffer(PointLightBuffer, PointLightInfoList.data(), PointLightInfoList.Num()* sizeof(FPointLightInfo));
    RHIDevice->UpdateStructuredBuffer(SpotLightBuffer, SpotLightInfoList.data(), 
        SpotLightInfoList.Num() * sizeof(FSpotLightInfo));
	

    LightBuffer.PointLightCount = PointLightInfoList.Num();
    LightBuffer.SpotLightCount = SpotLightInfoList.Num();
    PointLightNum = LightBuffer.PointLightCount;
    SpotLightNum  = LightBuffer.SpotLightCount;

	//슬롯 재활용 하고싶을 시 if문 밖으로 빼서 매 프레임 세팅 해줘야함.
	RHIDevice->SetAndUpdateConstantBuffer(LightBuffer);
	
	ID3D11ShaderResourceView* SRVList[2]{ PointLightBufferSRV, SpotLightBufferSRV };

	//Gouraud shader 사용 여부로 아래 세팅 분기도 가능, 일단 둘다 바인딩함
	RHIDevice->GetDeviceContext()->PSSetShaderResources(3, 2, SRVList);
	RHIDevice->GetDeviceContext()->VSSetShaderResources(3, 2, SRVList);
	
}

void FLightManager::SetAllLightShadowInfoToDefault()
{
	for (int Index = 0; Index < PointLightList.Num(); Index++)
	{
		PointLightList[Index]->SetShadowValuesToDefault();
	}
	for (int Index = 0; Index < SpotLightList.Num(); Index++)
	{
		SpotLightList[Index]->SetShadowValuesToDefault();
	}
	for(int Index = 0; Index < AmbientLightList.Num(); Index++)
	{
		AmbientLightList[Index]->SetShadowValuesToDefault();
	}
	for(int Index = 0; Index < DirectionalLightList.Num(); Index++)
	{
		DirectionalLightList[Index]->SetShadowValuesToDefault();
	}
}

UDirectionalLightComponent* FLightManager::GetDirectionalLight()
{
	if (DirectionalLightList.Num() > 0)
	{
		if (DirectionalLightList[0]->IsVisible() &&
			DirectionalLightList[0]->GetOwner()->IsActorVisible())
		{
			return DirectionalLightList[0];
		}
	}
	return nullptr;
}


//TArray<FPointLightInfo>& FLightManager::GetPointLightInfoListNew()
//{
//	PointLightInfoList.clear();
//	for (int Index = 0; Index < PointLightList.Num(); Index++)
//	{
//		if (PointLightList[Index]->IsVisible() &&
//			PointLightList[Index]->GetOwner()->IsActorVisible())
//		{
//			PointLightInfoList.Add(PointLightList[Index]->GetLightInfo());
//		}
//	}
//	return PointLightInfoList;
//}
//
//TArray<FSpotLightInfo>& FLightManager::GetSpotLightInfoListNew()
//{
//	SpotLightInfoList.clear();
//	for (int Index = 0; Index < SpotLightList.Num(); Index++)
//	{
//		if (SpotLightList[Index]->IsVisible() &&
//			SpotLightList[Index]->GetOwner()->IsActorVisible())
//		{
//			SpotLightInfoList.Add(SpotLightList[Index]->GetLightInfo());
//		}
//	}
//	return SpotLightInfoList;
//}
//
//FDirectionalLightInfo& FLightManager::GetDirectionalLightInfoNew()
//{
//	DirectionalLightInfo = FDirectionalLightInfo();
//	if (DirectionalLightList.Num() > 0)
//	{
//		if (DirectionalLightList[0]->IsVisible() &&
//			DirectionalLightList[0]->GetOwner()->IsActorVisible())
//		{
//			DirectionalLightInfo = DirectionalLightList[0]->GetLightInfo();
//		}
//	}
//	return DirectionalLightInfo;
//}

void FLightManager::ClearAllLightList()
{
	AmbientLightList.clear();
	DirectionalLightList.clear();
	PointLightList.clear();
	SpotLightList.clear();

	//이미 레지스터된 라이트인지 확인하는 용도
	LightComponentList.clear();
	PointLightNum = 0;
	SpotLightNum = 0;
}

template<>
void FLightManager::RegisterLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent)
{
	if (LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Add(LightComponent);
	AmbientLightList.Add(LightComponent);
}
template<>
void FLightManager::RegisterLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent)
{
	if (LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Add(LightComponent);
	DirectionalLightList.Add(LightComponent);
}
template<>
void FLightManager::RegisterLight<UPointLightComponent>(UPointLightComponent* LightComponent)
{
	if (LightComponentList.Contains(LightComponent)||
		Cast<USpotLightComponent>(LightComponent))
	{
		return;
	}
	LightComponentList.Add(LightComponent);
	PointLightList.Add(LightComponent);
	PointLightNum++;
}
template<>
void FLightManager::RegisterLight<USpotLightComponent>(USpotLightComponent* LightComponent)
{
	if (LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Add(LightComponent);
	SpotLightList.Add(LightComponent);
	SpotLightNum++;
}


template<>
void FLightManager::DeRegisterLight<UAmbientLightComponent>(UAmbientLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Remove(LightComponent);
	AmbientLightList.Remove(LightComponent);
}
template<>
void FLightManager::DeRegisterLight<UDirectionalLightComponent>(UDirectionalLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Remove(LightComponent);
	DirectionalLightList.Remove(LightComponent);
}
template<>
void FLightManager::DeRegisterLight<UPointLightComponent>(UPointLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent) ||
		Cast<USpotLightComponent>(LightComponent))
	{
		return;
	}
	LightComponentList.Remove(LightComponent);
	PointLightList.Remove(LightComponent);
	PointLightNum--;
}
template<>
void FLightManager::DeRegisterLight<USpotLightComponent>(USpotLightComponent* LightComponent)
{
	if (!LightComponentList.Contains(LightComponent))
	{
		return;
	}
	LightComponentList.Remove(LightComponent);
	SpotLightList.Remove(LightComponent);
	SpotLightNum--;
}
