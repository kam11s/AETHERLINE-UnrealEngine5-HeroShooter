#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ALSettingsSave.generated.h"
UCLASS()
class AETHERLINE_API UALSettingsSave : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() FString DisplayName = TEXT("Operator");
};
