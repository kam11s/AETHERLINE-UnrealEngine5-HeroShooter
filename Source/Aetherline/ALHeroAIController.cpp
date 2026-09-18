#include "ALHeroAIController.h"
#include "ALHeroCharacter.h"
#include "ALArenaBuilder.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<float> CVarALBotSpread(
	TEXT("al.BotSpread"), 5.0f,
	TEXT("Bot aim cone half-angle in degrees (0 = laser accurate)."));
static TAutoConsoleVariable<float> CVarALBotDamageScale(
	TEXT("al.BotDamageScale"), 0.5f,
	TEXT("Multiplier applied to bot weapon damage."));
static TAutoConsoleVariable<int32> CVarALBotDebug(
	TEXT("al.BotDebug"), 0,
	TEXT("1 = draw bot goal / line-of-sight debug lines."));

namespace
{
	constexpr float SightRange = 6000.f;
	constexpr float FireRange = 3200.f;
	constexpr float EngageRange = 1500.f;
	constexpr float ReactionTime = 0.45f;
	constexpr float GoalAccept = 90.f;
	constexpr float ThinkInterval = 0.3f;
	constexpr float BurstPeriod = 1.3f;
	constexpr float BurstOn = 0.75f;
	constexpr float ProbeDist = 240.f;
}

