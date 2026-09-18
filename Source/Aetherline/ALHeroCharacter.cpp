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
	GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->JumpZVelocity = 520.f;
	GetCharacterMovement()->AirControl = 0.35f;
	FPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPCamera"));
	FPCamera->SetupAttachment(GetCapsuleComponent());
	FPCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	FPCamera->SetFieldOfView(90.f);
	FPCamera->bUsePawnControlRotation = true;

	GunRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GunRoot"));
	GunRoot->SetupAttachment(FPCamera);
	GunRoot->SetRelativeLocation(GunRestLocation);
	GunRoot->SetRelativeRotation(GunRestRotation);
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -24.f));
	BodyMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, 1.28f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetOwnerNoSee(true);
	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 62.f));
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
	GetCharacterMovement()->MaxWalkSpeed = Def.MoveSpeed;
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
	UpdateViewmodel(DeltaSeconds);
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

	GunKick = FMath::FInterpTo(GunKick, 0.f, DeltaSeconds, KickRecoverSpeed);
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

	const float OffX = -GunKick * KickBackCm;
	const float OffY = static_cast<float>(SwayRot.Yaw) * 0.12f + BobY;
	const float OffZ = BobZ + Breath + GunKick * 0.8f - GunLandDip * LandDipCm;
	const float TiltPitch = static_cast<float>(SwayRot.Pitch) + GunKick * KickPitchDeg - GunLandDip * 3.f;
	const FVector Offset(OffX, OffY, OffZ);
	const FRotator Tilt(TiltPitch, static_cast<float>(SwayRot.Yaw), static_cast<float>(SwayRot.Roll));
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
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AALHeroCharacter::OnCrouch);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Released, this, &AALHeroCharacter::OnUnCrouch);
	PlayerInputComponent->BindAction(TEXT("HeroPrev"), IE_Pressed, this, &AALHeroCharacter::HeroPrev);
	PlayerInputComponent->BindAction(TEXT("HeroNext"), IE_Pressed, this, &AALHeroCharacter::HeroNext);
}
void AALHeroCharacter::OnJump() { if (bOnDropship || bSkydiving) DeployFromDropship(); else Jump(); }
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
	if (FireCooldown > 0.f || !IsAlive() || !GetWorld()) return;
	const FALHeroDef Def = UALHeroCatalog::Get(HeroId);
	FireCooldown = 1.f / FMath::Max(Def.FireRate, 0.1f);
	const FVector Start = FPCamera ? FPCamera->GetComponentLocation() : GetActorLocation();
	const FVector Dir = FPCamera ? FPCamera->GetForwardVector() : GetActorForwardVector();
	const FVector End = Start + Dir * Def.Range;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ALFire), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	const FVector TracerEnd = bHit ? Hit.ImpactPoint : End;
	const FVector Muzzle = GetMuzzleLocation();
	DrawDebugLine(GetWorld(), Muzzle, TracerEnd, FColor(40, 220, 255), false, 0.08f, 0, 2.0f);
	DrawDebugPoint(GetWorld(), Muzzle, 22.f, FColor(255, 170, 50), false, 0.05f);
	if (bHit)
	{
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.f, FColor(255, 160, 40), false, 0.12f);
		if (AALHeroCharacter* Other = Cast<AALHeroCharacter>(Hit.GetActor()))
		{
			if (Other->TeamId != TeamId) ServerApplyDamageTo(Other, Def.Damage);
		}
	}
	// Heavier hitters kick harder, both on the viewmodel and on the camera.
	const float Heft = FMath::Clamp(Def.Damage / 70.f, 0.25f, 1.f);
	GunKick = FMath::Min(1.f, GunKick + Heft);
	const float ViewKick = FMath::Clamp(Def.Damage * ViewKickScale, 0.05f, 0.35f);
	AddControllerPitchInput(-ViewKick);
	AddControllerYawInput(FMath::FRandRange(-0.25f, 0.25f) * ViewKick);
}
void AALHeroCharacter::ServerApplyDamageTo_Implementation(AALHeroCharacter* Target, float Amount)
{
	if (!HasAuthority() || !Target || !Target->IsAlive()) return;
	if (Target->TeamId == TeamId) return;
	Target->Health = FMath::Max(0.f, Target->Health - Amount);
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

void AALHeroCharacter::OnCrouch()
{
	if (bHoldCrouch) return;
	bHoldCrouch = true;
	StandingHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	if (UCharacterMovementComponent* M = GetCharacterMovement())
	{
		StandingSpeed = M->MaxWalkSpeed;
		M->MaxWalkSpeed = CrouchedSpeed;
	}
	GetCapsuleComponent()->SetCapsuleHalfHeight(CrouchedHalfHeight);
	if (FPCamera) FPCamera->SetRelativeLocation(FVector(0.f, 0.f, CrouchedHalfHeight - 12.f));
}

void AALHeroCharacter::OnUnCrouch()
{
	if (!bHoldCrouch) return;
	bHoldCrouch = false;
	GetCapsuleComponent()->SetCapsuleHalfHeight(StandingHalfHeight);
	if (UCharacterMovementComponent* M = GetCharacterMovement()) M->MaxWalkSpeed = StandingSpeed;
	if (FPCamera) FPCamera->SetRelativeLocation(FVector(0.f, 0.f, StandingHalfHeight - 12.f));
}
