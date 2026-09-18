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
#include "Engine/Scene.h"
#include "GameFramework/PlayerStart.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
// UE 5.8: ASkyAtmosphere / AVolumetricCloud actors live in the component headers.
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "EngineUtils.h"

// Industrial yard greybox. Sizes are half-extents (Box() takes half-extents) and are tuned
// against the hero capsule (176 tall, eye at 152, jump apex ~138, step-up 45):
//   crate 110       vaultable in one jump, two crates (220) reachable from the first
//   barrier 136     chest cover, eye clears it by ~16 so you can shoot over it
//   container 260   full block, taller than the hero
//   catwalk 278     deck top; underside at 262 so the hero walks under with room
//   pipe rack       lowest pipe underside at 222, walk-under
namespace ALYard
{
	const FLinearColor Rust(0.34f, 0.14f, 0.06f);
	const FLinearColor Ochre(0.80f, 0.42f, 0.09f);
	const FLinearColor DeepTeal(0.05f, 0.36f, 0.44f);
	const FLinearColor Gunmetal(0.08f, 0.10f, 0.12f);
	const FLinearColor Concrete(0.30f, 0.31f, 0.30f);
	const FLinearColor Hazard(1.00f, 0.68f, 0.10f);
	const FLinearColor Grate(0.14f, 0.18f, 0.20f);

	// 20 ft shipping container: 1220 x 244 x 260.
	const FVector ContainerExt(610.f, 122.f, 130.f);
	constexpr float CrateHalf = 55.f;
	const FVector BarrierExt(150.f, 30.f, 62.f);
	constexpr float BarrierCapHalf = 6.f;
	constexpr float DeckTop = 278.f;
	constexpr float DeckHalfThick = 8.f;
	const FVector DeckExt(350.f, 110.f, DeckHalfThick);
	constexpr float RampRun = 480.f;
	constexpr float RampHalfWidth = 100.f;
	constexpr float PipePostHalf = 150.f;
	constexpr float PipeSpan = 340.f;
	const FVector PillarExt(45.f, 45.f, 200.f);

	// Nothing is built inside these discs so the spawn plate and PlayerStart stay open.
	constexpr float CenterClear = 600.f;
	constexpr float StartClear = 420.f;
	constexpr float RingRadius = 3000.f;
}

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

	Box(FVector(0.f, 0.f, -40.f), FVector(Size, Size, 40.f), Ink);

	const float H = Size;
	Box(FVector( H, 0.f, WallH * 0.5f), FVector(60.f, H, WallH), Steel);
	Box(FVector(-H, 0.f, WallH * 0.5f), FVector(60.f, H, WallH), Steel);
	Box(FVector(0.f,  H, WallH * 0.5f), FVector(H, 60.f, WallH), Steel);
	Box(FVector(0.f, -H, WallH * 0.5f), FVector(H, 60.f, WallH), Steel);

	Box(FVector(0.f, 0.f, 16.f), FVector(560.f, 560.f, 16.f), Teal);

	GatherKeepClear();
	Yard();

	Box(FVector(0.f, 1900.f, 160.f), FVector(70.f, 70.f, 160.f), Teal);
	Box(FVector(0.f,-1900.f, 160.f), FVector(70.f, 70.f, 160.f), Teal);
	Box(FVector(1900.f, 0.f, 160.f), FVector(70.f, 70.f, 160.f), Amber);
	Box(FVector(-1900.f, 0.f, 160.f), FVector(70.f, 70.f, 160.f), Amber);

	Lights();
}

void AALArenaBuilder::Box(const FVector& Loc, const FVector& Ext, const FLinearColor& Color)
{
	Box(Loc, Ext, FRotator::ZeroRotator, Color);
}

void AALArenaBuilder::Box(const FVector& Loc, const FVector& Ext, const FRotator& Rot, const FLinearColor& Color)
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube) return;
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	Mesh->SetStaticMesh(Cube);
	Mesh->SetWorldLocation(Loc);
	Mesh->SetWorldRotation(Rot);
	Mesh->SetWorldScale3D(Ext / 50.f);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->RegisterComponent();

	// One dynamic material per colour; the yard is a few hundred cubes in ~10 colours.
	const uint32 Key = Color.ToFColor(false).DWColor();
	UMaterialInstanceDynamic* Dyn = MaterialCache.FindRef(Key);
	if (!Dyn)
	{
		if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			Dyn = UMaterialInstanceDynamic::Create(Mat, this);
			if (Dyn)
			{
				Dyn->SetVectorParameterValue(TEXT("Color"), Color);
				MaterialCache.Add(Key, Dyn);
			}
		}
	}
	if (Dyn) Mesh->SetMaterial(0, Dyn);
}

