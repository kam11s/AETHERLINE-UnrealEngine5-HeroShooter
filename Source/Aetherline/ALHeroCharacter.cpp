#include "ALHeroCharacter.h"
#include "ALGameState.h"
#include "ALGameMode.h"
#include "ALTypes.h"
#include "ALHeroCatalog.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

static TAutoConsoleVariable<int32> CVarALAimAssistDebug(
	TEXT("al.AimAssistDebug"), 0,
	TEXT("1 = draw the aim assist target point (teal) and the pull line from the crosshair (amber)."));
static TAutoConsoleVariable<int32> CVarALAimAssistForcePad(
	TEXT("al.AimAssistForcePad"), 0,
	TEXT("1 = use the gamepad assist strength and friction whatever the input device (test the pad feel on a mouse)."));

namespace
{
	// Compact carbine built from Engine BasicShapes, laid out in GunRoot space (cm).
	// X runs down the barrel, origin is the rear-bottom corner of the receiver, so the receiver's rear face
	// sits exactly GunRestLocation.X in front of the camera. The camera sits above-left of the gun, so it sees
	// the top and left faces: a stepped profile (receiver -> forend -> barrel -> muzzle), parts hanging below
	// (grip, magazine) and parts on top (rail, sights) are what make the wedge read as a weapon.
	//
	//                 rear sight        rail                          front sight
	//                    [ ]  ============================               |
	//  camera ->   +---------------------------------+------------+======#==+  muzzle
	//              |            receiver             |   forend   |  barrel
	//              +----+---------------+------------+------------+
	//                    \ grip \        | mag |
	//                     \      \       |     |
	const FVector ReceiverSize(38.f, 7.5f, 9.f);
	const FVector ReceiverCenter(19.f, 0.f, 4.5f);
	const FVector GripSize(5.f, 4.f, 14.f);
	const FVector GripCenter(7.f, 0.f, -6.f);
	const FRotator GripTilt(-18.f, 0.f, 0.f); // bottom of the grip rakes back toward the camera
	const FVector MagazineSize(4.5f, 3.5f, 13.f);
	const FVector MagazineCenter(19.f, 0.f, -5.5f);
	const FRotator MagazineTilt(7.f, 0.f, 0.f); // bottom of the magazine leans forward
	const FVector ForendSize(12.f, 6.f, 5.5f);
	const FVector ForendCenter(44.f, 0.f, 4.75f);
	// Cylinder.Cylinder runs along its local Z, so cylinder sizes are (diameter, diameter, length) and the part
	// is pitched 90 degrees to lay that length along the barrel axis (X).
	const FRotator AlongX(90.f, 0.f, 0.f);
	const FVector BarrelSize(3.4f, 3.4f, 26.f);   // 26cm long, X 50..76
	const FVector BarrelCenter(63.f, 0.f, 6.f);
	const FVector MuzzleSize(5.f, 5.f, 5.f);      // X 75..80
	const FVector MuzzleCenter(77.5f, 0.f, 6.f);
	const FVector RailSize(32.f, 3.f, 1.5f);
	const FVector RailCenter(22.f, 0.f, 9.75f);
	const FVector RearSightSize(2.f, 2.5f, 2.5f);
	const FVector RearSightCenter(3.f, 0.f, 10.25f);
	const FVector FrontSightSize(1.5f, 1.5f, 4.f);
	const FVector FrontSightCenter(73.f, 0.f, 9.5f); // top at Z=11.5, level with the rear sight
	const FVector GunMuzzleLocal(80.5f, 0.f, 6.f);

	// Palette: graphite body, gunmetal grip/forend, dark teal accents, amber sights.
	const FLinearColor GunGraphite(0.07f, 0.08f, 0.09f);
	const FLinearColor GunSteel(0.11f, 0.12f, 0.13f);
	const FLinearColor GunDarkTeal(0.04f, 0.19f, 0.23f);
	const FLinearColor GunTeal(0.05f, 0.26f, 0.30f);
	const FLinearColor GunNearBlack(0.02f, 0.025f, 0.03f);
	const FLinearColor GunAmber(0.89f, 0.60f, 0.18f);

	// Third-person proxy (owner-no-see) standing vs. crouched, relative to the capsule centre. The body cylinder is
	// 100cm tall before scale, so standing it spans -88..40 and crouched -52..20, hugging the capsule in both poses.
	const float BodyStandZ = -24.f;
	const float BodyCrouchZ = -16.f;
	const float BodyStandScaleZ = 1.28f;
	const float BodyCrouchScaleZ = 0.72f;
	const float HeadStandZ = 62.f;
	const float HeadCrouchZ = 30.f;
	// Overlap tests use a capsule shrunk by this much so resting contact with floor / walls does not count as a block
	// (same trick as UCharacterMovementComponent::UnCrouch).
	const float StandSweepInflation = 0.1f;

	// The kick spring is kept under-damped so the closed-form solution below applies (and so it overshoots a touch).
	const float KickMinDamping = 0.05f;
	const float KickMaxDamping = 0.95f;

	// Velocity impulse that makes a damped spring (natural frequency Omega rad/s, damping ratio Zeta) starting at
	// rest peak at exactly 1.0. Lets the kick be tuned in "peak displacement" rather than raw velocity.
	float SpringUnitImpulse(float Omega, float Zeta)
	{
		const float Wd = Omega * FMath::Sqrt(1.f - Zeta * Zeta);
		const float PeakTime = FMath::Atan2(Wd, Zeta * Omega) / Wd;
		const float Peak = FMath::Exp(-Zeta * Omega * PeakTime) * FMath::Sin(Wd * PeakTime) / Wd;
		return 1.f / FMath::Max(Peak, UE_KINDA_SMALL_NUMBER);
	}

	// Advances a damped spring by Dt using the exact under-damped solution, so the result is frame-rate independent
	// and cannot blow up on a hitch (a plain Euler step at 60fps over-damps the first frame by ~half).
	void StepDampedSpring(float& X, float& V, float Omega, float Zeta, float Dt)
	{
		const float Wd = Omega * FMath::Sqrt(1.f - Zeta * Zeta);
		const float Decay = FMath::Exp(-Zeta * Omega * Dt);
		const float C = FMath::Cos(Wd * Dt);
		const float S = FMath::Sin(Wd * Dt);
		const float NewX = Decay * (X * C + (V + Zeta * Omega * X) / Wd * S);
		const float NewV = Decay * (V * C - (Omega * Omega * X + Zeta * Omega * V) / Wd * S);
		X = NewX;
		V = NewV;
	}

