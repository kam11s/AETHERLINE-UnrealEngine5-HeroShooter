#pragma once
#include "CoreMinimal.h"
#include "ALTypes.generated.h"

UENUM(BlueprintType)
enum class EALHero : uint8 { Wraith, Bastion, Pulse, Volt, Grav, Ember };
UENUM(BlueprintType)
enum class EALPlaylist : uint8 { Training, BotSkirmish, QuickPlay, Ranked };
UENUM(BlueprintType)
enum class EALTeam : uint8 { Ally, Enemy, None };

USTRUCT(BlueprintType)
struct FALHeroDef
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EALHero Id = EALHero::Wraith;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DisplayName = TEXT("Wraith");
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxHealth = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveSpeed = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float FireRate = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Range = 20000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Spread = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Color = FLinearColor(0.1f, 0.7f, 0.9f);
};
