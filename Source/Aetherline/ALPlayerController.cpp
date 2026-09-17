#include "ALPlayerController.h"
#include "ALHeroCharacter.h"
#include "ALPlayerState.h"
#include "ALLoadGate.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
AALPlayerController::AALPlayerController() { bShowMouseCursor = false; }
void AALPlayerController::BeginPlay()
{
	Super::BeginPlay();
	OnLoadFinished();
}
void AALPlayerController::OnLoadFinished()
{
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	FInputModeGameOnly Mode; SetInputMode(Mode);
}
void AALPlayerController::SelectHero(EALHero Hero) { ServerSelectHero(Hero); }
void AALPlayerController::ServerSelectHero_Implementation(EALHero Hero)
{
	if (AALPlayerState* PS = GetPlayerState<AALPlayerState>()) PS->SelectedHero = Hero;
	if (AALHeroCharacter* HeroPawn = Cast<AALHeroCharacter>(GetPawn())) HeroPawn->ApplyHero(Hero);
}
