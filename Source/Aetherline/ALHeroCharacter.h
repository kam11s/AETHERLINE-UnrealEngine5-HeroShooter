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
	// Player fire: one shot down the camera axis plus view / viewmodel kick.
	UFUNCTION(BlueprintCallable) void FireOnce();
	// Fires one hitscan shot along Dir. Returns false if on cooldown or dead. Used by both player input and bots.
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
	// Viewmodel pivot under the camera. Bob / sway / kick move this, the gun parts hang off it.
	// GunRoot space: X forward along the barrel, origin at the rear-bottom corner of the receiver.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> GunRoot;
	// Receiver body. Kept as "GunMesh" so existing references / Details-panel layouts still work.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunGrip;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunMagazine;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunForend;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunBarrel;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunMuzzle;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunRail;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunRearSight;
	// Front sight post on the barrel tip.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> GunSight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALHero HeroId = EALHero::Wraith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALTeam TeamId = EALTeam::Ally;
	UPROPERTY(Replicated) float UltCharge = 0.f;
	UPROPERTY(Replicated) bool bOnDropship = false;
	UPROPERTY(Replicated) bool bSkydiving = false;
	UPROPERTY(EditAnywhere) int32 SquadId = 0;
	// World time stamps the HUD reads for damage flash / hit-marker feedback.
	float LastDamagedTime = -100.f;
	float LastHitConfirmTime = -100.f;
	void AttachToDropship(AActor* Ship);
	void DeployFromDropship();

	// Rest pose of GunRoot relative to FPCamera. X must stay > ~20 so the receiver's rear face never crosses the
	// 10cm near clip, even at full recoil kick-back (KickBackCm). Negative yaw angles the barrel in toward the
	// crosshair; the slight downward pitch drops the muzzle below it and shows more of the gun's top face.
	UPROPERTY(EditAnywhere, Category="Viewmodel") FVector GunRestLocation = FVector(33.f, 17.f, -13.f);
	UPROPERTY(EditAnywhere, Category="Viewmodel") FRotator GunRestRotation = FRotator(-2.f, -6.f, 0.f);
	UPROPERTY(EditAnywhere, Category="Viewmodel") float BobAmplitude = 1.1f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float BobFrequency = 9.5f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwayScale = 0.012f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwayMaxDeg = 4.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float SwaySpeed = 9.f;
	// Viewmodel kick is a damped spring. Each shot adds a velocity impulse sized so a full-heft shot peaks at 1.0,
	// so KickBackCm / KickPitchDeg read as "how far a heavy hitter shoves the gun"; the carbine lands around a third
	// of that. The spring ramps to its peak over a few frames and settles with a slight overshoot instead of
	// snapping to full offset and fading, which is what makes full-auto read as a living rhythm.
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickBackCm = 6.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickPitchDeg = 7.f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickFrequencyHz = 4.5f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickDampingRatio = 0.55f;
	// Sideways share of the viewmodel kick (lateral shove, yaw, roll), as a fraction of the vertical kick. Each shot
	// re-rolls the side so sustained fire wanders slightly instead of pumping in a straight line.
	UPROPERTY(EditAnywhere, Category="Viewmodel") float KickSideFraction = 0.1f;
	UPROPERTY(EditAnywhere, Category="Viewmodel") float LandDipCm = 6.f;

	// Camera kick per shot, light hitters -> heavy hitters. It is a control-rotation offset that eases in at
	// ViewKickSnapSpeed and eases back out at ViewKickRecoverSpeed, so the crosshair climbs a touch and comes home on
	// its own (Halo-style) rather than leaving the aim point permanently higher. ViewKickClimbMaxDeg caps how far
	// sustained full-auto can walk the view up.
	UPROPERTY(EditAnywhere, Category="Recoil") float ViewKickLightDeg = 0.15f;
	UPROPERTY(EditAnywhere, Category="Recoil") float ViewKickHeavyDeg = 0.35f;
	UPROPERTY(EditAnywhere, Category="Recoil") float ViewKickYawFraction = 0.1f;
	UPROPERTY(EditAnywhere, Category="Recoil") float ViewKickSnapSpeed = 30.f;
	UPROPERTY(EditAnywhere, Category="Recoil") float ViewKickRecoverSpeed = 7.f;
	UPROPERTY(EditAnywhere, Category="Recoil") float ViewKickClimbMaxDeg = 1.2f;

	// Crouch is a blend, not a snap: capsule half-height, FP eye height and walk speed all interpolate between the
	// Standing* / Crouched* values over CrouchDownTime / StandUpTime. Standing up is gated on a capsule overlap test
	// so the player is never grown into a ceiling or crate gap; while blocked they stay crouched and stand as soon
	// as the space is clear.
	UPROPERTY(EditAnywhere, Category="Crouch") float StandingHalfHeight = 88.f;
	UPROPERTY(EditAnywhere, Category="Crouch") float CrouchedHalfHeight = 52.f;
	UPROPERTY(EditAnywhere, Category="Crouch") float StandingEyeZ = 64.f;
	UPROPERTY(EditAnywhere, Category="Crouch") float CrouchedEyeZ = 40.f;
	UPROPERTY(EditAnywhere, Category="Crouch") float CrouchDownTime = 0.22f;
	UPROPERTY(EditAnywhere, Category="Crouch") float StandUpTime = 0.26f;
	UPROPERTY(EditAnywhere, Category="Crouch") float CrouchSpeedScale = 0.47f;
	// "CrouchHold" action (gamepad B by default): true = crouch only while held, false = toggle like mouse + keyboard.
	UPROPERTY(EditAnywhere, Category="Crouch") bool bGamepadHoldToCrouch = true;

	FVector GetMuzzleLocation() const;

