#include "ALHeroCharacter.h"
#include "ALHeroCatalog.h"
#include "ALGameMode.h"
#include "ALGameState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

AALHeroCharacter::AALHeroCharacter()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->JumpZVelocity = 520.f;
	FPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPCamera"));
	FPCamera->SetupAttachment(GetCapsuleComponent());
	FPCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	FPCamera->bUsePawnControlRotation = true;

	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(FPCamera);
	GunMesh->SetRelativeLocation(FVector(42.f, 18.f, -14.f));
	GunMesh->SetRelativeRotation(FRotator(0.f, 4.f, 0.f));
	GunMesh->SetRelativeScale3D(FVector(0.85f, 0.22f, 0.18f));
	GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube)
	{
		GunMesh->SetStaticMesh(Cube);
	}

	// Capsule is 176 tall: cylinder spans -88..+40, head cube sits 42..82.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -24.f));
	BodyMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, 1.28f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetOwnerNoSee(true);
	if (UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		BodyMesh->SetStaticMesh(Cylinder);
	}

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 62.f));
	HeadMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.4f));
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetOwnerNoSee(true);
	if (Cube)
	{
		HeadMesh->SetStaticMesh(Cube);
	}
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
FVector AALHeroCharacter::GetEyeLocation() const
{
	return FPCamera ? FPCamera->GetComponentLocation() : GetActorLocation() + FVector(0.f, 0.f, 64.f);
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
	if (TeamId != VisualTeam) RefreshTeamVisuals();
	if (bSkydiving && GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround()) bSkydiving = false;
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
	PlayerInputComponent->BindAction(TEXT("HeroPrev"), IE_Pressed, this, &AALHeroCharacter::HeroPrev);
	PlayerInputComponent->BindAction(TEXT("HeroNext"), IE_Pressed, this, &AALHeroCharacter::HeroNext);
}
void AALHeroCharacter::OnJump() { if (bOnDropship || bSkydiving) DeployFromDropship(); else Jump(); }
void AALHeroCharacter::AttachToDropship(AActor* Ship) { if (!Ship) return; bOnDropship = true; AttachToActor(Ship, FAttachmentTransformRules::SnapToTargetNotIncludingScale); }
void AALHeroCharacter::DeployFromDropship() { bOnDropship = false; bSkydiving = true; DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); LaunchCharacter(FVector(0.f,0.f,-800.f)+GetActorForwardVector()*400.f,true,true); }
void AALHeroCharacter::OnFire() { FireOnce(); }
void AALHeroCharacter::MoveForward(float V) { if (FMath::Abs(V) > StickDeadZone) AddMovementInput(GetActorForwardVector(), V); }
void AALHeroCharacter::MoveRight(float V) { if (FMath::Abs(V) > StickDeadZone) AddMovementInput(GetActorRightVector(), V); }
void AALHeroCharacter::LookYaw(float V) { if (!FMath::IsNearlyZero(V)) AddControllerYawInput(V); }
void AALHeroCharacter::LookPitch(float V) { if (!FMath::IsNearlyZero(V)) AddControllerPitchInput(V); }
void AALHeroCharacter::LookYawGamepad(float V) { if (FMath::Abs(V) > StickDeadZone && GetWorld()) AddControllerYawInput(V * GamepadLookYawRate * GetWorld()->GetDeltaSeconds()); }
void AALHeroCharacter::LookPitchGamepad(float V) { if (FMath::Abs(V) > StickDeadZone && GetWorld()) AddControllerPitchInput(V * GamepadLookPitchRate * GetWorld()->GetDeltaSeconds()); }
void AALHeroCharacter::FireAxis(float V) { if (V >= 0.45f) FireOnce(); }
void AALHeroCharacter::HeroPrev() { if (IsAlive()) ApplyHero(static_cast<EALHero>((static_cast<int32>(HeroId)+5)%6)); }
void AALHeroCharacter::HeroNext() { if (IsAlive()) ApplyHero(static_cast<EALHero>((static_cast<int32>(HeroId)+1)%6)); }
void AALHeroCharacter::FireOnce()
{
	const FVector Dir = FPCamera ? FPCamera->GetForwardVector() : GetActorForwardVector();
	if (FireShot(Dir, 1.f)) AddControllerPitchInput(-0.12f);
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
	// Teal = friendly fire lanes, amber = hostile, so incoming fire reads at a glance.
	const FColor Tracer = (TeamId == EALTeam::Enemy) ? FColor(255, 150, 40) : FColor(40, 220, 255);
	DrawDebugLine(GetWorld(), Start + Dir * 40.f, TracerEnd, Tracer, false, 0.08f, 0, 2.0f);
	if (bHit)
	{
		AALHeroCharacter* Other = Cast<AALHeroCharacter>(Hit.GetActor());
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, Other ? 18.f : 10.f, Other ? FColor::White : FColor(255, 160, 40), false, 0.12f);
		if (Other && Other->TeamId != TeamId) ServerApplyDamageTo(Other, Def.Damage * DamageScale);
	}
	return true;
}
void AALHeroCharacter::ServerApplyDamageTo_Implementation(AALHeroCharacter* Target, float Amount)
{
	if (!HasAuthority() || !Target || !Target->IsAlive()) return;
	if (Target->TeamId == TeamId) return;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Target->Health = FMath::Max(0.f, Target->Health - Amount);
	Target->LastDamagedTime = Now;
	LastHitConfirmTime = Now;
	if (!Target->IsAlive()) Target->HandleDeath(this);
}
void AALHeroCharacter::HandleDeath(AALHeroCharacter* Killer)
{
	UWorld* W = GetWorld();
	if (!W) return;
	bool bBattleRoyale = false;
	if (AALGameState* GS = W->GetGameState<AALGameState>())
	{
		bBattleRoyale = GS->IsBattleRoyale();
		if (Killer && Killer->TeamId == EALTeam::Ally) ++GS->AllyScore;
		else if (Killer && Killer->TeamId == EALTeam::Enemy) ++GS->EnemyScore;
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	// Battle Royale eliminations are final; every other playlist respawns in place after a short delay.
	if (bBattleRoyale) return;
	W->GetTimerManager().SetTimer(RespawnHandle, this, &AALHeroCharacter::Respawn, FMath::Max(0.5f, RespawnDelay), false);
}
void AALHeroCharacter::Respawn()
{
	UWorld* W = GetWorld();
	if (!W) return;
	FVector Loc = GetActorLocation() + FVector(0.f, 0.f, 120.f);
	if (const AALGameMode* GM = W->GetAuthGameMode<AALGameMode>()) Loc = GM->FindSpawnLocation(TeamId);
	SetActorEnableCollision(true);
	SetActorHiddenInGame(false);
	TeleportTo(Loc, FRotator(0.f, GetActorRotation().Yaw, 0.f));
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Health = MaxHealth;
	LastDamagedTime = -100.f;
}