// ---------------------------------------------------------------------------------------------
// Yard
// ---------------------------------------------------------------------------------------------

void AALArenaBuilder::GatherKeepClear()
{
	KeepClear.Reset();
	KeepClearRadius.Reset();
	KeepClear.Add(FVector::ZeroVector);
	KeepClearRadius.Add(ALYard::CenterClear);
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		KeepClear.Add(It->GetActorLocation());
		KeepClearRadius.Add(ALYard::StartClear);
	}
}

bool AALArenaBuilder::Fits(const FVector& Center, float FootprintRadius) const
{
	for (int32 i = 0; i < KeepClear.Num(); ++i)
	{
		if (FVector::Dist2D(Center, KeepClear[i]) < KeepClearRadius[i] + FootprintRadius) return false;
	}
	return true;
}

// Layout notes: the lamp posts, pylons and spot masts from Practicals() sit at (+-640,-+640),
// (+-1250,+-450)/(+-450,+-1250), (0,+-1900)/(+-1900,0) and (+-2300,-+2300); every footprint
// below was placed to leave those free. Distances from the origin grow ring by ring so the
// spawn view stacks silhouettes: barriers/crates near, containers and pipe racks mid, catwalks
// and pillars behind them, then the container ring shortening the sightline to the walls.
void AALArenaBuilder::Yard()
{
	using namespace ALYard;

	// Four container blocks: two on the ground, a third bridging them one tier up and
	// overhanging by 200 so the stack reads as a real yard, not a grid.
	Container(FVector( 1500.f,  1000.f, 0.f),   0.f, Ochre, 1);
	Container(FVector( 1500.f,  1270.f, 0.f),   0.f, DeepTeal, 1);
	Container(FVector( 1700.f,  1135.f, 260.f), 0.f, Rust, 1);
	Container(FVector(-1500.f, -1000.f, 0.f),   0.f, DeepTeal, 1);
	Container(FVector(-1500.f, -1270.f, 0.f),   0.f, Ochre, 1);
	Container(FVector(-1700.f, -1135.f, 260.f), 0.f, Rust, 1);
	Container(FVector(-1000.f,  1500.f, 0.f),  90.f, Ochre, 1);
	Container(FVector(-1270.f,  1500.f, 0.f),  90.f, Rust, 1);
	Container(FVector(-1135.f,  1700.f, 260.f), 90.f, DeepTeal, 1);
	Container(FVector( 1000.f, -1500.f, 0.f),  90.f, DeepTeal, 1);
	Container(FVector( 1270.f, -1500.f, 0.f),  90.f, Rust, 1);
	Container(FVector( 1135.f, -1700.f, 260.f), 90.f, Ochre, 1);

	// Two-high diagonal stacks in the corners the spot masts do not use.
	Container(FVector( 2300.f,  2300.f, 0.f), 45.f, Ochre, 2);
	Container(FVector(-2300.f, -2300.f, 0.f), 45.f, DeepTeal, 2);

	// Chest-high barriers in pairs along the four lanes out of the plate.
	Barrier(FVector(-200.f,  1000.f, 0.f),  0.f); Barrier(FVector( 200.f,  1000.f, 0.f),  0.f);
	Barrier(FVector(-200.f, -1000.f, 0.f),  0.f); Barrier(FVector( 200.f, -1000.f, 0.f),  0.f);
	Barrier(FVector( 1000.f, -200.f, 0.f), 90.f); Barrier(FVector( 1000.f,  200.f, 0.f), 90.f);
	Barrier(FVector(-1000.f, -200.f, 0.f), 90.f); Barrier(FVector(-1000.f,  200.f, 0.f), 90.f);
	Barrier(FVector( 1580.f,  600.f, 0.f),  0.f); Barrier(FVector( 1920.f,  600.f, 0.f),  0.f);
	Barrier(FVector(-1580.f, -600.f, 0.f),  0.f); Barrier(FVector(-1920.f, -600.f, 0.f),  0.f);
	Barrier(FVector( 350.f,  1620.f, 0.f), 90.f); Barrier(FVector( 350.f,  1960.f, 0.f), 90.f);
	Barrier(FVector(-350.f, -1620.f, 0.f), 90.f); Barrier(FVector(-350.f, -1960.f, 0.f), 90.f);

	// Crate stacks: vaultable singles near the plate, bigger piles further out.
	CrateStack(FVector( 780.f,  240.f, 0.f),  12.f, 3);
	CrateStack(FVector(-780.f, -240.f, 0.f), -20.f, 3);
	CrateStack(FVector( 240.f, -780.f, 0.f),  35.f, 2);
	CrateStack(FVector(-240.f,  780.f, 0.f), -50.f, 2);
	CrateStack(FVector( 1900.f,  1560.f, 0.f),  8.f, 5);
	CrateStack(FVector(-1900.f, -1560.f, 0.f), 70.f, 5);
	CrateStack(FVector( 1560.f, -1900.f, 0.f), -30.f, 4);
	CrateStack(FVector(-1560.f,  1900.f, 0.f), 55.f, 4);
	CrateStack(FVector( 2600.f,     0.f, 0.f), 15.f, 5);
	CrateStack(FVector(-2600.f,     0.f, 0.f), 40.f, 5);
	CrateStack(FVector(    0.f,  2600.f, 0.f), 25.f, 4);
	CrateStack(FVector(    0.f, -2600.f, 0.f), 65.f, 4);

	// Pipe racks bridge each lane at walk-under height.
	PipeRack(FVector(    0.f,  1400.f, 0.f),  0.f);
	PipeRack(FVector(    0.f, -1400.f, 0.f),  0.f);
	PipeRack(FVector( 1400.f,     0.f, 0.f), 90.f);
	PipeRack(FVector(-1400.f,     0.f, 0.f), 90.f);

	// Catwalk decks with a ramp running back toward the plate.
	Catwalk(FVector( 2000.f,  -700.f, 0.f),  0.f, -1.f);
	Catwalk(FVector(-2000.f,   700.f, 0.f),  0.f,  1.f);
	Catwalk(FVector(  700.f,  2000.f, 0.f), 90.f, -1.f);
	Catwalk(FVector( -700.f, -2000.f, 0.f), 90.f,  1.f);

	// Concrete pillars behind the cover field, before the ring.
	Pillar(FVector( 2500.f,  1300.f, 0.f)); Pillar(FVector( 2500.f, -1300.f, 0.f));
	Pillar(FVector(-2500.f,  1300.f, 0.f)); Pillar(FVector(-2500.f, -1300.f, 0.f));
	Pillar(FVector( 1300.f,  2500.f, 0.f)); Pillar(FVector(-1300.f,  2500.f, 0.f));
	Pillar(FVector( 1300.f, -2500.f, 0.f)); Pillar(FVector(-1300.f, -2500.f, 0.f));

	ContainerRing();
}

