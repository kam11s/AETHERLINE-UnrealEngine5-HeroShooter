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

	// Aim recoil. Every shot pushes the control rotation - the crosshair and the direction the next shot leaves on -
	// up by RecoilPitchLightDeg (carbine class) .. RecoilPitchHeavyDeg (Bastion), +-RecoilPitchNoise of that, plus
	// +-RecoilYawNoiseDeg of yaw, eased in over a few frames at RecoilSnapSpeed. Sustained fire climbs progressively:
	// once the accumulated climb passes RecoilClimbSoftStart x RecoilClimbMaxDeg each shot's kick shrinks, so a spray
	// flattens out at the cap instead of walking to the sky (and the player has to pull down against it).
	// RecoilRecoverDelay after the last shot the view drifts back at RecoilRecoverSpeed (fraction of the remaining
	// climb per second, never slower than RecoilRecoverMinDegPerSec). Pitch the player - or aim assist - pulls down
	// mid-burst counts as compensation and is taken off the climb, so recovery never dips below the point they held.
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilPitchLightDeg = 0.8f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilPitchHeavyDeg = 2.6f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilPitchNoise = 0.2f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilYawNoiseDeg = 0.3f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilClimbMaxDeg = 6.f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilClimbSoftStart = 0.45f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilSnapSpeed = 40.f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilRecoverDelay = 0.12f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilRecoverSpeed = 9.f;
	UPROPERTY(EditAnywhere, Category="Recoil") float RecoilRecoverMinDegPerSec = 8.f;

	// Aim assist (Rivals-style soft magnetism). Each frame the closest live enemy hitbox inside AssistConeDeg of the
	// crosshair, with line of sight and within AssistMaxRangeCm, pulls the control rotation toward itself: the pull
	// closes AssistPullPerSec of the remaining angle per second (capped at AssistMaxPullDegPerSec), fades to nothing
	// at the cone edge and between AssistFullRangeCm and AssistMaxRangeCm, and stops once the crosshair is over the
	// body (AssistBodyDeadFraction of the capsule's angular radius), so it settles on the target rather than
	// centre-locking it. AssistTrackStrength is the share of the target's apparent motion (their strafe or ours)
	// the view rides along with; AssistFriction* slow stick look while over a target. Everything is scaled by
	// AssistStrengthPad on gamepad or AssistStrengthMnK on mouse + keyboard (last look / fire device wins), and
	// drops to AssistIdleScale when nobody - player, look stick, target - is moving, so a still reticle is never
	// dragged onto a still enemy.
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
	// Called for every shot the local player fires: shoves the viewmodel spring and queues the aim recoil.
	void AddFireKick(float Heft);
	// Eases queued recoil into the control rotation, credits player / assist pull-down against it, recovers after a burst.
	void UpdateAimRecoil(float DeltaSeconds);
	// Soft magnetism toward the best enemy hitbox near the crosshair. Runs after recoil so its pull counts as compensation.
	void UpdateAimAssist(float DeltaSeconds);
	// Picks the enemy the assist should work on. Returns null when nothing qualifies; out params describe the pull.
	AALHeroCharacter* FindAssistTarget(const FVector& Eye, const FVector& Fwd, FVector& OutAimDir, float& OutEdgeDeg, float& OutBodyDeg, float& OutDist) const;
	// Remembers which device the player is aiming / firing with so the assist can pick its strength.
	void NoteInputDevice(bool bGamepad);
	// Look-input multiplier for assist friction: 1 with no target, down to 1 - AssistFriction* over one.
	float AssistLookScale() const;
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
	// Aim recoil, in degrees of control rotation. *Target is the climb the burst has earned (minus what has been
	// compensated or recovered), *Applied is how much of it has actually been written into the control rotation.
	// RecoilPitchMark / RecoilYawMark are the control rotation right after our write, so next frame's difference is
	// whatever the player, the aim assist or the pitch clamp moved on top.
	float RecoilPitchTarget = 0.f;
	float RecoilYawTarget = 0.f;
	float RecoilPitchApplied = 0.f;
	float RecoilYawApplied = 0.f;
	float RecoilPitchMark = 0.f;
	float RecoilYawMark = 0.f;
	bool bRecoilMarkValid = false;
	float LastShotTime = -100.f;
	// Aim assist state: current target (for hysteresis + tracking), the world direction to its centre last frame,
	// and the 0..1 weight the friction reads.
	TWeakObjectPtr<AALHeroCharacter> AssistTarget;
	FVector AssistPrevCenterDir = FVector::ZeroVector;
	float AssistWeight = 0.f;
	// Look input seen this frame (stick or mouse) so the assist knows the player is actively aiming.
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
};
