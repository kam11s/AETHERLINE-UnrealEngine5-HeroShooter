#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ALTypes.h"
#include "ALGameMode.generated.h"
class AALHeroCharacter;
UCLASS()
class AETHERLINE_API AALGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AALGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	UFUNCTION(BlueprintCallable) void StartPlaylist(EALPlaylist InPlaylist);
	UFUNCTION(BlueprintCallable) void SpawnBots();
	UFUNCTION(BlueprintCallable) void StartBattleRoyale();
	// Called by the authoritative damage path when a hero's health reaches zero.
	UFUNCTION(BlueprintCallable) void OnHeroKilled(AALHeroCharacter* Killer, AALHeroCharacter* Victim);
	// Play Again: only acts while the match is over.
	UFUNCTION(BlueprintCallable) void RequestPlayAgain();
	UFUNCTION(BlueprintPure) bool IsScoredPlaylist(EALPlaylist InPlaylist) const;
	UFUNCTION(BlueprintPure) FVector FindSpawnLocation(EALTeam Team) const;
	UPROPERTY(EditAnywhere) float MatchTimeSeconds = 360.f;
	UPROPERTY(EditAnywhere) int32 ScoreToWin = 25;
	UPROPERTY(EditAnywhere) int32 BotFill = 7;
	UPROPERTY(EditAnywhere) int32 BRPlayers = 21;
	UPROPERTY(EditAnywhere) int32 BRSquadSize = 3;
	// Auto-rematch fallback after match over. <= 0 waits for the Play Again key.
	UPROPERTY(EditAnywhere) float RematchDelaySeconds = 12.f;
protected:
	void SpawnArenaIfMissing();
	void CheckMatchEnd();
	void RestartSkirmish();
	float RematchTimer = 0.f;
};
