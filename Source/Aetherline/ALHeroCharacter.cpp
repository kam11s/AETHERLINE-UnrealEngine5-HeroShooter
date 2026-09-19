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
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

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
	UStaticMesh* Cube = CubeFinder.Object;
	UStaticMesh* Cylinder = CylinderFinder.Object ? CylinderFinder.Object : Cube;
	GunBaseMaterial = BasicMaterialFinder.Object;

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

UStaticMeshComponent* AALHeroCharacter::MakeGunPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Center, const FVector& Size, const FRotator& Rotation)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(GunRoot);
	Part->SetRelativeLocation(Center);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Size / 100.f);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCastShadow(false);
	if (Mesh) Part->SetStaticMesh(Mesh);
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
	UpdateViewKick(DeltaSeconds);
	UpdateViewmodel(DeltaSeconds);
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

	// Camera: queue the climb; UpdateViewKick eases the control rotation toward it and back. Bots keep the
	// viewmodel kick (others can see their gun) but never have their aim pushed around.
	if (!Cast<APlayerController>(GetController())) return;
	const float Kick = FMath::Lerp(ViewKickLightDeg, ViewKickHeavyDeg, Heft);
	ViewKickPitchTarget = FMath::Min(ViewKickPitchTarget + Kick, FMath::Max(ViewKickClimbMaxDeg, Kick));
	ViewKickYawTarget += FMath::FRandRange(-1.f, 1.f) * Kick * ViewKickYawFraction;
}
void AALHeroCharacter::UpdateViewKick(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f || !IsLocallyControlled()) return;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		ViewKickPitch = ViewKickYaw = ViewKickPitchTarget = ViewKickYawTarget = 0.f;
		return;
	}
	const float PrevPitch = ViewKickPitch;
	const float PrevYaw = ViewKickYaw;
	ViewKickPitch = FMath::FInterpTo(ViewKickPitch, ViewKickPitchTarget, DeltaSeconds, ViewKickSnapSpeed);
	ViewKickYaw = FMath::FInterpTo(ViewKickYaw, ViewKickYawTarget, DeltaSeconds, ViewKickSnapSpeed);
	ViewKickPitchTarget = FMath::FInterpTo(ViewKickPitchTarget, 0.f, DeltaSeconds, ViewKickRecoverSpeed);
	ViewKickYawTarget = FMath::FInterpTo(ViewKickYawTarget, 0.f, DeltaSeconds, ViewKickRecoverSpeed);
	const float DeltaPitch = ViewKickPitch - PrevPitch;
	const float DeltaYaw = ViewKickYaw - PrevYaw;
	if (FMath::IsNearlyZero(DeltaPitch, 1e-4f) && FMath::IsNearlyZero(DeltaYaw, 1e-4f)) return;
	// Only the frame-to-frame change is applied, so the player's own look input passes straight through and the
	// pitch limits in the camera manager still apply on the next controller update.
	FRotator Ctrl = PC->GetControlRotation();
	Ctrl.Pitch += DeltaPitch;
	Ctrl.Yaw += DeltaYaw;
	PC->SetControlRotation(Ctrl);
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
void AALHeroCharacter::OnFire() { bFireHeld = true; FireOnce(); }
void AALHeroCharacter::OnFireReleased() { bFireHeld = false; }
void AALHeroCharacter::MoveForward(float V) { if (FMath::Abs(V) > StickDeadZone) AddMovementInput(GetActorForwardVector(), V); }
void AALHeroCharacter::MoveRight(float V) { if (FMath::Abs(V) > StickDeadZone) AddMovementInput(GetActorRightVector(), V); }
void AALHeroCharacter::LookYaw(float V) { if (!FMath::IsNearlyZero(V)) AddControllerYawInput(V); }
void AALHeroCharacter::LookPitch(float V) { if (!FMath::IsNearlyZero(V)) AddControllerPitchInput(V); }
void AALHeroCharacter::LookYawGamepad(float V) { if (FMath::Abs(V) > StickDeadZone && GetWorld()) AddControllerYawInput(V * GamepadLookYawRate * GetWorld()->GetDeltaSeconds()); }
void AALHeroCharacter::LookPitchGamepad(float V) { if (FMath::Abs(V) > StickDeadZone && GetWorld()) AddControllerPitchInput(V * GamepadLookPitchRate * GetWorld()->GetDeltaSeconds()); }
void AALHeroCharacter::FireAxis(float V) { if (V >= 0.45f) FireOnce(); }
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
