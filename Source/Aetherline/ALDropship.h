#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALDropship.generated.h"
UCLASS()
class AETHERLINE_API AALDropship : public AActor
{
	GENERATED_BODY()
public:
	AALDropship();
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(EditAnywhere) float Altitude = 2800.f;
	UPROPERTY(EditAnywhere) float Speed = 1800.f;
	UPROPERTY(EditAnywhere) float PathHalf = 14000.f;
	float Travel = 0.f;
};
