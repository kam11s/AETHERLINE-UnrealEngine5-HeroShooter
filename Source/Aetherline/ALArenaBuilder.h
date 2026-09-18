#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALArenaBuilder.generated.h"
class UPointLightComponent;
UCLASS()
class AETHERLINE_API AALArenaBuilder : public AActor
{
	GENERATED_BODY()
public:
	AALArenaBuilder();
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere) float Size = 4200.f;
	UPROPERTY(EditAnywhere) float WallH = 280.f;
private:
	void Box(const FVector& Loc, const FVector& Ext, const FLinearColor& Color);
	void Lights();
	void NightSky();
	void Practicals();
	UPointLightComponent* Lamp(const FVector& Loc, const FLinearColor& Col, float Candelas, float Radius, float Scatter);
	void LampPost(const FVector& Base, const FLinearColor& HeadColor, const FLinearColor& LightColor, float Candelas, float Radius);
	void SpotMast(const FVector& Base, const FVector& Target, const FLinearColor& Col, float Candelas, float Radius);
};
