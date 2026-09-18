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
		if (GS->Playlist != EALPlaylist::Training) GS->TimeLeft = FMath::Max(0.f, GS->TimeLeft - DeltaSeconds);
		if (GS->IsBattleRoyale())
		{
			GS->CircleRadius = FMath::Max(1800.f, GS->CircleRadius - DeltaSeconds * 18.f);
			int32 Alive = 0;
			for (TActorIterator<AALHeroCharacter> It(GetWorld()); It; ++It) if (It->IsAlive()) ++Alive;
			GS->AlivePlayers = Alive;
		}
	}
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
	// The yard is dense now; nudge bots out of any container/crate they roll into.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (int32 i = 0; i < BotFill; ++i)
	{
		const FVector Loc(FMath::FRandRange(-1800.f,1800.f), FMath::FRandRange(-1800.f,1800.f), 120.f);
		AALHeroCharacter* Bot = W->SpawnActor<AALHeroCharacter>(AALHeroCharacter::StaticClass(), Loc, FRotator::ZeroRotator, Params);
		if (!Bot) continue;
		Bot->TeamId = (i%2==0)? EALTeam::Enemy : EALTeam::Ally;
		Bot->ApplyHero(static_cast<EALHero>(i%6));
		if (AALHeroAIController* AIC = W->SpawnActor<AALHeroAIController>(AALHeroAIController::StaticClass(), Loc, FRotator::ZeroRotator)) AIC->Possess(Bot);
	}
}
AActor* AALGameMode::ChoosePlayerStart_Implementation(AController* Player) { return Super::ChoosePlayerStart_Implementation(Player); }
