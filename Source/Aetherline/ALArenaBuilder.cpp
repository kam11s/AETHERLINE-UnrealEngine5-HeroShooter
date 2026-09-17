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
		if (Name.Contains(TEXT("Floor")) || Name.Contains(TEXT("floor")))
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

	const FLinearColor Ink(0.03f, 0.04f, 0.05f);
	const FLinearColor Steel(0.07f, 0.09f, 0.12f);
	const FLinearColor Teal(0.08f, 0.55f, 0.72f);
	const FLinearColor Amber(0.55f, 0.28f, 0.08f);
	const FLinearColor Rust(0.16f, 0.10f, 0.06f);

	Box(FVector(0.f, 0.f, -30.f), FVector(Size, Size, 30.f), Ink);

	const float H = Size;
	Box(FVector( H, 0.f, WallH * 0.5f), FVector(50.f, H, WallH), Steel);
	Box(FVector(-H, 0.f, WallH * 0.5f), FVector(50.f, H, WallH), Steel);
	Box(FVector(0.f,  H, WallH * 0.5f), FVector(H, 50.f, WallH), Steel);
	Box(FVector(0.f, -H, WallH * 0.5f), FVector(H, 50.f, WallH), Steel);

	Box(FVector(0.f, 0.f, 18.f), FVector(520.f, 520.f, 18.f), Teal);

	const FVector Covers[] = {
		FVector( 900.f,  700.f, 90.f), FVector(-900.f, -650.f, 90.f),
		FVector( 700.f, -900.f, 90.f), FVector(-800.f,  850.f, 90.f),
		FVector(1400.f,  200.f, 80.f), FVector(-1400.f, -200.f, 80.f),
		FVector( 200.f, 1400.f, 80.f), FVector(-250.f,-1400.f, 80.f),
		FVector(1100.f,-1100.f, 70.f), FVector(-1100.f, 1100.f, 70.f)
	};
	for (int32 i = 0; i < 10; ++i)
	{
		const bool bAmber = (i % 3) == 0;
		Box(Covers[i], FVector(160.f + (i % 3) * 40.f, 70.f, 70.f + (i % 2) * 30.f), bAmber ? Amber : Rust);
	}

	Box(FVector(0.f, 1800.f, 140.f), FVector(80.f, 80.f, 140.f), Teal);
	Box(FVector(0.f,-1800.f, 140.f), FVector(80.f, 80.f, 140.f), Teal);
	Box(FVector(1800.f, 0.f, 140.f), FVector(80.f, 80.f, 140.f), Amber);
	Box(FVector(-1800.f, 0.f, 140.f), FVector(80.f, 80.f, 140.f), Amber);

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
			C->SetIntensity(4.5f);
			C->SetLightColor(FLinearColor(1.0f, 0.78f, 0.55f));
		}
	}

	UPointLightComponent* Center = NewObject<UPointLightComponent>(this);
	Center->SetWorldLocation(FVector(0.f, 0.f, 280.f));
	Center->SetIntensity(12000.f);
	Center->SetLightColor(FLinearColor(0.18f, 0.90f, 1.0f));
	Center->SetAttenuationRadius(2200.f);
	Center->RegisterComponent();

	UPointLightComponent* Warm = NewObject<UPointLightComponent>(this);
	Warm->SetWorldLocation(FVector(1200.f, 0.f, 260.f));
	Warm->SetIntensity(7000.f);
	Warm->SetLightColor(FLinearColor(0.89f, 0.45f, 0.12f));
	Warm->SetAttenuationRadius(1600.f);
	Warm->RegisterComponent();
}
