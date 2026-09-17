#pragma once
#include "CoreMinimal.h"
#include "ALTypes.generated.h"

UENUM(BlueprintType)
enum class EALHero : uint8
{
	Wraith,
	Bastion,
	Pulse,
	Volt,
	Grav,
	Ember
};

UENUM(BlueprintType)
enum class EALPlaylist : uint8
{
	Training,
	BotSkirmish,
	QuickPlay,
	Ranked,
	BattleRoyale
};

UENUM(BlueprintType)
enum class EALDropPhase : uint8
{
	Lobby,
	OnDropship,
	Skydiving,
	Grounded
};

UENUM(BlueprintType)
enum class EALTeam : uint8
{
	Ally,
	Enemy,
	None
};

USTRUCT(BlueprintType)
struct FALHeroDef
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EALHero Id = EALHero::Wraith;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxHealth = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveSpeed = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float FireRate = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Range = 20000.f;
};
