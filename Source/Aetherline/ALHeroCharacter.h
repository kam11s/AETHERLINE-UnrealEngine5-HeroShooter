#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ALTypes.h"
#include "ALHeroCharacter.generated.h"

class UCameraComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

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
	bool FireShot(const FVector& Dir, float DamageScale);
	FVector GetEyeLocation() const;
	UFUNCTION(BlueprintPure) float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure) float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure) bool IsAlive() const { return Health > 0.f; }
	UFUNCTION(Server, Reliable) void ServerApplyDamageTo(AALHeroCharacter* Target, float Amount);
	void RefreshTeamVisuals();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> FPCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> HeadMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> GunRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunGrip;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunMagazine;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunForend;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunBarrel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunMuzzle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunRail;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunRearSight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunSight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALHero HeroId = EALHero::Wraith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALTeam TeamId = EALTeam::Ally;
	UPROPERTY(Replicated) float UltCharge = 0.f;
	UPROPERTY(Replicated) bool bOnDropship = false;
	UPROPERTY(Replicated) bool bSkydiving = false;
	UPROPERTY(EditAnywhere) int32 SquadId = 0;
	float LastDamagedTime = -100.f;
	float LastHitConfirmTime = -100.f;
	UPROPERTY(EditAnywhere, Category="Combat") float RespawnDelay = 2.f;
	void HandleDeath();
	void Respawn();
	void AttachToDropship(AActor* Ship);
	void DeployFromDropship();

	UPROPERTY(EditAnywhere, Category="Viewmodel") FVector GunRestLocation = FVector(33.f, 17.f, -13.f);
	UPROPERTY(EditAnywhere, Category="Viewmodel") FRotator GunRestRotation = FRotator(-2.f, -6.f, 0.f);
	UPROPERTY(EditAnywhere, Category="Viewmodel") float BobAmplitude = 1.1f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float BobFrequency = 9.5f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwayScale = 0.012f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwayMaxDeg = 4.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwaySpeed = 9.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickBackCm = 6.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickPitchDeg = 7.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickFrequencyHz = 4.5f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickDampingRatio = 0.55f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickSideFraction = 0.1f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float LandDipCm = 6.f;

	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilPitchLightDeg = 0.4f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilPitchHeavyDeg = 1.3f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilPitchNoise = 0.2f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilYawNoiseDeg = 0.3f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilClimbMaxDeg = 3.f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilClimbSoftStart = 0.45f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilSnapSpeed = 40.f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilRecoverDelay = 0.12f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilRecoverSpeed = 9.f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilRecoverMinDegPerSec = 8.f;

	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistStrengthPad = 1.f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistStrengthMnK = 0.25f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistConeDeg = 8.f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistFullRangeCm = 1800.f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistMaxRangeCm = 4500.f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistPullPerSec = 5.f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistMaxPullDegPerSec = 45.f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistBodyDeadFraction = 0.6f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistTrackStrength = 0.6f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistFrictionPad = 0.35f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistFrictionMnK = 0.f;
	UPROPERTY(EditAnywhere, Category="AimAssist") float AssistIdleScale = 0.25f;

	UPROPERTY(EditAnywhere, Category="Crouch") float StandingHalfHeight = 88.f;
	UPROPERTY(EditAnywhere, Category="Crouch") float CrouchedHalfHeight = 52.f;
	UPROPERTY(EditAnywhere, Category="Crouch") float StandingEyeZ = 64.f;
	UPROPERTY(EditAnywhere, Category="Crouch") float CrouchedEyeZ = 40.f;
	UPROPERTY(EditAnywhere, Category="Crouch") float CrouchDownTime = 0.22f;
	UPROPERTY(EditAnywhere, Category="Crouch") float StandUpTime = 0.26f;
	UPROPERTY(EditAnywhere, Category="Crouch") float CrouchSpeedScale = 0.47f;
	UPROPERTY(EditAnywhere, Category="Crouch") bool bGamepadHoldToCrouch = true;

	FVector GetMuzzleLocation() const;

protected:
	void OnJump();
	void OnFire();
	void OnFireReleased();
	void OnCrouchToggle();
	void OnCrouchHoldPressed();
	void OnCrouchHoldReleased();
	void SetWantsCrouch(bool bCrouch);
	void UpdateCrouch(float DeltaSeconds);
	void SetCrouchProgress(float NewProgress);
	void ApplyCrouchPose(float Eased);
	bool TryClearStandUpSpace();
	void AddFireKick(float Heft);
	void UpdateAimRecoil(float DeltaSeconds);
	void UpdateAimAssist(float DeltaSeconds);
	AALHeroCharacter* FindAssistTarget(const FVector& Eye, const FVector& Fwd, FVector& OutAimDir, float& OutEdgeDeg, float& OutBodyDeg, float& OutDist) const;
	void NoteInputDevice(bool bGamepad);
	float AssistLookScale() const;
	void UpdateViewmodel(float DeltaSeconds);
	UStaticMeshComponent* MakeGunPart(const TCHAR* Name, UStaticMesh* InMesh, const FVector& Center, const FVector& Size, const FRotator& Rotation);
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
	UPROPERTY() TObjectPtr<UMaterialInterface> GunBaseMaterial;
	float FireCooldown = 0.f;
	bool bFireHeld = false;
	bool bWantsCrouch = false;
	float CrouchProgress = 0.f;
	float StandingSpeed = 600.f;
	EALTeam VisualTeam = EALTeam::None;
	float GunKick = 0.f;
	float GunKickVel = 0.f;
	float KickSide = 0.f;
	float RecoilPitchTarget = 0.f;
	float RecoilYawTarget = 0.f;
	float RecoilPitchApplied = 0.f;
	float RecoilYawApplied = 0.f;
	float RecoilPitchMark = 0.f;
	float RecoilYawMark = 0.f;
	bool bRecoilMarkValid = false;
	float LastShotTime = -100.f;
	TWeakObjectPtr<AALHeroCharacter> AssistTarget;
	FVector AssistPrevCenterDir = FVector::ZeroVector;
	float AssistWeight = 0.f;
	float LookInputThisFrame = 0.f;
	bool bUsingGamepad = false;
	float GunLandDip = 0.f;
	float BobTime = 0.f;
	float BobBlend = 0.f;
	float LastControlYaw = 0.f;
	float LastControlPitch = 0.f;
	FRotator SwayRot = FRotator::ZeroRotator;
	float GamepadLookYawRate = 120.f;
	float GamepadLookPitchRate = 80.f;
	float StickDeadZone = 0.24f;
	float RespawnTimer = -1.f;
};
