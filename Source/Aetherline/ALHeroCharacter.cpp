#include "ALHeroCharacter.h"
#include "ALHeroCatalog.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
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
	FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(ALFire), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Dir * Def.Range, ECC_Visibility, Params))
	{
		if (AALHeroCharacter* Other = Cast<AALHeroCharacter>(Hit.GetActor()))
		{
			if (Other->TeamId != TeamId) ServerApplyDamageTo(Other, Def.Damage);
		}
	}
}
void AALHeroCharacter::ServerApplyDamageTo_Implementation(AALHeroCharacter* Target, float Amount)
{
	if (!HasAuthority() || !Target || !Target->IsAlive()) return;
	if (Target->TeamId == TeamId) return;
	Target->Health = FMath::Max(0.f, Target->Health - Amount);
}
