#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ALTypes.h"
#include "ALHeroCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;

UCLASS()
class AETHERLINE_API AALHeroCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AALHeroCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable) void ApplyHero(EALHero Hero);
	UFUNCTION(BlueprintCallable) void FireOnce();
	UFUNCTION(BlueprintPure) float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure) float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure) bool IsAlive() const { return Health > 0.f; }
	UFUNCTION(Server, Reliable) void ServerApplyDamageTo(AALHeroCharacter* Target, float Amount);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> FPCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunMesh;
	// Third-person body seen by everyone except the owning player (bOwnerNoSee keeps the FP view clean).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> HeadMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALHero HeroId = EALHero::Wraith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALTeam TeamId = EALTeam::Ally;
	UPROPERTY(Replicated) float UltCharge = 0.f;
	UPROPERTY(Replicated) bool bOnDropship = false;
	UPROPERTY(Replicated) bool bSkydiving = false;
	UPROPERTY(EditAnywhere) int32 SquadId = 0;
	UPROPERTY(EditAnywhere) float RespawnDelay = 4.f;
	// World time stamps the HUD reads for damage flash / hit-marker feedback.
	float LastDamagedTime = -100.f;
	float LastHitConfirmTime = -100.f;
	void AttachToDropship(AActor* Ship);
	void DeployFromDropship();
	FVector GetEyeLocation() const;
	// Fires one hitscan shot along Dir. Returns false if on cooldown or dead. Used by both player input and bots.
	bool FireShot(const FVector& Dir, float DamageScale);
	void HandleDeath(AALHeroCharacter* Killer);
	void Respawn();

protected:
	void RefreshTeamVisuals();
	EALTeam VisualTeam = EALTeam::None;
	FTimerHandle RespawnHandle;
	void OnJump();
	void OnFire();
	void MoveForward(float V);
	void MoveRight(float V);
	void LookYaw(float V);
	void LookPitch(float V);
	void LookYawGamepad(float V);
	void LookPitchGamepad(float V);
	void FireAxis(float V);
	void HeroPrev();
	void HeroNext();

	UPROPERTY(Replicated) float Health = 200.f;
	UPROPERTY() float MaxHealth = 200.f;
	float FireCooldown = 0.f;
	float GamepadLookYawRate = 120.f;
	float GamepadLookPitchRate = 80.f;
	float StickDeadZone = 0.24f;
};
