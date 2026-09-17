#include "ALGameState.h"
#include "Net/UnrealNetwork.h"
void AALGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AALGameState, AllyScore);
	DOREPLIFETIME(AALGameState, EnemyScore);
	DOREPLIFETIME(AALGameState, TimeLeft);
	DOREPLIFETIME(AALGameState, Playlist);
	DOREPLIFETIME(AALGameState, AlivePlayers);
	DOREPLIFETIME(AALGameState, SquadSize);
	DOREPLIFETIME(AALGameState, CircleRadius);
	DOREPLIFETIME(AALGameState, DropPhase);
}
bool AALGameState::IsBattleRoyale() const { return Playlist == EALPlaylist::BattleRoyale; }
