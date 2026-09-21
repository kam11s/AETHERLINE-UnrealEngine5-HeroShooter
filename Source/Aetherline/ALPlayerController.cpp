#include "ALPlayerController.h"
#include "ALHeroCharacter.h"
#include "ALPlayerState.h"
#include "ALLoadGate.h"
#include "ALGameMode.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/InputComponent.h"
AALPlayerController::AALPlayerController() { bShowMouseCursor = false; }
void AALPlayerController::BeginPlay()
{
	Super::BeginPlay();
	OnLoadFinished();
}
void AALPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// Bound on the controller (not the pawn) so it still works while the player is downed.
	if (InputComponent) InputComponent->BindAction(TEXT("PlayAgain"), IE_Pressed, this, &AALPlayerController::OnPlayAgain);
}
void AALPlayerController::OnPlayAgain() { ServerRequestPlayAgain(); }
void AALPlayerController::ServerRequestPlayAgain_Implementation()
{
	if (AALGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AALGameMode>() : nullptr) GM->RequestPlayAgain();
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
