#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ALLoadGate.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FALLoadFinished);

UCLASS()
class AETHERLINE_API UALLoadGate : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void BeginLoad(const FString& Reason);

	UFUNCTION(BlueprintCallable)
	void MarkWorldReady();

	UPROPERTY(BlueprintAssignable)
	FALLoadFinished OnFinished;
};
