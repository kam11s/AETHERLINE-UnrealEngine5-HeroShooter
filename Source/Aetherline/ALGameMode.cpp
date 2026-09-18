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
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarALBots(
	TEXT("al.Bots"), -1,
	TEXT("Total bot count for skirmish playlists (-1 = GameMode BotFill). Read when the match starts."));

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
	const int32 Override = CVarALBots.GetValueOnGameThread();
	const int32 Total = FMath::Clamp(Override >= 0 ? Override : BotFill, 0, 64);
	const int32 Allies = FMath::Clamp(AllyBotFill, 0, Total);
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (int32 i = 0; i < Total; ++i)
	{
		const EALTeam Team = (i < Total - Allies) ? EALTeam::Enemy : EALTeam::Ally;
		const FVector Loc = FindSpawnLocation(Team);
		const FRotator Rot(0.f, FMath::FRandRange(-180.f, 180.f), 0.f);
		AALHeroCharacter* Bot = W->SpawnActor<AALHeroCharacter>(AALHeroCharacter::StaticClass(), Loc, Rot, SP);
		if (!Bot) continue;
		Bot->TeamId = Team;
		Bot->ApplyHero(static_cast<EALHero>(i%6));
		if (AALHeroAIController* AIC = W->SpawnActor<AALHeroAIController>(AALHeroAIController::StaticClass(), Loc, Rot, SP)) AIC->Possess(Bot);
	}
}
FVector AALGameMode::FindSpawnLocation(EALTeam Team) const
{
	FVector Center = FVector::ZeroVector;
	if (const APawn* P = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) Center = FVector(P->GetActorLocation().X, P->GetActorLocation().Y, 0.f);
	float Bound = 3800.f;
	for (TActorIterator<AALArenaBuilder> It(GetWorld()); It; ++It) { Bound = It->Size - 400.f; break; }
	const bool bHostile = Team == EALTeam::Enemy;
	const float R = bHostile ? FMath::FRandRange(2000.f, 2800.f) : FMath::FRandRange(500.f, 900.f);
	const float A = FMath::FRandRange(0.f, 2.f * PI);
	FVector Loc = Center + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f);
	Loc.X = FMath::Clamp(Loc.X, -Bound, Bound);
	Loc.Y = FMath::Clamp(Loc.Y, -Bound, Bound);
	Loc.Z = 120.f;
	return Loc;
}
AActor* AALGameMode::ChoosePlayerStart_Implementation(AController* Player) { return Super::ChoosePlayerStart_Implementation(Player); }