// Container body plus four gunmetal corner posts per tier. Base.Z is the underside, so a
// stacked unit is just another call with Base.Z = 260.
void AALArenaBuilder::Container(const FVector& Base, float Yaw, const FLinearColor& Color, int32 Tiers)
{
	using namespace ALYard;
	if (!Fits(Base, ContainerExt.Size2D())) return;
	const FRotator Rot(0.f, Yaw, 0.f);
	const FLinearColor Alt = (Color == Ochre) ? DeepTeal : Rust;
	for (int32 T = 0; T < Tiers; ++T)
	{
		const FVector C = Base + FVector(0.f, 0.f, ContainerExt.Z * (2.f * T + 1.f));
		Box(C, ContainerExt, Rot, (T % 2 == 0) ? Color : Alt);
		for (int32 SX = -1; SX <= 1; SX += 2)
		{
			for (int32 SY = -1; SY <= 1; SY += 2)
			{
				const FVector Local(SX * (ContainerExt.X - 10.f), SY * (ContainerExt.Y + 4.f), 0.f);
				Box(C + Rot.RotateVector(Local), FVector(12.f, 6.f, ContainerExt.Z), Rot, Gunmetal);
			}
		}
		// Thin hazard band just below the roof line on both long sides.
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			const FVector Local(0.f, SY * (ContainerExt.Y + 1.f), ContainerExt.Z - 28.f);
			Box(C + Rot.RotateVector(Local), FVector(ContainerExt.X - 60.f, 2.f, 6.f), Rot, Hazard);
		}
	}
}

