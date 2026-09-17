#include "ALPlayerState.h"
#include "ALAttributeSet.h"
#include "Net/UnrealNetwork.h"

AALPlayerState::AALPlayerState()
{
	bReplicates = true;
	Set = CreateDefaultSubobject<UALAttributeSet>(TEXT("Set"));
}

void AALPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AALPlayerState, SelectedHero);
	DOREPLIFETIME(AALPlayerState, TeamId);
	DOREPLIFETIME(AALPlayerState, Elims);
	DOREPLIFETIME(AALPlayerState, Deaths);
	DOREPLIFETIME(AALPlayerState, Assists);
}