	void TintGunPart(UStaticMeshComponent* Part, UMaterialInterface* Base, UObject* Outer, const FLinearColor& Color)
	{
		if (!Part || !Base) return;
		if (UMaterialInstanceDynamic* Dyn = UMaterialInstanceDynamic::Create(Base, Outer))
		{
			Dyn->SetVectorParameterValue(TEXT("Color"), Color);
			Part->SetMaterial(0, Dyn);
		}
	}
}

AALHeroCharacter::AALHeroCharacter()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCapsuleComponent()->SetCapsuleHalfHeight(StandingHalfHeight);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);
	// The engine's Pawn profile ignores Visibility, which is the channel FireShot traces on, so shots (and the aim
	// assist's line-of-sight test) would pass straight through heroes. The capsule is the hitbox; make it block.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = StandingSpeed;
	GetCharacterMovement()->JumpZVelocity = 520.f;
	GetCharacterMovement()->AirControl = 0.35f;
	FPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPCamera"));
	FPCamera->SetupAttachment(GetCapsuleComponent());
	FPCamera->SetRelativeLocation(FVector(0.f, 0.f, StandingEyeZ));
	FPCamera->SetFieldOfView(90.f);
	FPCamera->bUsePawnControlRotation = true;

	GunRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GunRoot"));
	GunRoot->SetupAttachment(FPCamera);
	GunRoot->SetRelativeLocation(GunRestLocation);
	GunRoot->SetRelativeRotation(GunRestRotation);
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, BodyStandZ));
	BodyMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, BodyStandScaleZ));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetOwnerNoSee(true);
	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, HeadStandZ));
	HeadMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.4f));
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetOwnerNoSee(true);


	// FObjectFinder is the constructor-safe way to reference Engine assets (LoadObject here failed silently).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterial> BasicMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	// .Object is a TObjectPtr in 5.8: resolve each finder to a raw pointer first so the fallback ternary below
	// compares like with like instead of mixing TObjectPtr and UStaticMesh*.
	UStaticMesh* Cube = CubeFinder.Object.Get();
	UStaticMesh* CylinderMesh = CylinderFinder.Object.Get();
	UStaticMesh* Cylinder = CylinderMesh ? CylinderMesh : Cube;
	GunBaseMaterial = BasicMaterialFinder.Object.Get();

	GunMesh = MakeGunPart(TEXT("GunMesh"), Cube, ReceiverCenter, ReceiverSize, FRotator::ZeroRotator);
	GunGrip = MakeGunPart(TEXT("GunGrip"), Cube, GripCenter, GripSize, GripTilt);
	GunMagazine = MakeGunPart(TEXT("GunMagazine"), Cube, MagazineCenter, MagazineSize, MagazineTilt);
	GunForend = MakeGunPart(TEXT("GunForend"), Cube, ForendCenter, ForendSize, FRotator::ZeroRotator);
	GunBarrel = MakeGunPart(TEXT("GunBarrel"), Cylinder, BarrelCenter, BarrelSize, AlongX);
	GunMuzzle = MakeGunPart(TEXT("GunMuzzle"), Cylinder, MuzzleCenter, MuzzleSize, AlongX);
	GunRail = MakeGunPart(TEXT("GunRail"), Cube, RailCenter, RailSize, FRotator::ZeroRotator);
	GunRearSight = MakeGunPart(TEXT("GunRearSight"), Cube, RearSightCenter, RearSightSize, FRotator::ZeroRotator);
	GunSight = MakeGunPart(TEXT("GunSight"), Cube, FrontSightCenter, FrontSightSize, FRotator::ZeroRotator);
	if (Cylinder) BodyMesh->SetStaticMesh(Cylinder);
	if (Cube) HeadMesh->SetStaticMesh(Cube);
	RefreshTeamVisuals();
}

