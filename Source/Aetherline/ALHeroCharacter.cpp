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
	if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		GunMesh->SetStaticMesh(Cube);
	}
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
	if (HasAuthority() && !IsAlive() && RespawnTimer > 0.f)
	{
		RespawnTimer -= DeltaSeconds;
		if (RespawnTimer <= 0.f) Respawn();
	}
}
void AALHeroCharacter::HandleDeath()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	if (AController* C = GetController()) C->StopMovement();
	// Battle Royale is elimination: no respawn there.
	const AALGameState* GS = GetWorld() ? GetWorld()->GetGameState<AALGameState>() : nullptr;
	RespawnTimer = (GS && GS->IsBattleRoyale()) ? 0.f : FMath::Max(RespawnDelay, 0.01f);
}
void AALHeroCharacter::Respawn()
{
	if (!HasAuthority() || !GetWorld()) return;
	RespawnTimer = 0.f;
	Health = MaxHealth;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Walking);
	}
	FVector Loc(FMath::FRandRange(-1800.f, 1800.f), FMath::FRandRange(-1800.f, 1800.f), 120.f);
	if (const AALGameMode* GM = GetWorld()->GetAuthGameMode<AALGameMode>()) Loc = GM->FindSpawnLocation(TeamId);
	const FRotator Rot(0.f, FMath::FRandRange(-180.f, 180.f), 0.f);
	// TeleportTo nudges out of geometry; if no free spot is found, place directly rather than staying dead in place.
	if (!TeleportTo(Loc, Rot)) SetActorLocationAndRotation(Loc, Rot, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = GetController()) C->SetControlRotation(FRotator(0.f, Rot.Yaw, 0.f));
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
	DrawDebugLine(GetWorld(), Start + Dir * 40.f, TracerEnd, FColor(40, 220, 255), false, 0.08f, 0, 2.0f);
	if (bHit)
	{
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.f, FColor(255, 160, 40), false, 0.12f);
		if (AALHeroCharacter* Other = Cast<AALHeroCharacter>(Hit.GetActor()))
		{
			if (Other->TeamId != TeamId) ServerApplyDamageTo(Other, Def.Damage);
		}
	}
	AddControllerPitchInput(-0.12f);
}
void AALHeroCharacter::ServerApplyDamageTo_Implementation(AALHeroCharacter* Target, float Amount)
{
	if (!HasAuthority() || !Target || !Target->IsAlive()) return;
	if (Target->TeamId == TeamId) return;
	Target->Health = FMath::Max(0.f, Target->Health - Amount);
	if (Target->IsAlive()) return;
	// Health just crossed to zero: this is the single kill event, so score it here exactly once.
	Target->HandleDeath();
	if (AALGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AALGameMode>() : nullptr) GM->OnHeroKilled(this, Target);
}
