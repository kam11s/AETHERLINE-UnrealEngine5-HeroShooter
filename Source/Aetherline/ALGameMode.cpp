#include "ALGameMode.h"
#include "ALGameState.h"
#include "ALHeroCharacter.h"
#include "ALPlayerController.h"
#include "ALPlayerState.h"
#include "ALHeroAIController.h"
#include "ALArenaBuilder.h"
#include "ALHUD.h"
#include "ALDropship.h"
#include "ALGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AALGameMode::AALGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AALHeroCharacter::StaticClass();
	PlayerControllerClass = AALPlayerController::StaticClass();
	PlayerStateClass = AALPlayerState::StaticClass();
	GameStateClass = AALGameState::StaticClass();
	HUDClass = AALHUD::StaticClass();
}
void AALGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpawnArenaIfMissing();
	EALPlaylist Start = EALPlaylist::BotSkirmish;
	if (const UALGameInstance* GI = GetGameInstance<UALGameInstance>()) Start = GI->SelectedPlaylist;
	StartPlaylist(Start);
}
void AALGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (AALGameState* GS = GetGameState<AALGameState>())
	{
		if (!GS->bMatchOver && GS->Playlist != EALPlaylist::Training)
		{
			GS->TimeLeft = FMath::Max(0.f, GS->TimeLeft - DeltaSeconds);
		}
		if (GS->IsBattleRoyale())
		{
			GS->CircleRadius = FMath::Max(1800.f, GS->CircleRadius - DeltaSeconds * 18.f);
			int32 Alive = 0;
			for (TActorIterator<AALHeroCharacter> It(GetWorld()); It; ++It) if (It->IsAlive()) ++Alive;
			GS->AlivePlayers = Alive;
		}
	}
	CheckMatchEnd();
}
void AALGameMode::StartPlaylist(EALPlaylist InPlaylist)
{
	if (AALGameState* GS = GetGameState<AALGameState>())
	{
		GS->Playlist = InPlaylist;
		GS->TimeLeft = (InPlaylist == EALPlaylist::Training) ? 0.f : MatchTimeSeconds;
		GS->DropPhase = EALDropPhase::Lobby;
		GS->CircleRadius = (InPlaylist == EALPlaylist::BattleRoyale) ? 12000.f : 4200.f;
		GS->SquadSize = BRSquadSize;
		GS->bMatchOver = false;
		GS->WinnerSide = 0;
		GS->AllyScore = 0;
		GS->EnemyScore = 0;
	}
	if (InPlaylist == EALPlaylist::BattleRoyale) { StartBattleRoyale(); return; }
	if (InPlaylist != EALPlaylist::Training) SpawnBots();
}
void AALGameMode::SpawnArenaIfMissing()
{
	for (TActorIterator<AALArenaBuilder> It(GetWorld()); It; ++It) return;
	GetWorld()->SpawnActor<AALArenaBuilder>(AALArenaBuilder::StaticClass(), FTransform::Identity);
}
void AALGameMode::StartBattleRoyale()
{
	UWorld* W = GetWorld(); if (!W) return;
	if (AALGameState* GS = GetGameState<AALGameState>()) { GS->Playlist = EALPlaylist::BattleRoyale; GS->DropPhase = EALDropPhase::OnDropship; GS->AlivePlayers = BRPlayers; }
	AALDropship* Ship = W->SpawnActor<AALDropship>(AALDropship::StaticClass(), FVector(-14000.f,0.f,2800.f), FRotator::ZeroRotator);
	if (AALHeroCharacter* Player = Cast<AALHeroCharacter>(UGameplayStatics::GetPlayerPawn(W, 0))) { Player->TeamId = EALTeam::Ally; if (Ship) Player->AttachToDropship(Ship); }
	for (int32 i = 0; i < FMath::Max(0, BRPlayers-1); ++i)
	{
		AALHeroCharacter* Bot = W->SpawnActor<AALHeroCharacter>(AALHeroCharacter::StaticClass());
		if (!Bot) continue;
		Bot->SquadId = (i+1)/BRSquadSize;
		Bot->TeamId = (Bot->SquadId==0)? EALTeam::Ally : EALTeam::Enemy;
		Bot->ApplyHero(static_cast<EALHero>(i%6));
		if (Ship) Bot->AttachToDropship(Ship);
		if (AALHeroAIController* AIC = W->SpawnActor<AALHeroAIController>()) AIC->Possess(Bot);
	}
}
void AALGameMode::SpawnBots()
{
	UWorld* W = GetWorld(); if (!W) return;
	for (int32 i = 0; i < BotFill; ++i)
	{
		const FVector Loc(FMath::FRandRange(-1800.f,1800.f), FMath::FRandRange(-1800.f,1800.f), 120.f);
		AALHeroCharacter* Bot = W->SpawnActor<AALHeroCharacter>(AALHeroCharacter::StaticClass(), Loc, FRotator::ZeroRotator);
		if (!Bot) continue;
		Bot->TeamId = (i%2==0)? EALTeam::Enemy : EALTeam::Ally;
		Bot->ApplyHero(static_cast<EALHero>(i%6));
		if (AALHeroAIController* AIC = W->SpawnActor<AALHeroAIController>(AALHeroAIController::StaticClass(), Loc, FRotator::ZeroRotator)) AIC->Possess(Bot);
	}
}
bool AALGameMode::IsScoredPlaylist(EALPlaylist InPlaylist) const
{
	return InPlaylist == EALPlaylist::BotSkirmish || InPlaylist == EALPlaylist::QuickPlay || InPlaylist == EALPlaylist::Ranked;
}
FVector AALGameMode::FindSpawnLocation(EALTeam Team) const
{
	// Allies spawn on the -X half of the yard, hostiles on +X, so respawns don't land in the enemy's lap.
	const float XMin = (Team == EALTeam::Enemy) ? 600.f : -1800.f;
	const float XMax = (Team == EALTeam::Enemy) ? 1800.f : -600.f;
	return FVector(FMath::FRandRange(XMin, XMax), FMath::FRandRange(-1800.f, 1800.f), 120.f);
}
void AALGameMode::OnHeroKilled(AALHeroCharacter* Killer, AALHeroCharacter* Victim)
{
	AALGameState* GS = GetGameState<AALGameState>();
	if (!GS || !Killer || !Victim || GS->bMatchOver || !IsScoredPlaylist(GS->Playlist)) return;
	if (Killer->TeamId == Victim->TeamId) return;
	if (Killer->TeamId == EALTeam::Ally) ++GS->AllyScore;
	else if (Killer->TeamId == EALTeam::Enemy) ++GS->EnemyScore;
	CheckMatchEnd();
}
void AALGameMode::CheckMatchEnd()
{
	AALGameState* GS = GetGameState<AALGameState>();
	if (!GS || !IsScoredPlaylist(GS->Playlist)) return;
	if (GS->bMatchOver)
	{
		if (RematchDelaySeconds > 0.f)
		{
			RematchTimer -= GetWorld()->GetDeltaSeconds();
			if (RematchTimer <= 0.f) RestartSkirmish();
		}
		return;
	}
	int32 Winner = 0;
	if (GS->AllyScore >= ScoreToWin) Winner = 1;
	else if (GS->EnemyScore >= ScoreToWin) Winner = 2;
	else if (GS->TimeLeft <= 0.f)
	{
		if (GS->AllyScore > GS->EnemyScore) Winner = 1;
		else if (GS->EnemyScore > GS->AllyScore) Winner = 2;
		else Winner = 3;
	}
	if (Winner != 0)
	{
		GS->bMatchOver = true;
		GS->WinnerSide = Winner;
		GS->TimeLeft = 0.f;
		RematchTimer = RematchDelaySeconds;
	}
}
void AALGameMode::RequestPlayAgain()
{
	const AALGameState* GS = GetGameState<AALGameState>();
	if (!GS || !GS->bMatchOver || !IsScoredPlaylist(GS->Playlist)) return;
	RestartSkirmish();
}
void AALGameMode::RestartSkirmish()
{
	UWorld* W = GetWorld();
	if (!W) return;
	EALPlaylist Playlist = EALPlaylist::BotSkirmish;
	if (const AALGameState* GS = GetGameState<AALGameState>())
	{
		if (IsScoredPlaylist(GS->Playlist)) Playlist = GS->Playlist;
	}
	TArray<AALHeroCharacter*> ToRemove;
	TArray<AALHeroCharacter*> Players;
	for (TActorIterator<AALHeroCharacter> It(W); It; ++It)
	{
		if (It->IsPlayerControlled()) Players.Add(*It);
		else ToRemove.Add(*It);
	}
	for (AALHeroCharacter* Bot : ToRemove)
	{
		if (AController* C = Bot->GetController())
		{
			C->UnPossess();
			C->Destroy();
		}
		Bot->Destroy();
	}
	StartPlaylist(Playlist);
	for (AALHeroCharacter* Player : Players) Player->Respawn();
}
AActor* AALGameMode::ChoosePlayerStart_Implementation(AController* Player) { return Super::ChoosePlayerStart_Implementation(Player); }
