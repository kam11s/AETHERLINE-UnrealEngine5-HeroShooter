#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ALAttributeSet.generated.h"

UCLASS()
class AETHERLINE_API UALAttributeSet : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() float Health = 200.f;
	UPROPERTY() float MaxHealth = 200.f;
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	void SetHealth(float V) { Health = V; }
	void SetMaxHealth(float V) { MaxHealth = V; }
};
