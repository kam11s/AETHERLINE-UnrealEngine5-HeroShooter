#include "ALArenaBuilder.h"
#include "ALGameInstance.h"
#include "ALTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/PointLightComponent.h"
AALArenaBuilder::AALArenaBuilder() { PrimaryActorTick.bCanEverTick = false; }
void AALArenaBuilder::BeginPlay()
{
	Super::BeginPlay();
	if (const UALGameInstance* GI = GetGameInstance<UALGameInstance>())
	{
		if (GI->SelectedPlaylist == EALPlaylist::BattleRoyale) { Size = 14000.f; WallH = 420.f; }
	}
	Box(FVector(0,0,-20), FVector(Size, Size, 20.f), FLinearColor(0.05f,0.07f,0.09f));
	const float H = Size;
	Box(FVector(H,0,WallH*0.5f), FVector(40.f,H,WallH), FLinearColor(0.08f,0.1f,0.14f));
	Box(FVector(-H,0,WallH*0.5f), FVector(40.f,H,WallH), FLinearColor(0.08f,0.1f,0.14f));
	Box(FVector(0,H,WallH*0.5f), FVector(H,40.f,WallH), FLinearColor(0.08f,0.1f,0.14f));
	Box(FVector(0,-H,WallH*0.5f), FVector(H,40.f,WallH), FLinearColor(0.08f,0.1f,0.14f));
	Box(FVector(0,0,40), FVector(420.f,420.f,20.f), FLinearColor(0.05f,0.55f,0.75f));
	Box(FVector(900,700,90), FVector(180.f,80.f,90.f), FLinearColor(0.15f,0.12f,0.08f));
	Box(FVector(-800,-500,90), FVector(160.f,90.f,90.f), FLinearColor(0.15f,0.12f,0.08f));
	Lights();
}
void AALArenaBuilder::Box(const FVector& Loc, const FVector& Ext, const FLinearColor& Color)
{
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube) return;
	Mesh->SetStaticMesh(Cube);
	Mesh->SetWorldLocation(Loc);
	Mesh->SetWorldScale3D(Ext / 50.f);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->RegisterComponent();
	if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* Dyn = UMaterialInstanceDynamic::Create(Mat, this))
		{
			Dyn->SetVectorParameterValue(TEXT("Color"), Color);
			Mesh->SetMaterial(0, Dyn);
		}
	}
}
void AALArenaBuilder::Lights()
{
	UWorld* W = GetWorld(); if (!W) return;
	FActorSpawnParameters P;
	if (ADirectionalLight* Sun = W->SpawnActor<ADirectionalLight>(FVector(0,0,1200), FRotator(-50.f,35.f,0.f), P))
	{
		if (UDirectionalLightComponent* C = Sun->FindComponentByClass<UDirectionalLightComponent>())
		{
			C->SetIntensity(6.0f); C->SetLightColor(FLinearColor(1.f,0.78f,0.55f)); C->SetCastShadows(true);
		}
	}
	if (ASkyLight* Sky = W->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, P))
	{
		if (USkyLightComponent* C = Sky->FindComponentByClass<USkyLightComponent>())
		{
			C->SetIntensity(1.2f); C->SetLightColor(FLinearColor(0.25f,0.45f,0.55f)); C->SetRealTimeCapture(true);
		}
	}
	UPointLightComponent* L = NewObject<UPointLightComponent>(this);
	L->SetWorldLocation(FVector(0,0,220)); L->SetIntensity(8000.f);
	L->SetLightColor(FLinearColor(0.18f,0.90f,1.0f)); L->SetAttenuationRadius(1400.f); L->RegisterComponent();
}
