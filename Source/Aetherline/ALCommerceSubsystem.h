#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ALCommerceSubsystem.generated.h"

UCLASS()
class AETHERLINE_API UALCommerceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly) int32 Credits = 0;
	UPROPERTY(BlueprintReadOnly) bool bOwnsBattlePass = false;
	UFUNCTION(BlueprintCallable) void RefreshEntitlements() {}
	UFUNCTION(BlueprintCallable) void RequestBattlePassCheckout() {}
	UFUNCTION(BlueprintCallable) void ReportMatchRewards(bool bWin, int32 Elims) { Credits += (bWin ? 120 : 40) + Elims * 10; }
	UFUNCTION(BlueprintPure) bool CanUsePremiumTrack() const { return bOwnsBattlePass; }
};
