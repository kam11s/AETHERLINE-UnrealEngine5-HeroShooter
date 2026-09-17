#include "ALHeroAIController.h"
#include "ALHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
AALHeroAIController::AALHeroAIController() { PrimaryActorTick.bCanEverTick = true; bWantsPlayerState = true; }
void AALHeroAIController::OnPossess(APawn* InPawn) { Super::OnPossess(InPawn); ThinkNow(); }
void AALHeroAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Think -= DeltaSeconds; if (Think <= 0.f) ThinkNow();
	if (AALHeroCharacter* Self = Cast<AALHeroCharacter>(GetPawn()))
	{
		if (AALHeroCharacter* Target = Cast<AALHeroCharacter>(GetFocusActor()))
		{
			const FVector To = Target->GetActorLocation() - Self->GetActorLocation();
			if (!To.IsNearlyZero()) { Self->SetActorRotation(To.Rotation()); if (To.Size() < 2500.f) Self->FireOnce(); }
		}
	}
}
void AALHeroAIController::ThinkNow()
{
	Think = 0.35f;
	AALHeroCharacter* Self = Cast<AALHeroCharacter>(GetPawn());
	if (!Self || !Self->IsAlive()) return;
	if (Self->bOnDropship) { if (FMath::FRand() < 0.12f) Self->DeployFromDropship(); return; }
	TArray<AActor*> Heroes; UGameplayStatics::GetAllActorsOfClass(this, AALHeroCharacter::StaticClass(), Heroes);
	AALHeroCharacter* Best = nullptr; float BestD = 1e9f;
	for (AActor* A : Heroes)
	{
		AALHeroCharacter* H = Cast<AALHeroCharacter>(A);
		if (!H || H == Self || !H->IsAlive() || H->TeamId == Self->TeamId) continue;
		const float D = FVector::Dist(H->GetActorLocation(), Self->GetActorLocation());
		if (D < BestD) { BestD = D; Best = H; }
	}
	if (Best) { SetFocus(Best); MoveToActor(Best, 700.f); }
	else MoveToLocation(FVector(0.f,0.f,100.f), 200.f);
}
