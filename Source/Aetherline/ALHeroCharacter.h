#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ALTypes.h"
#include "ALHeroCharacter.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;

UCLASS()
class AETHERLINE_API AALHeroCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AALHeroCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	UFUNCTION(BlueprintCallable) void ApplyHero(EALHero Hero);
	UFUNCTION(BlueprintCallable) void FireOnce();
	UFUNCTION(BlueprintCallable) void ActivateSlot(int32 Slot);
	UFUNCTION(BlueprintPure) float GetHealth() const;
	UFUNCTION(BlueprintPure) float GetMaxHealth() const;
	UFUNCTION(BlueprintPure) bool IsAlive() const { return GetHealth() > 0.f; }
	UFUNCTION(Server, Reliable) void ServerApplyDamageTo(AALHeroCharacter* Target, float Amount);
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> FPCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMeshComponent> FPMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALHero HeroId = EALHero::Wraith;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EALTeam TeamId = EALTeam::Ally;
	UPROPERTY(Replicated) float UltCharge = 0.f;
protected:
	void InitAbilityActorInfo();
	void Move2D(const struct FInputActionValue& Value);
	void Look2D(const struct FInputActionValue& Value);
	void OnJump(); void OnFire(); void OnPrimary(); void OnSecondary(); void OnUlt();
	void MoveForward(float V); void MoveRight(float V);
	void LookYaw(float V); void LookPitch(float V);
	void LookYawGamepad(float V); void LookPitchGamepad(float V);
	void FireAxis(float V); void HeroPrev(); void HeroNext();
	UPROPERTY(EditAnywhere, Category="Input") float GamepadLookYawRate = 120.f;
	UPROPERTY(EditAnywhere, Category="Input") float GamepadLookPitchRate = 80.f;
	UPROPERTY(EditAnywhere, Category="Input") float StickDeadZone = 0.24f;
	UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<class UInputMappingContext> IMC;
	UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<class UInputAction> IA_Move;
	UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<class UInputAction> IA_Look;
	UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<class UInputAction> IA_Jump;
	UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<class UInputAction> IA_Fire;
	UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<class UInputAction> IA_Primary;
	UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<class UInputAction> IA_Secondary;
	UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<class UInputAction> IA_Ult;
	float FireCooldown = 0.f;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
