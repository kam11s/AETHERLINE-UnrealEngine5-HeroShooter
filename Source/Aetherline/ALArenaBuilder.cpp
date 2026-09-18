#include "ALArenaBuilder.h"
#include "ALGameInstance.h"
#include "ALTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"

AALArenaBuilder::AALArenaBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AALArenaBuilder::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const FString Name = It->GetActorNameOrLabel();
		if (Name.Contains(TEXT("Floor")) || Name.Contains(TEXT("floor")) || Name.Contains(TEXT("SkySphere")))
		{
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
		}
	}

	if (const UALGameInstance* GI = GetGameInstance<UALGameInstance>())
	{
		if (GI->SelectedPlaylist == EALPlaylist::BattleRoyale)
		{
			Size = 14000.f;
			WallH = 420.f;
		}
	}

	const FLinearColor Ink(0.012f, 0.016f, 0.02f);
	const FLinearColor Steel(0.04f, 0.055f, 0.07f);
	const FLinearColor Teal(0.05f, 0.62f, 0.78f);
	const FLinearColor Amber(0.72f, 0.32f, 0.07f);
	const FLinearColor Rust(0.12f, 0.07f, 0.04f);

	Box(FVector(0.f, 0.f, -40.f), FVector(Size, Size, 40.f), Ink);

	const float H = Size;
	Box(FVector( H, 0.f, WallH * 0.5f), FVector(60.f, H, WallH), Steel);
	Box(FVector(-H, 0.f, WallH * 0.5f), FVector(60.f, H, WallH), Steel);
	Box(FVector(0.f,  H, WallH * 0.5f), FVector(H, 60.f, WallH), Steel);
	Box(FVector(0.f, -H, WallH * 0.5f), FVector(H, 60.f, WallH), Steel);

	Box(FVector(0.f, 0.f, 16.f), FVector(560.f, 560.f, 16.f), Teal);

	const FVector Covers[] = {
		FVector( 900.f,  700.f, 90.f), FVector(-900.f, -650.f, 90.f),
		FVector( 700.f, -900.f, 90.f), FVector(-800.f,  850.f, 90.f),
		FVector(1400.f,  200.f, 80.f), FVector(-1400.f, -200.f, 80.f),
		FVector( 200.f, 1400.f, 80.f), FVector(-250.f,-1400.f, 80.f),
		FVector(1100.f,-1100.f, 70.f), FVector(-1100.f, 1100.f, 70.f),
		FVector( 450.f,  300.f, 55.f), FVector(-480.f, -320.f, 55.f)
	};
	CoverPoints.Reset();
	for (int32 i = 0; i < 12; ++i)
	{
		const bool bAmber = (i % 3) == 0;
		Box(Covers[i], FVector(170.f + (i % 3) * 30.f, 72.f, 72.f + (i % 2) * 28.f), bAmber ? Amber : Rust);
		CoverPoints.Add(Covers[i]);
	}

	const FVector Pylons[] = {
		FVector(0.f, 1900.f, 160.f), FVector(0.f, -1900.f, 160.f),
		FVector(1900.f, 0.f, 160.f), FVector(-1900.f, 0.f, 160.f)
	};
	for (int32 i = 0; i < 4; ++i)
	{
		Box(Pylons[i], FVector(70.f, 70.f, 160.f), (i < 2) ? Teal : Amber);
		CoverPoints.Add(Pylons[i]);
	}

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
	UWorld* W = GetWorld();
	if (!W) return;

	for (TActorIterator<ADirectionalLight> It(W); It; ++It)
	{
		if (UDirectionalLightComponent* C = It->FindComponentByClass<UDirectionalLightComponent>())
		{
			C->SetIntensity(2.2f);
			C->SetLightColor(FLinearColor(0.55f, 0.72f, 0.85f));
			It->SetActorRotation(FRotator(-35.f, 40.f, 0.f));
		}
	}
	for (TActorIterator<ASkyLight> It(W); It; ++It)
	{
		if (USkyLightComponent* C = It->FindComponentByClass<USkyLightComponent>())
		{
			C->SetIntensity(0.35f);
			C->SetLightColor(FLinearColor(0.15f, 0.28f, 0.38f));
		}
	}
	for (TActorIterator<AExponentialHeightFog> It(W); It; ++It)
	{
		if (UExponentialHeightFogComponent* C = It->FindComponentByClass<UExponentialHeightFogComponent>())
		{
			C->SetFogDensity(0.06f);
			C->SetFogHeightFalloff(0.18f);
			C->SetFogInscatteringColor(FLinearColor(0.04f, 0.10f, 0.14f));
			C->SetVolumetricFog(true);
		}
	}

	auto Lamp = [this](FVector Loc, FLinearColor Col, float Intensity, float Radius)
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetWorldLocation(Loc);
		L->SetIntensity(Intensity);
		L->SetLightColor(Col);
		L->SetAttenuationRadius(Radius);
		L->SetCastShadows(false);
		L->RegisterComponent();
	};
	Lamp(FVector(0.f, 0.f, 260.f), FLinearColor(0.18f, 0.90f, 1.0f), 16000.f, 2400.f);
	Lamp(FVector(1200.f, 400.f, 220.f), FLinearColor(0.89f, 0.40f, 0.10f), 9000.f, 1800.f);
	Lamp(FVector(-1200.f, -400.f, 220.f), FLinearColor(0.89f, 0.40f, 0.10f), 9000.f, 1800.f);
	Lamp(FVector(0.f, 1600.f, 300.f), FLinearColor(0.18f, 0.90f, 1.0f), 7000.f, 1600.f);
}
