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
	UPROPERTY(EditAnywhere) int32 BotFill = 7;
	UPROPERTY(EditAnywhere) int32 BRPlayers = 21;
	UPROPERTY(EditAnywhere) int32 BRSquadSize = 3;
	UPROPERTY(EditAnywhere) float RematchDelaySeconds = 6.f;
protected:
	void SpawnArenaIfMissing();
	void CheckMatchEnd();
	void RestartSkirmish();
	float RematchTimer = 0.f;
};