// InMesh, not Mesh: a parameter named Mesh shadows ACharacter::Mesh and the 5.8 toolchain treats that as an error.
UStaticMeshComponent* AALHeroCharacter::MakeGunPart(const TCHAR* Name, UStaticMesh* InMesh, const FVector& Center, const FVector& Size, const FRotator& Rotation)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(GunRoot);
	Part->SetRelativeLocation(Center);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Size / 100.f);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCastShadow(false);
	if (InMesh) Part->SetStaticMesh(InMesh);
	return Part;
}
void AALHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	const FRotator Ctrl = GetControlRotation();
	LastControlYaw = static_cast<float>(Ctrl.Yaw);
	LastControlPitch = static_cast<float>(Ctrl.Pitch);
	if (UMaterialInterface* Base = GunBaseMaterial.Get())
	{
		TintGunPart(GunMesh, Base, this, GunGraphite);
		TintGunPart(GunGrip, Base, this, GunSteel);
		TintGunPart(GunMagazine, Base, this, GunDarkTeal);
		TintGunPart(GunForend, Base, this, GunSteel);
		TintGunPart(GunBarrel, Base, this, GunTeal);
		TintGunPart(GunMuzzle, Base, this, GunNearBlack);
		TintGunPart(GunRail, Base, this, GunDarkTeal);
		TintGunPart(GunRearSight, Base, this, GunAmber);
		TintGunPart(GunSight, Base, this, GunAmber);
	}
}
FVector AALHeroCharacter::GetMuzzleLocation() const
{
	if (GunRoot) return GunRoot->GetComponentTransform().TransformPosition(GunMuzzleLocal);
	return GetActorLocation() + GetActorForwardVector() * 60.f;
}
FVector AALHeroCharacter::GetEyeLocation() const
{
	return FPCamera ? FPCamera->GetComponentLocation() : GetActorLocation() + FVector(0.f, 0.f, StandingEyeZ);
}
void AALHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AALHeroCharacter, UltCharge);
	DOREPLIFETIME(AALHeroCharacter, bOnDropship);
	DOREPLIFETIME(AALHeroCharacter, bSkydiving);
	DOREPLIFETIME(AALHeroCharacter, Health);
}
void AALHeroCharacter::ApplyHero(EALHero Hero)
{
	HeroId = Hero;
	const FALHeroDef Def = UALHeroCatalog::Get(Hero);
	StandingSpeed = Def.MoveSpeed;
	GetCharacterMovement()->MaxWalkSpeed = Def.MoveSpeed;
	// Swapping hero mid-crouch keeps the crouched speed instead of popping back to the standing value.
	if (CrouchProgress > 0.f) ApplyCrouchPose(FMath::SmoothStep(0.f, 1.f, CrouchProgress));
	MaxHealth = Def.MaxHealth;
	Health = Def.MaxHealth;
}
void AALHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (TeamId != VisualTeam) RefreshTeamVisuals();
	FireCooldown = FMath::Max(0.f, FireCooldown - DeltaSeconds);
	UltCharge = FMath::Min(100.f, UltCharge + DeltaSeconds * 2.f);
	if (bSkydiving && GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround()) bSkydiving = false;
	if (bFireHeld && IsLocallyControlled()) FireOnce();
	UpdateCrouch(DeltaSeconds);
	// Recoil first, assist second: the assist's pull-down then reads as compensation on the next recoil update.
	UpdateAimRecoil(DeltaSeconds);
	UpdateAimAssist(DeltaSeconds);
	UpdateViewmodel(DeltaSeconds);
	LookInputThisFrame = 0.f;
}
void AALHeroCharacter::AddFireKick(float Heft)
{
	// Viewmodel: velocity impulse into the spring, capped at one full-heft shot so a burst cannot wind it up past
	// the pose the rest location was cleared for (see GunRestLocation). A returning gun (negative velocity) soaks up
	// part of the next impulse, which is what gives sustained fire its rhythm.
	const float Omega = 2.f * PI * FMath::Max(KickFrequencyHz, 0.1f);
	const float UnitImpulse = SpringUnitImpulse(Omega, FMath::Clamp(KickDampingRatio, KickMinDamping, KickMaxDamping));
	GunKickVel = FMath::Min(GunKickVel + Heft * UnitImpulse, UnitImpulse);
	// Re-roll the sideways direction but keep some of the last one so it drifts rather than jitters.
	KickSide = FMath::Lerp(KickSide, FMath::FRandRange(-1.f, 1.f), 0.6f);

	// Aim: queue the climb; UpdateAimRecoil eases the control rotation onto it. Bots keep the viewmodel kick
	// (others can see their gun) but never have their aim pushed around.
	if (!Cast<APlayerController>(GetController())) return;
	if (UWorld* W = GetWorld()) LastShotTime = static_cast<float>(W->GetTimeSeconds());
	// FireShot clamps heft to 0.3..1 (carbine class .. Bastion); remap so the carbine gets exactly the light value.
	const float HeftAlpha = FMath::Clamp((Heft - 0.3f) / 0.7f, 0.f, 1.f);
	float Kick = FMath::Lerp(RecoilPitchLightDeg, RecoilPitchHeavyDeg, HeftAlpha);
	Kick *= 1.f + FMath::FRandRange(-1.f, 1.f) * FMath::Clamp(RecoilPitchNoise, 0.f, 0.9f);
	// Progressive climb that plateaus: full kicks while the burst is young, then each shot adds less as the
	// accumulated climb closes on RecoilClimbMaxDeg, so a spray levels off at the cap instead of hard-stopping.
	const float ClimbMax = FMath::Max(RecoilClimbMaxDeg, Kick);
	const float SoftStart = FMath::Clamp(RecoilClimbSoftStart, 0.f, 0.99f) * ClimbMax;
	const float Remaining = FMath::Max(ClimbMax - RecoilPitchTarget, 0.f);
	if (RecoilPitchTarget > SoftStart)
	{
		Kick *= FMath::Clamp(Remaining / FMath::Max(ClimbMax - SoftStart, UE_KINDA_SMALL_NUMBER), 0.f, 1.f);
	}
	RecoilPitchTarget += FMath::Min(Kick, Remaining);
	RecoilYawTarget += FMath::FRandRange(-1.f, 1.f) * RecoilYawNoiseDeg * FMath::Lerp(1.f, 1.6f, HeftAlpha);
}
void AALHeroCharacter::UpdateAimRecoil(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f || !IsLocallyControlled()) return;
	APlayerController* PC = Cast<APlayerController>(GetController());
	UWorld* W = GetWorld();
	if (!PC || !W)
	{
		RecoilPitchTarget = RecoilYawTarget = RecoilPitchApplied = RecoilYawApplied = 0.f;
		bRecoilMarkValid = false;
		return;
	}
	FRotator Ctrl = PC->GetControlRotation();
	const float CtrlPitch = static_cast<float>(Ctrl.Pitch);
	const float CtrlYaw = static_cast<float>(Ctrl.Yaw);

	// Compensation: whatever moved the view since our last write (player look, aim assist, the camera's pitch
	// clamp) that opposes the recoil is credited against it. The climb the player has already fought off must not
	// be recovered again on release, or the crosshair would dive under the target they were holding.
	if (bRecoilMarkValid)
	{
		const float ExtPitch = FMath::FindDeltaAngleDegrees(RecoilPitchMark, CtrlPitch);
		if (ExtPitch < 0.f && RecoilPitchTarget > 0.f)
		{
			const float Credit = FMath::Min(-ExtPitch, RecoilPitchTarget);
			RecoilPitchTarget -= Credit;
			RecoilPitchApplied -= Credit;
		}
		const float ExtYaw = FMath::FindDeltaAngleDegrees(RecoilYawMark, CtrlYaw);
		if (!FMath::IsNearlyZero(RecoilYawTarget) && ExtYaw * RecoilYawTarget < 0.f)
		{
			const float Credit = FMath::Min(FMath::Abs(ExtYaw), FMath::Abs(RecoilYawTarget)) * FMath::Sign(RecoilYawTarget);
			RecoilYawTarget -= Credit;
			RecoilYawApplied -= Credit;
		}
	}

	// Recovery: once the burst has paused, drift the earned climb back toward rest. Proportional so the tail
	// eases out, floored so a small residual does not crawl.
	const float SinceShot = static_cast<float>(W->GetTimeSeconds()) - LastShotTime;
	const float Total = FMath::Abs(RecoilPitchTarget) + FMath::Abs(RecoilYawTarget);
	if (SinceShot >= RecoilRecoverDelay && Total > UE_KINDA_SMALL_NUMBER)
	{
		const float Step = FMath::Min(Total, FMath::Max(Total * RecoilRecoverSpeed, RecoilRecoverMinDegPerSec) * DeltaSeconds);
		const float Keep = 1.f - Step / Total;
		RecoilPitchTarget *= Keep;
		RecoilYawTarget *= Keep;
	}

	// Ease the applied offset onto the target and write only the frame-to-frame change into the control rotation,
	// so the player's own look input passes straight through and the camera manager's pitch limits still apply.
	const float PrevPitch = RecoilPitchApplied;
	const float PrevYaw = RecoilYawApplied;
	RecoilPitchApplied = FMath::FInterpTo(RecoilPitchApplied, RecoilPitchTarget, DeltaSeconds, RecoilSnapSpeed);
	RecoilYawApplied = FMath::FInterpTo(RecoilYawApplied, RecoilYawTarget, DeltaSeconds, RecoilSnapSpeed);
	const float DeltaPitch = RecoilPitchApplied - PrevPitch;
	const float DeltaYaw = RecoilYawApplied - PrevYaw;
	if (!FMath::IsNearlyZero(DeltaPitch, 1e-4f) || !FMath::IsNearlyZero(DeltaYaw, 1e-4f))
	{
		Ctrl.Pitch += DeltaPitch;
		Ctrl.Yaw += DeltaYaw;
		PC->SetControlRotation(Ctrl);
	}
	RecoilPitchMark = static_cast<float>(Ctrl.Pitch);
	RecoilYawMark = static_cast<float>(Ctrl.Yaw);
	bRecoilMarkValid = true;
}
void AALHeroCharacter::NoteInputDevice(bool bGamepad)
{
	bUsingGamepad = bGamepad;
}
AALHeroCharacter* AALHeroCharacter::FindAssistTarget(const FVector& Eye, const FVector& Fwd, FVector& OutAimDir, float& OutEdgeDeg, float& OutBodyDeg, float& OutDist) const
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;
	const float Cone = FMath::Max(AssistConeDeg, 0.f);
	const float MaxRange = FMath::Max(AssistMaxRangeCm, 1.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ALAimAssist), false, this);

	AALHeroCharacter* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (TActorIterator<AALHeroCharacter> It(W); It; ++It)
	{
		AALHeroCharacter* H = *It;
		if (H == this || !H->IsAlive() || H->TeamId == TeamId || H->bOnDropship) continue;
		const UCapsuleComponent* Cap = H->GetCapsuleComponent();
		if (!Cap) continue;
		const FVector Center = H->GetActorLocation();
		const float Dist = static_cast<float>(FVector::Dist(Eye, Center));
		if (Dist > MaxRange || Dist < 60.f) continue;
		// Cheap pre-reject on the centre: no part of the capsule can be inside the cone if the centre is further
		// off than the cone plus the capsule's own angular half-height at this range.
		const float HalfHeight = Cap->GetScaledCapsuleHalfHeight();
		const float PreDeg = FMath::Min(Cone + FMath::RadiansToDegrees(FMath::Atan(HalfHeight / Dist)), 89.f);
		if (FVector::DotProduct(Fwd, (Center - Eye).GetSafeNormal()) < FMath::Cos(FMath::DegreesToRadians(PreDeg))) continue;

		// Pull toward the nearest point of the capsule's core segment to the aim ray, not the centre, so a head
		// or a knee poking past cover is as good a magnet as centre mass.
		const float Radius = Cap->GetScaledCapsuleRadius();
		const float Core = FMath::Max(HalfHeight - Radius, 0.f);
		FVector OnRay, OnCore;
		FMath::SegmentDistToSegmentSafe(Eye, Eye + Fwd * MaxRange, Center - FVector(0.f, 0.f, Core), Center + FVector(0.f, 0.f, Core), OnRay, OnCore);
		const FVector ToCore = OnCore - Eye;
		const float CoreDist = static_cast<float>(ToCore.Size());
		if (CoreDist < UE_KINDA_SMALL_NUMBER) continue;
		const FVector AimDir = ToCore / CoreDist;
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(Fwd, AimDir)), -1.f, 1.f)));
		const float BodyDeg = FMath::RadiansToDegrees(FMath::Atan(Radius / CoreDist));
		// Angle from the crosshair to the visible edge of the body; inside the body it is zero.
		const float EdgeDeg = FMath::Max(AngleDeg - BodyDeg, 0.f);
		if (EdgeDeg > Cone) continue;

		FHitResult Hit;
		const bool bBlocked = W->LineTraceSingleByChannel(Hit, Eye, OnCore, ECC_Visibility, Params) && Hit.GetActor() != H;
		if (bBlocked) continue;

		// Nearest to the crosshair wins, with a mild bias toward closer enemies and toward the one we already have
		// so two overlapping bots do not make the pull flip-flop.
		float Score = EdgeDeg + Dist * 0.0004f;
		if (H == AssistTarget.Get()) Score -= 1.5f;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = H;
			OutAimDir = AimDir;
			OutEdgeDeg = EdgeDeg;
			OutBodyDeg = BodyDeg;
			OutDist = CoreDist;
		}
	}
	return Best;
}
void AALHeroCharacter::UpdateAimAssist(float DeltaSeconds)
{
	UWorld* W = GetWorld();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!W || !PC || DeltaSeconds <= 0.f || !IsLocallyControlled() || !IsAlive() || bOnDropship)
	{
		AssistTarget.Reset();
		AssistWeight = 0.f;
		return;
	}
	const bool bPad = bUsingGamepad || CVarALAimAssistForcePad.GetValueOnGameThread() != 0;
	const float Strength = FMath::Max(bPad ? AssistStrengthPad : AssistStrengthMnK, 0.f);
	if (Strength <= 0.f || AssistConeDeg <= 0.f)
	{
		AssistTarget.Reset();
		AssistWeight = 0.f;
		return;
	}

	const FVector Eye = GetEyeLocation();
	FRotator Ctrl = PC->GetControlRotation();
	const FVector Fwd = Ctrl.Vector();
	FVector AimDir = Fwd;
	float EdgeDeg = 0.f, BodyDeg = 0.f, Dist = 0.f;
	AALHeroCharacter* Target = FindAssistTarget(Eye, Fwd, AimDir, EdgeDeg, BodyDeg, Dist);
	if (!Target)
	{
		AssistTarget.Reset();
		AssistWeight = FMath::FInterpTo(AssistWeight, 0.f, DeltaSeconds, 12.f);
		return;
	}
	const bool bNewTarget = Target != AssistTarget.Get();
	AssistTarget = Target;

	// Falloff: full inside the cone's centre, easing to nothing at its edge; full out to AssistFullRangeCm, easing to
	// nothing at AssistMaxRangeCm. The assist never has a hard boundary the player can feel.
	const float AngleWeight = FMath::SmoothStep(0.f, 1.f, 1.f - EdgeDeg / FMath::Max(AssistConeDeg, UE_KINDA_SMALL_NUMBER));
	const float RangeSpan = FMath::Max(AssistMaxRangeCm - AssistFullRangeCm, 1.f);
	const float DistWeight = 1.f - FMath::Clamp((Dist - AssistFullRangeCm) / RangeSpan, 0.f, 1.f);
	const float Weight = AngleWeight * DistWeight * Strength;
	AssistWeight = FMath::Clamp(Weight, 0.f, 1.f);

	// Soft, not a lock: the pull needs someone to be doing something. A still player looking at a still bot only
	// gets AssistIdleScale of it, so the reticle is never dragged onto a target on its own.
	const bool bLooking = LookInputThisFrame > 0.f;
	const bool bMoving = GetVelocity().SizeSquared2D() > 30.f * 30.f;
	const bool bTargetMoving = Target->GetVelocity().SizeSquared() > 30.f * 30.f;
	const bool bFiring = bFireHeld || FireCooldown > 0.f;
	const float Activity = (bLooking || bMoving || bTargetMoving || bFiring) ? 1.f : FMath::Clamp(AssistIdleScale, 0.f, 1.f);

	// Magnetism: close a share of the angle to the nearest body point, minus a dead zone over the body itself.
	const FRotator ToRot = AimDir.Rotation();
	const float DPitch = FMath::FindDeltaAngleDegrees(static_cast<float>(Ctrl.Pitch), static_cast<float>(ToRot.Pitch));
	const float DYaw = FMath::FindDeltaAngleDegrees(static_cast<float>(Ctrl.Yaw), static_cast<float>(ToRot.Yaw));
	const float Mag = FMath::Sqrt(DPitch * DPitch + DYaw * DYaw);
	const float Dead = BodyDeg * FMath::Clamp(AssistBodyDeadFraction, 0.f, 1.f);
	float PullPitch = 0.f, PullYaw = 0.f;
	if (Mag > Dead + UE_KINDA_SMALL_NUMBER)
	{
		const float Alpha = FMath::Min(AssistPullPerSec * Weight * Activity * DeltaSeconds, 1.f) * (Mag - Dead) / Mag;
		PullPitch = DPitch * Alpha;
		PullYaw = DYaw * Alpha;
	}

	// Tracking: ride along with the target's apparent motion (their strafe, or ours past them) so a moving enemy
	// stays under the reticle without the stick having to lead it. Measured on the capsule centre, which does not
	// slide when we pitch, unlike the nearest-core point above.
	const FVector CenterDir = (Target->GetActorLocation() - Eye).GetSafeNormal();
	if (!bNewTarget && !CenterDir.IsNearlyZero() && !AssistPrevCenterDir.IsNearlyZero())
	{
		const FRotator PrevRot = AssistPrevCenterDir.Rotation();
		const FRotator CurRot = CenterDir.Rotation();
		const float Track = FMath::Max(AssistTrackStrength, 0.f) * Weight;
		PullPitch += FMath::FindDeltaAngleDegrees(static_cast<float>(PrevRot.Pitch), static_cast<float>(CurRot.Pitch)) * Track;
		PullYaw += FMath::FindDeltaAngleDegrees(static_cast<float>(PrevRot.Yaw), static_cast<float>(CurRot.Yaw)) * Track;
	}
	AssistPrevCenterDir = CenterDir;

	// Rate cap so nothing ever reads as a snap, however close the target or fast the frame.
	const float PullMag = FMath::Sqrt(PullPitch * PullPitch + PullYaw * PullYaw);
	const float MaxStep = FMath::Max(AssistMaxPullDegPerSec, 0.f) * DeltaSeconds;
	if (PullMag > MaxStep && PullMag > UE_KINDA_SMALL_NUMBER)
	{
		PullPitch *= MaxStep / PullMag;
		PullYaw *= MaxStep / PullMag;
	}
	if (!FMath::IsNearlyZero(PullPitch, 1e-5f) || !FMath::IsNearlyZero(PullYaw, 1e-5f))
	{
		Ctrl.Pitch += PullPitch;
		Ctrl.Yaw += PullYaw;
		PC->SetControlRotation(Ctrl);
	}
	if (CVarALAimAssistDebug.GetValueOnGameThread() != 0)
	{
		const FVector Point = Eye + AimDir * Dist;
		DrawDebugSphere(W, Point, 14.f, 8, FColor(40, 220, 255), false, -1.f, 0, 1.f);
		DrawDebugLine(W, Eye + Fwd * 100.f, Point, FColor(255, 170, 50), false, -1.f, 0, 0.6f);
	}
}
void AALHeroCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	const float FallSpeed = FMath::Abs(static_cast<float>(GetVelocity().Z));
	GunLandDip = FMath::Clamp(FallSpeed / 900.f, 0.3f, 1.f);
}
void AALHeroCharacter::UpdateViewmodel(float DeltaSeconds)
{
	UWorld* W = GetWorld();
	if (!W || !GunRoot || !IsLocallyControlled() || DeltaSeconds <= 0.f) return;

	// Kick spring. Zero is the rest pose; shots push GunKick positive through GunKickVel (AddFireKick).
	StepDampedSpring(GunKick, GunKickVel, 2.f * PI * FMath::Max(KickFrequencyHz, 0.1f),
		FMath::Clamp(KickDampingRatio, KickMinDamping, KickMaxDamping), DeltaSeconds);
	GunLandDip = FMath::FInterpTo(GunLandDip, 0.f, DeltaSeconds, 6.f);

	// Look sway: the gun lags behind the camera by an amount proportional to turn rate.
	const FRotator Ctrl = GetControlRotation();
	const float CtrlYaw = static_cast<float>(Ctrl.Yaw);
	const float CtrlPitch = static_cast<float>(Ctrl.Pitch);
	const float YawRate = FMath::FindDeltaAngleDegrees(LastControlYaw, CtrlYaw) / DeltaSeconds;
	const float PitchRate = FMath::FindDeltaAngleDegrees(LastControlPitch, CtrlPitch) / DeltaSeconds;
	LastControlYaw = CtrlYaw;
	LastControlPitch = CtrlPitch;
	const float SwayYaw = FMath::Clamp(-YawRate * SwayScale, -SwayMaxDeg, SwayMaxDeg);
	const float SwayPitch = FMath::Clamp(-PitchRate * SwayScale, -SwayMaxDeg, SwayMaxDeg);
	SwayRot = FMath::RInterpTo(SwayRot, FRotator(SwayPitch, SwayYaw, SwayYaw * 0.4f), DeltaSeconds, SwaySpeed);

	// Walk bob: figure-eight, scaled by ground speed, fades out in the air.
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const float MaxSpeed = Move ? FMath::Max(Move->MaxWalkSpeed, 1.f) : 600.f;
	const bool bGrounded = Move && Move->IsMovingOnGround();
	const float GroundSpeed = static_cast<float>(GetVelocity().Size2D());
	const float SpeedAlpha = bGrounded ? FMath::Clamp(GroundSpeed / MaxSpeed, 0.f, 1.f) : 0.f;
	BobBlend = FMath::FInterpTo(BobBlend, SpeedAlpha, DeltaSeconds, 8.f);
	if (SpeedAlpha > 0.05f) BobTime += DeltaSeconds * BobFrequency * FMath::Lerp(0.6f, 1.f, SpeedAlpha);
	const float BobY = FMath::Sin(BobTime) * BobAmplitude * BobBlend;
	const float BobZ = FMath::Sin(BobTime * 2.f) * BobAmplitude * 0.5f * BobBlend;
	const float Breath = FMath::Sin(static_cast<float>(W->GetTimeSeconds()) * 1.4f) * 0.25f;

	// Kick: straight back into the shoulder, a little up, muzzle pitches up; a small sideways share (lateral shove,
	// yaw, roll) in the direction rolled for this shot keeps a burst from pumping in a perfectly straight line.
	const float SideKick = GunKick * KickSide * KickSideFraction;
	const float OffX = -GunKick * KickBackCm;
	const float OffY = static_cast<float>(SwayRot.Yaw) * 0.12f + BobY + SideKick * KickBackCm;
	const float OffZ = BobZ + Breath + GunKick * KickBackCm * 0.15f - GunLandDip * LandDipCm;
	const float TiltPitch = static_cast<float>(SwayRot.Pitch) + GunKick * KickPitchDeg - GunLandDip * 3.f;
	const float TiltYaw = static_cast<float>(SwayRot.Yaw) + SideKick * KickPitchDeg;
	const float TiltRoll = static_cast<float>(SwayRot.Roll) + SideKick * KickPitchDeg;
	const FVector Offset(OffX, OffY, OffZ);
	const FRotator Tilt(TiltPitch, TiltYaw, TiltRoll);
	GunRoot->SetRelativeLocation(GunRestLocation + Offset);
	GunRoot->SetRelativeRotation(GunRestRotation + Tilt);
}
void AALHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AALHeroCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AALHeroCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("LookYaw"), this, &AALHeroCharacter::LookYaw);
	PlayerInputComponent->BindAxis(TEXT("LookPitch"), this, &AALHeroCharacter::LookPitch);
	PlayerInputComponent->BindAxis(TEXT("LookYawGamepad"), this, &AALHeroCharacter::LookYawGamepad);
	PlayerInputComponent->BindAxis(TEXT("LookPitchGamepad"), this, &AALHeroCharacter::LookPitchGamepad);
	PlayerInputComponent->BindAxis(TEXT("FireAxis"), this, &AALHeroCharacter::FireAxis);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AALHeroCharacter::OnJump);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AALHeroCharacter::OnFire);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AALHeroCharacter::OnFireReleased);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AALHeroCharacter::OnCrouchToggle);
	PlayerInputComponent->BindAction(TEXT("CrouchHold"), IE_Pressed, this, &AALHeroCharacter::OnCrouchHoldPressed);
	PlayerInputComponent->BindAction(TEXT("CrouchHold"), IE_Released, this, &AALHeroCharacter::OnCrouchHoldReleased);
	PlayerInputComponent->BindAction(TEXT("HeroPrev"), IE_Pressed, this, &AALHeroCharacter::HeroPrev);
	PlayerInputComponent->BindAction(TEXT("HeroNext"), IE_Pressed, this, &AALHeroCharacter::HeroNext);
}
// Jumping out of a crouch stands you up (CoD style); if the head is blocked UpdateCrouch keeps you down anyway.
void AALHeroCharacter::OnJump() { if (bOnDropship || bSkydiving) DeployFromDropship(); else { bWantsCrouch = false; Jump(); } }
void AALHeroCharacter::AttachToDropship(AActor* Ship) { if (!Ship) return; bOnDropship = true; AttachToActor(Ship, FAttachmentTransformRules::SnapToTargetNotIncludingScale); }
void AALHeroCharacter::DeployFromDropship() { bOnDropship = false; bSkydiving = true; DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); LaunchCharacter(FVector(0.f,0.f,-800.f)+GetActorForwardVector()*400.f,true,true); }
void AALHeroCharacter::OnFire()
{
	// "Fire" is bound to both the mouse button and the pad trigger; ask which one is actually down.
	if (const APlayerController* PC = Cast<APlayerController>(GetController())) NoteInputDevice(PC->IsInputKeyDown(EKeys::Gamepad_RightTrigger));
	bFireHeld = true;
	FireOnce();
}
void AALHeroCharacter::OnFireReleased() { bFireHeld = false; }
void AALHeroCharacter::MoveForward(float V) { if (FMath::Abs(V) > StickDeadZone) AddMovementInput(GetActorForwardVector(), V); }
void AALHeroCharacter::MoveRight(float V) { if (FMath::Abs(V) > StickDeadZone) AddMovementInput(GetActorRightVector(), V); }
// Mouse look. Friction only applies if AssistFrictionMnK is raised above its default of zero.
void AALHeroCharacter::LookYaw(float V)
{
	if (FMath::IsNearlyZero(V)) return;
	NoteInputDevice(false);
	LookInputThisFrame += FMath::Abs(V);
	AddControllerYawInput(V * AssistLookScale());
}
void AALHeroCharacter::LookPitch(float V)
{
	if (FMath::IsNearlyZero(V)) return;
	NoteInputDevice(false);
	LookInputThisFrame += FMath::Abs(V);
	AddControllerPitchInput(V * AssistLookScale());
}
// Stick look. Over an enemy the stick slows by AssistFrictionPad x assist weight (the "sticky" half of the assist).
void AALHeroCharacter::LookYawGamepad(float V)
{
	if (FMath::Abs(V) <= StickDeadZone || !GetWorld()) return;
	NoteInputDevice(true);
	LookInputThisFrame += FMath::Abs(V);
	AddControllerYawInput(V * GamepadLookYawRate * GetWorld()->GetDeltaSeconds() * AssistLookScale());
}
void AALHeroCharacter::LookPitchGamepad(float V)
{
	if (FMath::Abs(V) <= StickDeadZone || !GetWorld()) return;
	NoteInputDevice(true);
	LookInputThisFrame += FMath::Abs(V);
	AddControllerPitchInput(V * GamepadLookPitchRate * GetWorld()->GetDeltaSeconds() * AssistLookScale());
}
float AALHeroCharacter::AssistLookScale() const
{
	const bool bPad = bUsingGamepad || CVarALAimAssistForcePad.GetValueOnGameThread() != 0;
	const float Friction = FMath::Clamp(bPad ? AssistFrictionPad : AssistFrictionMnK, 0.f, 0.9f);
	return 1.f - Friction * FMath::Clamp(AssistWeight, 0.f, 1.f);
}
void AALHeroCharacter::FireAxis(float V) { if (V >= 0.45f) { NoteInputDevice(true); FireOnce(); } }
void AALHeroCharacter::HeroPrev() { ApplyHero(static_cast<EALHero>((static_cast<int32>(HeroId)+5)%6)); }
void AALHeroCharacter::HeroNext() { ApplyHero(static_cast<EALHero>((static_cast<int32>(HeroId)+1)%6)); }
void AALHeroCharacter::FireOnce()
{
	const FVector Dir = FPCamera ? FPCamera->GetForwardVector() : GetActorForwardVector();
	FireShot(Dir, 1.f);
}
bool AALHeroCharacter::FireShot(const FVector& Dir, float DamageScale)
{
	if (FireCooldown > 0.f || !IsAlive() || !GetWorld()) return false;
	const FALHeroDef Def = UALHeroCatalog::Get(HeroId);
	FireCooldown = 1.f / FMath::Max(Def.FireRate, 0.1f);
	const FVector Start = GetEyeLocation();
	const FVector End = Start + Dir * Def.Range;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ALFire), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	const FVector TracerEnd = bHit ? Hit.ImpactPoint : End;
	const FVector Muzzle = GetMuzzleLocation();
	// Teal = friendly fire lanes, amber = hostile, so incoming fire reads at a glance.
	const FColor Tracer = (TeamId == EALTeam::Enemy) ? FColor(255, 150, 40) : FColor(40, 220, 255);
	DrawDebugLine(GetWorld(), Muzzle, TracerEnd, Tracer, false, 0.08f, 0, 2.0f);
	DrawDebugPoint(GetWorld(), Muzzle, 22.f, FColor(255, 170, 50), false, 0.05f);
	if (bHit)
	{
		AALHeroCharacter* Other = Cast<AALHeroCharacter>(Hit.GetActor());
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, Other ? 18.f : 10.f, Other ? FColor::White : FColor(255, 160, 40), false, 0.12f);
		if (Other && Other->TeamId != TeamId) ServerApplyDamageTo(Other, Def.Damage * DamageScale);
	}
	// Heavier hitters kick harder: the carbine-class heroes sit at the light end, Bastion at the heavy end.
	if (IsLocallyControlled()) AddFireKick(FMath::Clamp(Def.Damage / 60.f, 0.3f, 1.f));
	return true;
}
void AALHeroCharacter::ServerApplyDamageTo_Implementation(AALHeroCharacter* Target, float Amount)
{
	if (!HasAuthority() || !Target || !Target->IsAlive()) return;
	if (Target->TeamId == TeamId) return;
	const float Now = GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.f;
	Target->Health = FMath::Max(0.f, Target->Health - Amount);
	Target->LastDamagedTime = Now;
	LastHitConfirmTime = Now;
}

