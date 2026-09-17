#pragma once
#include "CoreMinimal.h"
#include "ALTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ALHeroCatalog.generated.h"
UCLASS()
class AETHERLINE_API UALHeroCatalog : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="Aetherline") static FALHeroDef Get(EALHero Hero);
};
