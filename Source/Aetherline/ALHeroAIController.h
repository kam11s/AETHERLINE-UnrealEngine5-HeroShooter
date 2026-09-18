#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "ALHeroAIController.generated.h"
class AALHeroCharacter;
class AALArenaBuilder;
UCLASS()
class AETHERLINE_API AALHeroAIController : public AAIController
{
	GENERATED_BODY()
public:
	AALHeroAIController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
protected:
	void ThinkNow();
	AALHeroCharacter* PickTarget(AALHeroCharacter* Self) const;
	FVector PickCoverGoal(const FVector& Loc, const FVector& TargetLoc, bool bRetreat) const;
	FVector PickWanderGoal(const FVector& Loc) const;
	FVector ClampToArena(FVector P) const;
	void SetGoal(const FVector& NewGoal);
	void UpdateMovement(AALHeroCharacter* Self, float DeltaSeconds);
	void SteerTowardGoal(AALHeroCharacter* Self);
	void UpdateCombat(AALHeroCharacter* Self, float DeltaSeconds);

	TWeakObjectPtr<AALHeroCharacter> Target;
	TWeakObjectPtr<AALArenaBuilder> Arena;
	FVector Goal = FVector::ZeroVector;
	bool bHasGoal = false;
	bool bGoalReached = false;
	// True when no NavMesh path is available and the bot steers directly with obstacle probes.
	bool bSteerManually = true;
	// 0 = untested, 1 = NavMesh pathing works in this level, -1 = no NavMesh (always steer directly).
	int8 NavMode = 0;
	float AvoidSide = 1.f;
	float StuckTime = 0.f;
	float Think = 0.f;
	float PeekTimer = 0.f;
	float StrafeSign = 0.f;
	float BurstClock = 0.f;
	float AcquireTime = 0.f;
	float LastSeenTime = -100.f;
	bool bWasVisible = false;
};