void AALHeroCharacter::RefreshTeamVisuals()
{
	VisualTeam = TeamId;
	const bool bHostile = TeamId == EALTeam::Enemy;
	const FLinearColor BodyCol = bHostile ? FLinearColor(0.85f, 0.30f, 0.06f) : FLinearColor(0.06f, 0.55f, 0.72f);
	const FLinearColor HeadCol = bHostile ? FLinearColor(1.00f, 0.55f, 0.12f) : FLinearColor(0.18f, 0.90f, 1.00f);
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Mat) return;
	auto Tint = [this, Mat](UStaticMeshComponent* M, const FLinearColor& C)
	{
		if (!M) return;
		if (UMaterialInstanceDynamic* Dyn = UMaterialInstanceDynamic::Create(Mat, this))
		{
			Dyn->SetVectorParameterValue(TEXT("Color"), C);
			M->SetMaterial(0, Dyn);
		}
	};
	Tint(BodyMesh, BodyCol);
	Tint(HeadMesh, HeadCol);
}

void AALHeroCharacter::OnCrouchToggle() { SetWantsCrouch(!bWantsCrouch); }
void AALHeroCharacter::OnCrouchHoldPressed() { SetWantsCrouch(bGamepadHoldToCrouch ? true : !bWantsCrouch); }
void AALHeroCharacter::OnCrouchHoldReleased() { if (bGamepadHoldToCrouch) SetWantsCrouch(false); }
void AALHeroCharacter::SetWantsCrouch(bool bCrouch)
{
	// Attached to the dropship the capsule must not move; the blend picks the request up once we are off it.
	if (bOnDropship) return;
	bWantsCrouch = bCrouch;
}