AALHeroAIController::AALHeroAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = true;
	BurstClock = FMath::FRandRange(0.f, BurstPeriod);
	AvoidSide = (FMath::FRand() < 0.5f) ? 1.f : -1.f;
}
void AALHeroAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// Stagger the first think so a squad does not re-plan on the same frame.
	Think = FMath::FRandRange(0.05f, ThinkInterval);
	bHasGoal = false;
	Target.Reset();
}
void AALHeroAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AALHeroCharacter* Self = Cast<AALHeroCharacter>(GetPawn());
	if (!Self) return;
	Think -= DeltaSeconds;
	if (Think <= 0.f) ThinkNow();
	if (!Self->IsAlive() || Self->bOnDropship || Self->bSkydiving)
	{
		if (bHasGoal) { StopMovement(); bHasGoal = false; }
		if (!Self->IsAlive()) { ClearFocus(EAIFocusPriority::Gameplay); Target.Reset(); }
		return;
	}
	UpdateMovement(Self, DeltaSeconds);
	UpdateCombat(Self, DeltaSeconds);
	if (CVarALBotDebug.GetValueOnGameThread() != 0 && bHasGoal)
	{
		DrawDebugLine(GetWorld(), Self->GetActorLocation(), Goal, FColor(40, 220, 255), false, -1.f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), Goal, 30.f, 8, bSteerManually ? FColor(255, 150, 40) : FColor(40, 220, 255), false, -1.f, 0, 1.f);
	}
}
void AALHeroAIController::ThinkNow()
{
	Think = ThinkInterval;
	AALHeroCharacter* Self = Cast<AALHeroCharacter>(GetPawn());
	if (!Self || !Self->IsAlive()) return;
	if (Self->bOnDropship) { if (FMath::FRand() < 0.12f) Self->DeployFromDropship(); return; }
	if (Self->bSkydiving) return;
	if (!Arena.IsValid())
	{
		for (TActorIterator<AALArenaBuilder> It(GetWorld()); It; ++It) { Arena = *It; break; }
	}

	AALHeroCharacter* NewTarget = PickTarget(Self);
	if (NewTarget != Target.Get()) { Target = NewTarget; bWasVisible = false; }

	const FVector Loc = Self->GetActorLocation();
	const float Now = GetWorld()->GetTimeSeconds();
	if (AALHeroCharacter* T = Target.Get())
	{
		SetFocus(T, EAIFocusPriority::Gameplay);
		const FVector TLoc = T->GetActorLocation();
		const float D = FVector::Dist2D(Loc, TLoc);
		const bool bHurt = Self->GetHealth() < Self->GetMaxHealth() * 0.3f;
		const bool bLostSight = (Now - LastSeenTime) > 3.f;
		if (bHurt)
		{
			SetGoal(PickCoverGoal(Loc, TLoc, true));
		}
		else if (D > EngageRange || bLostSight)
		{
			SetGoal(PickCoverGoal(Loc, TLoc, false));
		}
		else
		{
			// In range with line of sight: hold, then occasionally strafe so the exchange reads as a firefight.
			PeekTimer -= ThinkInterval;
			if (PeekTimer <= 0.f)
			{
				PeekTimer = FMath::FRandRange(1.2f, 2.2f);
				StrafeSign = (FMath::FRand() < 0.35f) ? 0.f : ((FMath::FRand() < 0.5f) ? 1.f : -1.f);
				if (StrafeSign != 0.f)
				{
					const FVector Right = FVector::CrossProduct(FVector::UpVector, (TLoc - Loc).GetSafeNormal2D());
					SetGoal(Loc + Right * StrafeSign * 260.f);
				}
			}
		}
	}
	else
	{
		if (!bHasGoal || bGoalReached) SetGoal(PickWanderGoal(Loc));
		SetFocalPoint(Goal + FVector(0.f, 0.f, 64.f), EAIFocusPriority::Gameplay);
	}
}
AALHeroCharacter* AALHeroAIController::PickTarget(AALHeroCharacter* Self) const
{
	AALHeroCharacter* Best = nullptr;
	float BestScore = SightRange;
	for (TActorIterator<AALHeroCharacter> It(GetWorld()); It; ++It)
	{
		AALHeroCharacter* H = *It;
		if (H == Self || !H->IsAlive() || H->TeamId == Self->TeamId || H->bOnDropship) continue;
		float Score = FVector::Dist(H->GetActorLocation(), Self->GetActorLocation());
		// Bias toward the human so the player always has pressure; small hysteresis keeps bots from flip-flopping.
		if (H->IsPlayerControlled()) Score -= 800.f;
		if (H == Target.Get()) Score -= 300.f;
		if (Score < BestScore) { BestScore = Score; Best = H; }
	}
	return Best;
}
FVector AALHeroAIController::PickCoverGoal(const FVector& Loc, const FVector& TargetLoc, bool bRetreat) const
{
	const float DSelf = FVector::Dist2D(Loc, TargetLoc);
	const FVector* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	if (const AALArenaBuilder* A = Arena.Get())
	{
		for (const FVector& C : A->CoverPoints)
		{
			const float DCT = FVector::Dist2D(C, TargetLoc);
			const float DBC = FVector::Dist2D(Loc, C);
			if (DCT < 450.f) continue;
			float Score;
			if (bRetreat)
			{
				if (DBC > 1600.f) continue;
				Score = DBC - DCT;
			}
			else
			{
				// Only leapfrog to cover that brings us closer; penalise the block we are already hugging.
				if (DCT > DSelf + 150.f) continue;
				Score = DBC + 0.6f * DCT + (DBC < 220.f ? 500.f : 0.f);
			}
			if (Score < BestScore) { BestScore = Score; Best = &C; }
		}
	}
	FVector G;
	if (Best) G = *Best + (*Best - TargetLoc).GetSafeNormal2D() * 170.f;
	else G = TargetLoc + (Loc - TargetLoc).GetSafeNormal2D() * 700.f;
	G.Z = Loc.Z;
	return G;
}
FVector AALHeroAIController::PickWanderGoal(const FVector& Loc) const
{
	FVector G(FMath::FRandRange(-1500.f, 1500.f), FMath::FRandRange(-1500.f, 1500.f), Loc.Z);
	if (const AALArenaBuilder* A = Arena.Get())
	{
		if (A->CoverPoints.Num() > 0)
		{
			const FVector C = A->CoverPoints[FMath::RandRange(0, A->CoverPoints.Num() - 1)];
			const float Ang = FMath::FRandRange(0.f, 2.f * PI);
			G = FVector(C.X + FMath::Cos(Ang) * 220.f, C.Y + FMath::Sin(Ang) * 220.f, Loc.Z);
		}
	}
	return G;
}
FVector AALHeroAIController::ClampToArena(FVector P) const
{
	float Bound = 3900.f;
	if (const AALArenaBuilder* A = Arena.Get()) Bound = A->Size - 250.f;
	P.X = FMath::Clamp(P.X, -Bound, Bound);
	P.Y = FMath::Clamp(P.Y, -Bound, Bound);
	return P;
}
void AALHeroAIController::SetGoal(const FVector& NewGoal)
{
	const bool bWasNavMove = bHasGoal && !bSteerManually;
	Goal = ClampToArena(NewGoal);
	bHasGoal = true;
	bGoalReached = false;
	StuckTime = 0.f;
	bSteerManually = true;
	if (NavMode >= 0)
	{
		// Use the level's NavMesh when the owner's map has one; the template yard has none, so fall through to steering.
		const EPathFollowingRequestResult::Type R = MoveToLocation(Goal, GoalAccept, true, true, true, true);
		if (R == EPathFollowingRequestResult::Failed)
		{
			if (NavMode == 0) NavMode = -1;
		}
		else
		{
			NavMode = 1;
			bSteerManually = false;
			if (R == EPathFollowingRequestResult::AlreadyAtGoal) bGoalReached = true;
		}
	}
	if (bSteerManually && bWasNavMove) StopMovement();
}
void AALHeroAIController::UpdateMovement(AALHeroCharacter* Self, float DeltaSeconds)
{
	if (!bHasGoal || bGoalReached) return;
	if (!bSteerManually && GetMoveStatus() == EPathFollowingStatus::Idle) { bGoalReached = true; return; }
	const FVector Loc = Self->GetActorLocation();
	const float Speed = Self->GetVelocity().Size2D();
	StuckTime = (Speed < 40.f) ? StuckTime + DeltaSeconds : 0.f;
	if (StuckTime > 1.0f)
	{
		StuckTime = 0.f;
		AvoidSide = -AvoidSide;
		if (!bSteerManually)
		{
			// A static NavMesh does not know about the runtime yard blocks; steer directly instead.
			StopMovement();
			bSteerManually = true;
		}
		else
		{
			const FVector Right = FVector::CrossProduct(FVector::UpVector, (Goal - Loc).GetSafeNormal2D());
			Goal = ClampToArena(Loc + Right * AvoidSide * 320.f);
		}
	}
	if (bSteerManually) SteerTowardGoal(Self);
}
void AALHeroAIController::SteerTowardGoal(AALHeroCharacter* Self)
{
	UWorld* W = GetWorld();
	if (!W) return;
	const FVector Loc = Self->GetActorLocation();
	FVector To = Goal - Loc;
	To.Z = 0.f;
	const float Dist = To.Size();
	if (Dist < GoalAccept) { bGoalReached = true; return; }
	FVector Dir = To / Dist;

	FCollisionQueryParams P(SCENE_QUERY_STAT(ALBotSteer), false, Self);
	const FVector ProbeStart = Loc + FVector(0.f, 0.f, -40.f);
	auto Blocked = [&](const FVector& D)
	{
		FHitResult H;
		return W->SweepSingleByChannel(H, ProbeStart, ProbeStart + D * ProbeDist, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(30.f), P);
	};
	if (Blocked(Dir))
	{
		bool bFound = false;
		const float Angles[] = { 50.f, 95.f };
		for (float A : Angles)
		{
			for (int32 s = 0; s < 2 && !bFound; ++s)
			{
				const float Sign = (s == 0) ? AvoidSide : -AvoidSide;
				const FVector Cand = Dir.RotateAngleAxis(A * Sign, FVector::UpVector);
				if (!Blocked(Cand))
				{
					Dir = Cand;
					if (s == 1) AvoidSide = -AvoidSide;
					bFound = true;
				}
			}
			if (bFound) break;
		}
		if (!bFound) Dir = -Dir;
	}
	Self->AddMovementInput(Dir, 1.f);
}
void AALHeroAIController::UpdateCombat(AALHeroCharacter* Self, float DeltaSeconds)
{
	BurstClock += DeltaSeconds;
	AALHeroCharacter* T = Target.Get();
	if (!T || !T->IsAlive()) { bWasVisible = false; return; }
	UWorld* W = GetWorld();
	if (!W) return;
	const float Now = W->GetTimeSeconds();
	const FVector Eye = Self->GetEyeLocation();
	const FVector Aim = T->GetActorLocation() + FVector(0.f, 0.f, 20.f);
	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(ALBotSight), false, Self);
	const bool bBlocked = W->LineTraceSingleByChannel(Hit, Eye, Aim, ECC_Visibility, P) && Hit.GetActor() != T;
	if (bBlocked) { bWasVisible = false; return; }
	if (!bWasVisible) { bWasVisible = true; AcquireTime = Now; }
	LastSeenTime = Now;
	if (CVarALBotDebug.GetValueOnGameThread() != 0)
	{
		DrawDebugLine(W, Eye, Aim, FColor(255, 150, 40), false, -1.f, 0, 0.8f);
	}
	if (FVector::Dist(Eye, Aim) > FireRange || (Now - AcquireTime) < ReactionTime) return;
	// Burst cadence: fire for BurstOn seconds, then pause, so tracers read as volleys rather than a constant beam.
	if (FMath::Fmod(BurstClock, BurstPeriod) > BurstOn) return;
	const float SpreadRad = FMath::DegreesToRadians(FMath::Max(0.f, CVarALBotSpread.GetValueOnGameThread()));
	const FVector Dir = FMath::VRandCone((Aim - Eye).GetSafeNormal(), SpreadRad);
	Self->FireShot(Dir, CVarALBotDamageScale.GetValueOnGameThread());
}
