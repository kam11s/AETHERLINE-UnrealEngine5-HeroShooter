#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ALArenaBuilder.generated.h"
class UPointLightComponent;
class UMaterialInstanceDynamic;
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
	void Box(const FVector& Loc, const FVector& Ext, const FRotator& Rot, const FLinearColor& Color);

	// Yard (dense industrial cover). Everything here is Engine BasicShapes cubes.
	void Yard();
	void GatherKeepClear();
	bool Fits(const FVector& Center, float FootprintRadius) const;
	void Container(const FVector& Base, float Yaw, const FLinearColor& Color, int32 Tiers);
	void ContainerRing();
	void CrateStack(const FVector& Base, float Yaw, int32 Count);
	void Barrier(const FVector& Base, float Yaw);
	void PipeRack(const FVector& Center, float Yaw);
	void Catwalk(const FVector& Center, float Yaw, float RampDir);
	void Ramp(const FVector& Bottom, const FVector& Top, float HalfWidth);
	void Pillar(const FVector& Base);
	TArray<FVector> KeepClear;
	TArray<float> KeepClearRadius;
	TMap<uint32, UMaterialInstanceDynamic*> MaterialCache;

	void Lights();
	void NightSky();
	void Practicals();
	UPointLightComponent* Lamp(const FVector& Loc, const FLinearColor& Col, float Candelas, float Radius, float Scatter);
	void LampPost(const FVector& Base, const FLinearColor& HeadColor, const FLinearColor& LightColor, float Candelas, float Radius);
	void SpotMast(const FVector& Base, const FVector& Target, const FLinearColor& Col, float Candelas, float Radius);
};