void AALHeroCharacter::UpdateCrouch(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f) return;
	// Wanting to stand is not enough: the standing capsule has to fit. While it does not we hold (or return to) the
	// crouched pose and re-test every frame, so walking out from under a crate stands you up on its own.
	const bool bHeadBlocked = !bWantsCrouch && CrouchProgress > 0.f && !TryClearStandUpSpace();
	const float Target = (bWantsCrouch || bHeadBlocked) ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(CrouchProgress, Target)) return;
	const float BlendTime = FMath::Max(Target > CrouchProgress ? CrouchDownTime : StandUpTime, 0.01f);
	SetCrouchProgress(FMath::FInterpConstantTo(CrouchProgress, Target, DeltaSeconds, 1.f / BlendTime));
}

bool AALHeroCharacter::TryClearStandUpSpace()
{
	UWorld* W = GetWorld();
	UCapsuleComponent* Cap = GetCapsuleComponent();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!W || !Cap) return true;

	const float Radius = FMath::Max(Cap->GetUnscaledCapsuleRadius() - StandSweepInflation, 1.f);
	const FCollisionShape Standing = FCollisionShape::MakeCapsule(Radius, FMath::Max(StandingHalfHeight - StandSweepInflation, Radius));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ALStandUp), false, this);
	FCollisionResponseParams Response;
	Cap->InitSweepCollisionParams(Params, Response);
	const ECollisionChannel Channel = Cap->GetCollisionObjectType();
	const FQuat Rot = Cap->GetComponentQuat();
	const FVector Here = Cap->GetComponentLocation();

	// Airborne the capsule grows around its centre, so test the standing capsule in place.
	if (!Move || !Move->IsMovingOnGround())
	{
		return !W->OverlapBlockingTestByChannel(Here, Rot, Channel, Standing, Params, Response);
	}

	// On the ground the feet stay planted, so the standing capsule's centre sits higher by the height we gain.
	const float Rise = StandingHalfHeight - Cap->GetUnscaledCapsuleHalfHeight();
	FVector StandAt = Here + FVector(0.f, 0.f, Rise);
	if (!W->OverlapBlockingTestByChannel(StandAt, Rot, Channel, Standing, Params, Response)) return true;

	// Something is just barely overhead. The movement component normally hovers us ~2cm above the floor; settle onto
	// it and try once more before giving up (mirrors the engine's UnCrouch nudge).
	const float FloorDist = Move->CurrentFloor.bBlockingHit ? Move->CurrentFloor.FloorDist : 0.f;
	const float Settle = FloorDist - UE_KINDA_SMALL_NUMBER * 10.f;
	if (Settle <= 0.f) return false;
	StandAt.Z -= Settle;
	if (W->OverlapBlockingTestByChannel(StandAt, Rot, Channel, Standing, Params, Response)) return false;
	Cap->MoveComponent(FVector(0.f, 0.f, -Settle), Rot, false, nullptr, MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
	Move->bForceNextFloorCheck = true;
	return true;
}

