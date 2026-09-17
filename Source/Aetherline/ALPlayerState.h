#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ALTypes.h"
#include "ALPlayerState.generated.h"

class UALAttributeSet;

UCLASS()
class AETHERLINE_API AALPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	AALPlayerState();
	UALAttributeSet* GetSet() const { return Set; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated, BlueprintReadOnly) EALHero SelectedHero = EALHero::Wraith;
	UPROPERTY(Replicated, BlueprintReadOnly) EALTeam TeamId = EALTeam::Ally;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Elims = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Deaths = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 Assists = 0;
protected:
	UPROPERTY() TObjectPtr<UALAttributeSet> Set;
};
