#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ALSocialSubsystem.generated.h"
UCLASS()
class AETHERLINE_API UALSocialSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly) FString DisplayName;
	UFUNCTION(BlueprintCallable) FString ShareLink() const { return TEXT("https://github.com/kam11s/AETHERLINE-UnrealEngine5-HeroShooter"); }
};
