#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ALTypes.h"
#include "ALGameInstance.generated.h"
UCLASS()
class AETHERLINE_API UALGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite) EALHero SelectedHero = EALHero::Wraith;
	UPROPERTY(BlueprintReadWrite) EALPlaylist SelectedPlaylist = EALPlaylist::BotSkirmish;
	UPROPERTY(BlueprintReadWrite) int32 Credits = 0;
	UPROPERTY(BlueprintReadWrite) int32 BattlePassXP = 0;
	UPROPERTY(BlueprintReadWrite) int32 BattlePassTier = 1;
	UFUNCTION(BlueprintCallable) void QueuePlaylist(EALPlaylist Playlist);
	UFUNCTION(BlueprintCallable) void GrantMatchRewards(bool bWin, int32 Elims);
	UFUNCTION(BlueprintPure) bool OwnsPremiumBattlePass() const;
};
