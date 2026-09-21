#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ALTypes.h"
#include "ALGameState.generated.h"
UCLASS()
class AETHERLINE_API AALGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	UPROPERTY(Replicated, BlueprintReadOnly) int32 AllyScore = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 EnemyScore = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) float TimeLeft = 360.f;
	UPROPERTY(Replicated, BlueprintReadOnly) EALPlaylist Playlist = EALPlaylist::Training;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 AlivePlayers = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 SquadSize = 3;
	UPROPERTY(Replicated, BlueprintReadOnly) float CircleRadius = 12000.f;
	UPROPERTY(Replicated, BlueprintReadOnly) EALDropPhase DropPhase = EALDropPhase::Lobby;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bMatchOver = false;
	// 0 = none, 1 = allies, 2 = hostiles
	UPROPERTY(Replicated, BlueprintReadOnly) int32 WinnerSide = 0;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintPure) bool IsBattleRoyale() const;
};
