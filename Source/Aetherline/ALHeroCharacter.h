#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ALTypes.h"
#include "ALHeroCharacter.generated.h"

class UCameraComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class AETHERLINE_API AALHeroCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AALHeroCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable) void ApplyHero(EALHero Hero);
	UFUNCTION(BlueprintCallable) void FireOnce();
	UFUNCTION(BlueprintPure) float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure) float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure) bool IsAlive() const { return Health > 0.f; }
	UFUNCTION(Server, Reliable) void ServerApplyDamageTo(AALHeroCharacter* Target, float Amount);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> FPCamera;
	// Viewmodel pivot under the camera. Bob / sway / kick move this, the gun parts hang off it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> GunRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunBarrel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunSight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALHero HeroId = EALHero::Wraith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALTeam TeamId = EALTeam::Ally;
	UPROPERTY(Replicated) float UltCharge = 0.f;
	UPROPERTY(Replicated) bool bOnDropship = false;
	UPROPERTY(Replicated) bool bSkydiving = false;
	UPROPERTY(EditAnywhere) int32 SquadId = 0;
	void AttachToDropship(AActor* Ship);
	void DeployFromDropship();

	// Rest pose of GunRoot relative to FPCamera. X must stay > ~20 so the body never crosses the 10cm near clip.
	UPROPERTY(EditAnywhere, Category="Viewmodel") FVector GunRestLocation = FVector(26.f, 17.f, -13.f);
	UPROPERTY(EditAnywhere, Category="Viewmodel") FRotator GunRestRotation = FRotator(0.f, -3.f, 0.f);
	UPROPERTY(EditAnywhere, Category="Viewmodel") float BobAmplitude = 1.1f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float BobFrequency = 9.5f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwayScale = 0.012f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwayMaxDeg = 4.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwaySpeed = 9.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickBackCm = 5.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickPitchDeg = 5.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickRecoverSpeed = 14.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float LandDipCm = 6.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float ViewKickScale = 0.005f;

	FVector GetMuzzleLocation() const;

protected:
	void OnJump();
	void OnFire();
	void OnFireReleased();
	void UpdateViewmodel(float DeltaSeconds);
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
	bool bFireHeld = false;
	float GunKick = 0.f;
	float GunLandDip = 0.f;
	float BobTime = 0.f;
	float BobBlend = 0.f;
	float LastControlYaw = 0.f;
	float LastControlPitch = 0.f;
	FRotator SwayRot = FRotator::ZeroRotator;
	float GamepadLookYawRate = 120.f;
	float GamepadLookPitchRate = 80.f;
	float StickDeadZone = 0.24f;
};
