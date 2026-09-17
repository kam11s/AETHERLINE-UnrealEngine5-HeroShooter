#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "ALHeroAIController.generated.h"
UCLASS()
class AETHERLINE_API AALHeroAIController : public AAIController
{
	GENERATED_BODY()
public:
	AALHeroAIController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
protected:
	float Think = 0.f;
	void ThinkNow();
};