// A broken ring of containers at RingRadius so the far walls are never the first thing you see.
void AALArenaBuilder::ContainerRing()
{
	using namespace ALYard;
	const float R = FMath::Min(RingRadius, Size - 700.f);
	const float Along[] = { -2800.f, -1400.f, 0.f, 1400.f, 2800.f };
	for (int32 i = 0; i < 5; ++i)
	{
		const int32 Tiers = (i % 2 == 1) ? 2 : 1;
		Container(FVector( R, Along[i], 0.f), 90.f, (i % 2 == 0) ? DeepTeal : Ochre, Tiers);
		Container(FVector(-R, Along[i], 0.f), 90.f, (i % 2 == 0) ? Ochre : Rust, Tiers);
	}
	const float Across[] = { -2000.f, -680.f, 680.f, 2000.f };
	for (int32 i = 0; i < 4; ++i)
	{
		const int32 Tiers = (i % 2 == 0) ? 2 : 1;
		Container(FVector(Across[i],  R, 0.f), 0.f, (i % 2 == 0) ? Rust : DeepTeal, Tiers);
		Container(FVector(Across[i], -R, 0.f), 0.f, (i % 2 == 0) ? Ochre : DeepTeal, Tiers);
	}
}

// Up to a 2x2 of 110 crates with a fifth on top. Each crate gets its own small yaw so the pile
// looks dropped rather than placed.
void AALArenaBuilder::CrateStack(const FVector& Base, float Yaw, int32 Count)
{
	using namespace ALYard;
	if (!Fits(Base, 130.f)) return;
	const FRotator Rot(0.f, Yaw, 0.f);
	const float S = CrateHalf + 3.f;
	const FVector Slots[] = {
		FVector(-S, -S, CrateHalf), FVector(S, -S, CrateHalf), FVector(-S, S, CrateHalf), FVector(S, S, CrateHalf),
		FVector(0.f, 0.f, CrateHalf * 3.f)
	};
	const FLinearColor Colors[] = { Ochre, Rust, Concrete, DeepTeal, Ochre };
	const float Jitter[] = { 4.f, -3.f, 6.f, -5.f, 9.f };
	const int32 N = FMath::Clamp(Count, 1, 5);
	for (int32 i = 0; i < N; ++i)
	{
		Box(Base + Rot.RotateVector(Slots[i]), FVector(CrateHalf), FRotator(0.f, Yaw + Jitter[i], 0.f), Colors[i]);
	}
}

// Concrete block with an amber hazard cap: 136 total, chest height for a 152 eye.
void AALArenaBuilder::Barrier(const FVector& Base, float Yaw)
{
	using namespace ALYard;
	if (!Fits(Base, BarrierExt.Size2D())) return;
	const FRotator Rot(0.f, Yaw, 0.f);
	Box(Base + FVector(0.f, 0.f, BarrierExt.Z), BarrierExt, Rot, Concrete);
	Box(Base + FVector(0.f, 0.f, BarrierExt.Z * 2.f + BarrierCapHalf), FVector(BarrierExt.X + 2.f, BarrierExt.Y + 2.f, BarrierCapHalf), Rot, Hazard);
}

// Two gunmetal posts 600 apart carrying five pipes on two tiers; lowest pipe underside at 222.
void AALArenaBuilder::PipeRack(const FVector& Center, float Yaw)
{
	using namespace ALYard;
	if (!Fits(Center, PipeSpan + 20.f)) return;
	const FRotator Rot(0.f, Yaw, 0.f);
	for (int32 SX = -1; SX <= 1; SX += 2)
	{
		const FVector PostBase = Center + Rot.RotateVector(FVector(SX * 300.f, 0.f, 0.f));
		Box(PostBase + FVector(0.f, 0.f, PipePostHalf), FVector(12.f, 12.f, PipePostHalf), Rot, Gunmetal);
		Box(PostBase + FVector(0.f, 0.f, 100.f), FVector(13.f, 13.f, 8.f), Rot, Hazard);
		Box(PostBase + FVector(0.f, 0.f, 216.f), FVector(14.f, 60.f, 6.f), Rot, Gunmetal);
		Box(PostBase + FVector(0.f, 0.f, 264.f), FVector(14.f, 40.f, 6.f), Rot, Gunmetal);
	}
	const float LowY[] = { -36.f, 0.f, 36.f };
	for (int32 i = 0; i < 3; ++i)
	{
		Box(Center + Rot.RotateVector(FVector(0.f, LowY[i], 236.f)), FVector(PipeSpan, 14.f, 14.f), Rot, (i == 1) ? DeepTeal : Rust);
	}
	const float HighY[] = { -18.f, 18.f };
	for (int32 i = 0; i < 2; ++i)
	{
		Box(Center + Rot.RotateVector(FVector(0.f, HighY[i], 284.f)), FVector(PipeSpan, 14.f, 14.f), Rot, (i == 0) ? Rust : Gunmetal);
	}
}

