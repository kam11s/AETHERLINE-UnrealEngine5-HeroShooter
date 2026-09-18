#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ALTypes.h"
#include "ALGameMode.generated.h"
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
	UPROPERTY(EditAnywhere) float MatchTimeSeconds = 360.f;
	UPROPERTY(EditAnywhere) int32 ScoreToWin = 100;
	// Total bots in skirmish playlists (override at runtime with `al.Bots`). AllyBotFill of them join the player's team.
	UPROPERTY(EditAnywhere) int32 BotFill = 7;
	UPROPERTY(EditAnywhere) int32 AllyBotFill = 2;
	UPROPERTY(EditAnywhere) int32 BRPlayers = 21;
	UPROPERTY(EditAnywhere) int32 BRSquadSize = 3;
	// Hostiles spawn on a ring around the player near the yard edge; allies spawn close to the player.
	FVector FindSpawnLocation(EALTeam Team) const;
protected:
	void SpawnArenaIfMissing();
};
