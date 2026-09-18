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
#include "Engine/SkyAtmosphere.h"
#include "Engine/VolumetricCloud.h"
#include "Engine/Scene.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "EngineUtils.h"

// Industrial night rig. The project runs with auto exposure OFF (DefaultEngine.ini), so
// every value below is absolute. Calibration point: the old 2.2 lux sun rendered as full
// daylight, so "night" means a key light a few stops under that and practicals that pool
// to roughly 1-3 lux on the ground directly beneath a fixture.
namespace ALNight
{
	// Moon / dim key. Steep enough to still rim the carbine, low enough for long shadows.
	constexpr float MoonLux = 0.35f;
	const FLinearColor MoonColor(0.42f, 0.60f, 0.85f);
	const FRotator MoonRotation(-32.f, 155.f, 0.f);

	// Sky: multiplier on the SkyAtmosphere sky pixels. This is what kills the blue daytime dome.
	const FLinearColor SkyLuminance(0.05f, 0.11f, 0.17f);
	constexpr float SkyLightIntensity = 1.1f;
	const FLinearColor SkyLightTint(0.45f, 0.72f, 0.90f);

	// Ground haze. Dark teal, hugging the floor, volumetric so practicals get halos.
	constexpr float FogDensity = 0.028f;
	constexpr float FogHeightFalloff = 0.30f;
	const FLinearColor FogColor(0.006f, 0.018f, 0.027f);
	const FLinearColor FogMoonGlow(0.018f, 0.032f, 0.055f);

	// Practical colours. Lights are saturated; fixture heads are lighter so they read as lit.
	const FLinearColor TealLight(0.18f, 0.90f, 1.00f);
	const FLinearColor TealHead(0.60f, 0.98f, 1.00f);
	const FLinearColor AmberLight(1.00f, 0.48f, 0.12f);
	const FLinearColor AmberHead(1.00f, 0.66f, 0.25f);
	const FLinearColor Steel(0.04f, 0.055f, 0.07f);

	constexpr float PostHeight = 350.f;
	constexpr float MastHeight = 460.f;
}

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
	const FLinearColor& Steel = ALNight::Steel;
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
	for (int32 i = 0; i < 12; ++i)
	{
		const bool bAmber = (i % 3) == 0;
		Box(Covers[i], FVector(170.f + (i % 3) * 30.f, 72.f, 72.f + (i % 2) * 28.f), bAmber ? Amber : Rust);
	}

	Box(FVector(0.f, 1900.f, 160.f), FVector(70.f, 70.f, 160.f), Teal);
	Box(FVector(0.f,-1900.f, 160.f), FVector(70.f, 70.f, 160.f), Teal);
	Box(FVector(1900.f, 0.f, 160.f), FVector(70.f, 70.f, 160.f), Amber);
	Box(FVector(-1900.f, 0.f, 160.f), FVector(70.f, 70.f, 160.f), Amber);

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
	NightSky();
	Practicals();
}

// Retunes whatever sky/sun/fog actors the loaded map already has (Template_Default or the
// local RavelinYard) so the default Play map turns to night without placing anything.
void AALArenaBuilder::NightSky()
{
	UWorld* W = GetWorld();
	if (!W) return;

	for (TActorIterator<ADirectionalLight> It(W); It; ++It)
	{
		if (UDirectionalLightComponent* C = It->FindComponentByClass<UDirectionalLightComponent>())
		{
			C->SetIntensity(ALNight::MoonLux);
			C->SetLightColor(ALNight::MoonColor);
			C->SetLightSourceAngle(0.6f);
			C->SetVolumetricScatteringIntensity(0.25f);
			It->SetActorRotation(ALNight::MoonRotation);
		}
	}
	for (TActorIterator<ASkyAtmosphere> It(W); It; ++It)
	{
		if (USkyAtmosphereComponent* C = It->FindComponentByClass<USkyAtmosphereComponent>())
		{
			C->SetSkyLuminanceFactor(ALNight::SkyLuminance);
		}
	}
	for (TActorIterator<AVolumetricCloud> It(W); It; ++It)
	{
		It->SetActorHiddenInGame(true);
	}
	for (TActorIterator<ASkyLight> It(W); It; ++It)
	{
		if (USkyLightComponent* C = It->FindComponentByClass<USkyLightComponent>())
		{
			C->SetIntensity(ALNight::SkyLightIntensity);
			C->SetLightColor(ALNight::SkyLightTint);
		}
	}
	for (TActorIterator<AExponentialHeightFog> It(W); It; ++It)
	{
		if (UExponentialHeightFogComponent* C = It->FindComponentByClass<UExponentialHeightFogComponent>())
		{
			C->SetFogDensity(ALNight::FogDensity);
			C->SetFogHeightFalloff(ALNight::FogHeightFalloff);
			C->SetFogInscatteringColor(ALNight::FogColor);
			C->SetDirectionalInscatteringColor(ALNight::FogMoonGlow);
			C->SetDirectionalInscatteringExponent(16.f);
			C->SetFogMaxOpacity(0.9f);
			C->SetVolumetricFog(true);
		}
	}
}