// Grate deck at 278 on four legs, amber toe strips, two rails per side, and a ramp off one end.
// RampDir is +1/-1 along the deck's local X.
void AALArenaBuilder::Catwalk(const FVector& Center, float Yaw, float RampDir)
{
	using namespace ALYard;
	const FRotator Rot(0.f, Yaw, 0.f);
	const float Dir = (RampDir < 0.f) ? -1.f : 1.f;
	const FVector RampTop = Center + Rot.RotateVector(FVector(Dir * DeckExt.X, 0.f, DeckTop - DeckHalfThick));
	const FVector RampBottom = Center + Rot.RotateVector(FVector(Dir * (DeckExt.X + RampRun), 0.f, 0.f));
	if (!Fits(Center, DeckExt.Size2D()) || !Fits((RampTop + RampBottom) * 0.5f, RampRun * 0.5f + RampHalfWidth)) return;

	const float DeckCenterZ = DeckTop - DeckHalfThick;
	Box(Center + FVector(0.f, 0.f, DeckCenterZ), DeckExt, Rot, Grate);

	const float LegHalf = (DeckTop - DeckHalfThick * 2.f) * 0.5f;
	for (int32 SX = -1; SX <= 1; SX += 2)
	{
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			const FVector Local(SX * (DeckExt.X - 20.f), SY * (DeckExt.Y - 15.f), LegHalf);
			Box(Center + Rot.RotateVector(Local), FVector(10.f, 10.f, LegHalf), Rot, Gunmetal);
		}
	}
	for (int32 SY = -1; SY <= 1; SY += 2)
	{
		Box(Center + Rot.RotateVector(FVector(0.f, SY * (DeckExt.Y - 4.f), DeckTop + 6.f)), FVector(DeckExt.X, 4.f, 6.f), Rot, Hazard);
		Box(Center + Rot.RotateVector(FVector(0.f, SY * (DeckExt.Y - 2.f), DeckTop + 25.f)), FVector(DeckExt.X, 3.f, 3.f), Rot, Gunmetal);
		Box(Center + Rot.RotateVector(FVector(0.f, SY * (DeckExt.Y - 2.f), DeckTop + 50.f)), FVector(DeckExt.X, 3.f, 3.f), Rot, Gunmetal);
		for (int32 SX = -1; SX <= 1; SX += 2)
		{
			Box(Center + Rot.RotateVector(FVector(SX * (DeckExt.X - 4.f), SY * (DeckExt.Y - 2.f), DeckTop + 27.f)), FVector(3.f, 3.f, 27.f), Rot, Gunmetal);
		}
	}
	// Rail across the closed end.
	Box(Center + Rot.RotateVector(FVector(-Dir * (DeckExt.X - 2.f), 0.f, DeckTop + 50.f)), FVector(3.f, DeckExt.Y, 3.f), Rot, Gunmetal);

	Ramp(RampBottom, RampTop, RampHalfWidth);
}

// Tilted slab from Bottom to Top. 270 rise over 480 run is ~29 deg, under the 44 deg walkable limit.
void AALArenaBuilder::Ramp(const FVector& Bottom, const FVector& Top, float HalfWidth)
{
	using namespace ALYard;
	const FVector Delta = Top - Bottom;
	const double Len = Delta.Size();
	if (Len < KINDA_SMALL_NUMBER) return;
	const FRotator Rot = FRotationMatrix::MakeFromX(Delta / Len).Rotator();
	const FVector Mid = (Bottom + Top) * 0.5f;
	const double HalfLen = Len * 0.5;
	Box(Mid, FVector(HalfLen, HalfWidth, DeckHalfThick), Rot, Grate);
	for (int32 SY = -1; SY <= 1; SY += 2)
	{
		Box(Mid + Rot.RotateVector(FVector(0.f, SY * (HalfWidth - 4.f), DeckHalfThick + 6.f)), FVector(HalfLen, 4.0, 6.0), Rot, Hazard);
	}
}

// 400 tall concrete column with an amber band at hero chest height and a gunmetal cap.
void AALArenaBuilder::Pillar(const FVector& Base)
{
	using namespace ALYard;
	if (!Fits(Base, PillarExt.Size2D())) return;
	Box(Base + FVector(0.f, 0.f, PillarExt.Z), PillarExt, Concrete);
	Box(Base + FVector(0.f, 0.f, 130.f), FVector(PillarExt.X + 2.f, PillarExt.Y + 2.f, 10.f), Hazard);
	Box(Base + FVector(0.f, 0.f, PillarExt.Z * 2.f + 6.f), FVector(PillarExt.X + 7.f, PillarExt.Y + 7.f, 6.f), Gunmetal);
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
