#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ALTypes.h"
#include "ALPlayerController.generated.h"
UCLASS()
class AETHERLINE_API AALPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AALPlayerController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	UFUNCTION() void OnLoadFinished();
	UFUNCTION(BlueprintCallable) void SelectHero(EALHero Hero);
	UFUNCTION(Server, Reliable) void ServerSelectHero(EALHero Hero);
	UFUNCTION(Server, Reliable) void ServerRequestPlayAgain();
protected:
	void OnPlayAgain();
};
