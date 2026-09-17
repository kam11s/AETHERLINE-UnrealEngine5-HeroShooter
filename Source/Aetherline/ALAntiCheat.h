#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ALAntiCheat.generated.h"
UCLASS(ClassGroup=(Aetherline))
class AETHERLINE_API UALAntiCheat : public UActorComponent
{
	GENERATED_BODY()
public:
	UALAntiCheat() { PrimaryComponentTick.bCanEverTick = false; }
};
