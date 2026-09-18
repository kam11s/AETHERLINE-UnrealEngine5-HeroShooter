#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALArenaBuilder.generated.h"
UCLASS()
class AETHERLINE_API AALArenaBuilder : public AActor
{
	GENERATED_BODY()
public:
	AALArenaBuilder();
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere) float Size = 4200.f;
	UPROPERTY(EditAnywhere) float WallH = 280.f;
	// World-space centres of the yard's cover blocks and pylons; filled at BeginPlay, read by ALHeroAIController.
	TArray<FVector> CoverPoints;
private:
	void Box(const FVector& Loc, const FVector& Ext, const FLinearColor& Color);
	void Lights();
};