protected:
	void OnJump();
	void OnFire();
	void OnFireReleased();
	// "Crouch" action: toggle (mouse + keyboard). "CrouchHold" action: hold or toggle per bGamepadHoldToCrouch.
	void OnCrouchToggle();
	void OnCrouchHoldPressed();
	void OnCrouchHoldReleased();
	void SetWantsCrouch(bool bCrouch);
	void UpdateCrouch(float DeltaSeconds);
	void SetCrouchProgress(float NewProgress);
	void ApplyCrouchPose(float Eased);
	// True when the full standing capsule fits where it would end up. May settle the capsule onto the floor first.
	bool TryClearStandUpSpace();
	// Called for every shot the local player fires: shoves the viewmodel spring and queues the camera kick.
	void AddFireKick(float Heft);
	void UpdateViewKick(float DeltaSeconds);
	void UpdateViewmodel(float DeltaSeconds);
	// Constructor-only: creates one gun part under GunRoot. Size is in cm (BasicShapes are 100cm).
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
	// BasicShapeMaterial, found in the constructor; tinted per part in BeginPlay.
	UPROPERTY() TObjectPtr<UMaterialInterface> GunBaseMaterial;
	float FireCooldown = 0.f;
	bool bFireHeld = false;
	// Requested crouch state (input) vs. blend progress 0 = standing, 1 = crouched. They differ mid-blend and while
	// standing up is blocked by geometry.
	bool bWantsCrouch = false;
	float CrouchProgress = 0.f;
	// Hero walk speed; MaxWalkSpeed is derived from this and the crouch blend.
	float StandingSpeed = 600.f;
	EALTeam VisualTeam = EALTeam::None;
	// Viewmodel kick spring: 1.0 = a full-heft shot's peak. KickSide is the current shot's sideways direction (-1..1).
	float GunKick = 0.f;
	float GunKickVel = 0.f;
	float KickSide = 0.f;
	// Camera kick: the offset currently applied to the control rotation and the value it is chasing (degrees).
	float ViewKickPitch = 0.f;
	float ViewKickYaw = 0.f;
	float ViewKickPitchTarget = 0.f;
	float ViewKickYawTarget = 0.f;
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
