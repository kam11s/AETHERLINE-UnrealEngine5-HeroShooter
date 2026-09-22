#include "ALHeroCharacter.h"
#include "ALHeroCatalog.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// Gun parts in GunRoot space (cm). Cube.Cube is 100cm, so scale = size / 100.
	const FVector GunBodySize(44.f, 15.f, 18.f);
	const FVector GunBodyCenter(20.f, 0.f, 0.f);
	const FVector GunBarrelSize(36.f, 6.f, 6.f);
	const FVector GunBarrelCenter(56.f, 0.f, 3.f);
	const FVector GunSightSize(6.f, 3.f, 3.f);
	const FVector GunSightCenter(12.f, 0.f, 10.5f);
	const FVector GunMuzzleLocal(74.f, 0.f, 3.f);

	void SetupGunPart(UStaticMeshComponent* Part, UStaticMesh* Cube, const FVector& Center, const FVector& Size)
	{
		if (!Part) return;
		Part->SetRelativeLocation(Center);
		Part->SetRelativeScale3D(Size / 100.f);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCastShadow(false);
		if (Cube) Part->SetStaticMesh(Cube);
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

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(GunRoot);
	SetupGunPart(GunMesh, Cube, GunBodyCenter, GunBodySize);
	GunBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunBarrel"));
	GunBarrel->SetupAttachment(GunRoot);
	SetupGunPart(GunBarrel, Cube, GunBarrelCenter, GunBarrelSize);
	GunSight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunSight"));
	GunSight->SetupAttachment(GunRoot);
	SetupGunPart(GunSight, Cube, GunSightCenter, GunSightSize);
}
void AALHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	const FRotator Ctrl = GetControlRotation();
	LastControlYaw = static_cast<float>(Ctrl.Yaw);
	LastControlPitch = static_cast<float>(Ctrl.Pitch);
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		TintGunPart(GunMesh, Base, this, FLinearColor(0.035f, 0.045f, 0.055f));
		TintGunPart(GunBarrel, Base, this, FLinearColor(0.05f, 0.22f, 0.26f));
		TintGunPart(GunSight, Base, this, FLinearColor(0.89f, 0.60f, 0.18f));
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