UPointLightComponent* AALArenaBuilder::Lamp(const FVector& Loc, const FLinearColor& Col, float Candelas, float Radius, float Scatter)
{
	UPointLightComponent* L = NewObject<UPointLightComponent>(this);
	L->SetWorldLocation(Loc);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(Candelas);
	L->SetLightColor(Col);
	L->SetAttenuationRadius(Radius);
	L->SetSourceRadius(10.f);
	L->SetCastShadows(false);
	L->SetVolumetricScatteringIntensity(Scatter);
	L->RegisterComponent();
	return L;
}

// Steel pole with a lit head. The lamp sits just under the head so its underside glows.
void AALArenaBuilder::LampPost(const FVector& Base, const FLinearColor& HeadColor, const FLinearColor& LightColor, float Candelas, float Radius)
{
	const float H = ALNight::PostHeight;
	Box(Base + FVector(0.f, 0.f, H * 0.5f), FVector(9.f, 9.f, H * 0.5f), ALNight::Steel);
	Box(Base + FVector(0.f, 0.f, H + 12.f), FVector(30.f, 30.f, 10.f), HeadColor);
	Lamp(Base + FVector(0.f, 0.f, H - 22.f), LightColor, Candelas, Radius, 1.2f);
}

// Tall mast with a narrow flood aimed at Target. Cones show up in the volumetric fog.
void AALArenaBuilder::SpotMast(const FVector& Base, const FVector& Target, const FLinearColor& Col, float Candelas, float Radius)
{
	const float H = ALNight::MastHeight;
	Box(Base + FVector(0.f, 0.f, H * 0.5f), FVector(12.f, 12.f, H * 0.5f), ALNight::Steel);
	Box(Base + FVector(0.f, 0.f, H + 14.f), FVector(34.f, 34.f, 12.f), ALNight::TealHead);

	const FVector Loc = Base + FVector(0.f, 0.f, H - 30.f);
	USpotLightComponent* S = NewObject<USpotLightComponent>(this);
	S->SetWorldLocation(Loc);
	S->SetWorldRotation(FRotationMatrix::MakeFromX((Target - Loc).GetSafeNormal()).Rotator());
	S->SetIntensityUnits(ELightUnits::Candelas);
	S->SetIntensity(Candelas);
	S->SetLightColor(Col);
	S->SetAttenuationRadius(Radius);
	S->SetInnerConeAngle(12.f);
	S->SetOuterConeAngle(30.f);
	S->SetSourceRadius(8.f);
	S->SetCastShadows(false);
	S->SetVolumetricScatteringIntensity(0.4f);
	S->RegisterComponent();
}

void AALArenaBuilder::Practicals()
{
	using namespace ALNight;

	// Capture plate: two teal posts on its diagonal corners.
	LampPost(FVector(-640.f,  640.f, 0.f), TealHead, TealLight, 26.f, 2200.f);
	LampPost(FVector( 640.f, -640.f, 0.f), TealHead, TealLight, 26.f, 2200.f);

	// Amber yard floods around the cover field.
	LampPost(FVector( 1250.f,  450.f, 0.f), AmberHead, AmberLight, 24.f, 1900.f);
	LampPost(FVector(-1250.f, -450.f, 0.f), AmberHead, AmberLight, 24.f, 1900.f);
	LampPost(FVector(  450.f,-1250.f, 0.f), AmberHead, AmberLight, 24.f, 1900.f);
	LampPost(FVector( -450.f, 1250.f, 0.f), AmberHead, AmberLight, 24.f, 1900.f);

	// Corner pylons (built in BeginPlay, top at z=320): lit cap plus a lamp above it.
	const FVector Pylons[] = { FVector(0.f, 1900.f, 0.f), FVector(0.f, -1900.f, 0.f), FVector(1900.f, 0.f, 0.f), FVector(-1900.f, 0.f, 0.f) };
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bTeal = i < 2;
		Box(Pylons[i] + FVector(0.f, 0.f, 328.f), FVector(84.f, 84.f, 8.f), bTeal ? TealHead : AmberHead);
		Lamp(Pylons[i] + FVector(0.f, 0.f, 372.f), bTeal ? TealLight : AmberLight, 18.f, 1600.f, 0.8f);
	}

	// Two cool teal accent floods raking across the plate from opposite corners.
	SpotMast(FVector( 2300.f, -2300.f, 0.f), FVector(0.f, 0.f, 40.f), TealLight, 900.f, 4200.f);
	SpotMast(FVector(-2300.f,  2300.f, 0.f), FVector(0.f, 0.f, 40.f), TealLight, 900.f, 4200.f);
}
