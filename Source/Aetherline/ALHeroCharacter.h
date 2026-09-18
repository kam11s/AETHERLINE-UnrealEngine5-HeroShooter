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
	void OnCrouch();
	void OnUnCrouch();
	void UpdateViewmodel(float DeltaSeconds);
	// Constructor-only: creates one gun part under GunRoot. Size is in cm (BasicShapes are 100cm).
	UStaticMeshComponent* MakeGunPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Center, const FVector& Size, const FRotator& Rotation);
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
	bool bHoldCrouch = false;
	float StandingHalfHeight = 88.f;
	float CrouchedHalfHeight = 52.f;
	float StandingSpeed = 600.f;
	float CrouchedSpeed = 280.f;
	EALTeam VisualTeam = EALTeam::None;
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