void AALHeroCharacter::SetCrouchProgress(float NewProgress)
{
	CrouchProgress = FMath::Clamp(NewProgress, 0.f, 1.f);
	// Ease in/out so the camera settles instead of stopping dead at either end.
	const float Eased = FMath::SmoothStep(0.f, 1.f, CrouchProgress);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		const float NewHalfHeight = FMath::Lerp(StandingHalfHeight, CrouchedHalfHeight, Eased);
		const float Delta = NewHalfHeight - Cap->GetUnscaledCapsuleHalfHeight();
		const bool bGrounded = Move && Move->IsMovingOnGround();
		Cap->SetCapsuleHalfHeight(NewHalfHeight, true);
		// The capsule is centred on the actor, so on the ground every height change is paired with an equal vertical
		// move to keep the feet planted: shrinking never lifts us off the floor and growing never pushes the feet into
		// it (which is what used to wedge the player between floor and ceiling). Shrinking sweeps down onto the floor;
		// growing was already cleared by TryClearStandUpSpace, and each partial capsule sits inside the standing one.
		if (bGrounded && !FMath::IsNearlyZero(Delta))
		{
			Cap->MoveComponent(FVector(0.f, 0.f, Delta), Cap->GetComponentQuat(), Delta < 0.f, nullptr, MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
			if (Move) Move->bForceNextFloorCheck = true;
		}
	}
	ApplyCrouchPose(Eased);
}

void AALHeroCharacter::ApplyCrouchPose(float Eased)
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = FMath::Lerp(StandingSpeed, StandingSpeed * CrouchSpeedScale, Eased);
	}
	// Eye stays inside the capsule in both poses (top is 24cm / 12cm above it), so it can never end up inside a prop.
	if (FPCamera) FPCamera->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(StandingEyeZ, CrouchedEyeZ, Eased)));
	if (HeadMesh) HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(HeadStandZ, HeadCrouchZ, Eased)));
	if (BodyMesh)
	{
		BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(BodyStandZ, BodyCrouchZ, Eased)));
		BodyMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, FMath::Lerp(BodyStandScaleZ, BodyCrouchScaleZ, Eased)));
	}
}
