#include "ALHUD.h"
#include "ALHeroCharacter.h"
#include "ALGameState.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"

void AALHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	const float SX = static_cast<float>(Canvas->SizeX);
	const float SY = static_cast<float>(Canvas->SizeY);
	const FLinearColor Teal(0.18f, 0.90f, 1.0f);
	const FLinearColor Amber(0.89f, 0.60f, 0.18f);
	const FLinearColor Dim(0.02f, 0.03f, 0.04f, 0.55f);
	const float Cx = SX * 0.5f;
	const float Cy = SY * 0.5f;

	DrawLine(Cx - 10.f, Cy, Cx - 3.f, Cy, Teal, 1.4f);
	DrawLine(Cx + 3.f, Cy, Cx + 10.f, Cy, Teal, 1.4f);
	DrawLine(Cx, Cy - 10.f, Cx, Cy - 3.f, Teal, 1.4f);
	DrawLine(Cx, Cy + 3.f, Cx, Cy + 10.f, Teal, 1.4f);

	float Health = 1.f;
	float Ult = 0.f;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (const AALHeroCharacter* Hero = Cast<AALHeroCharacter>(GetOwningPawn()))
	{
		Health = Hero->GetMaxHealth() > 0.f ? Hero->GetHealth() / Hero->GetMaxHealth() : 0.f;
		Ult = Hero->UltCharge / 100.f;
		DamageFeedback(Hero, Now, SX, SY);
	}
	Bar(48.f, SY - 72.f, 280.f, 16.f, Health, Dim, Teal);
	Bar(SX - 328.f, SY - 72.f, 280.f, 10.f, Ult, Dim, Amber);

	DrawText(TEXT("RAVELIN"), Teal, 48.f, SY - 96.f);

	int32 Hostiles = 0;
	for (TActorIterator<AALHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (It->TeamId == EALTeam::Enemy && It->IsAlive()) ++Hostiles;
	}
	DrawText(FString::Printf(TEXT("HOSTILES %d"), Hostiles), Amber, SX - 160.f, 28.f);

	if (const AALGameState* GS = GetWorld() ? GetWorld()->GetGameState<AALGameState>() : nullptr)
	{
		if (GS->IsBattleRoyale())
		{
			DrawText(FString::Printf(TEXT("ALIVE %d   CIRCLE %.0f"), GS->AlivePlayers, GS->CircleRadius), Amber, Cx - 120.f, 24.f);
			if (const AALHeroCharacter* Hero = Cast<AALHeroCharacter>(GetOwningPawn()))
			{
				if (Hero->bOnDropship)
				{
					DrawText(TEXT("JUMP TO DEPLOY"), Amber, Cx - 80.f, SY * 0.62f);
				}
			}
		}
		else
		{
			DrawText(FString::Printf(TEXT("%d    %d"), GS->AllyScore, GS->EnemyScore), Teal, Cx - 40.f, 28.f);
		}
	}
}

void AALHUD::Bar(float X, float Y, float W, float H, float Fill, FLinearColor Back, FLinearColor Front)
{
	DrawRect(Back, X, Y, W, H);
	DrawRect(Front, X, Y, W * FMath::Clamp(Fill, 0.f, 1.f), H);
}

void AALHUD::DamageFeedback(const AALHeroCharacter* Hero, float Now, float SX, float SY)
{
	const float Cx = SX * 0.5f;
	const float Cy = SY * 0.5f;
	const FLinearColor Amber(0.89f, 0.60f, 0.18f);

	// Amber-red edge flash when the player takes damage.
	const float SinceHit = Now - Hero->LastDamagedTime;
	if (SinceHit >= 0.f && SinceHit < 0.5f)
	{
		const FLinearColor Flash(0.95f, 0.30f, 0.08f, (1.f - SinceHit / 0.5f) * 0.55f);
		const float T = 26.f;
		DrawRect(Flash, 0.f, 0.f, SX, T);
		DrawRect(Flash, 0.f, SY - T, SX, T);
		DrawRect(Flash, 0.f, 0.f, T, SY);
		DrawRect(Flash, SX - T, 0.f, T, SY);
	}

	// Diagonal hit marker when the player's shot lands on a hostile.
	const float SinceConfirm = Now - Hero->LastHitConfirmTime;
	if (SinceConfirm >= 0.f && SinceConfirm < 0.18f)
	{
		DrawLine(Cx - 16.f, Cy - 16.f, Cx - 7.f, Cy - 7.f, Amber, 2.f);
		DrawLine(Cx + 16.f, Cy - 16.f, Cx + 7.f, Cy - 7.f, Amber, 2.f);
		DrawLine(Cx - 16.f, Cy + 16.f, Cx - 7.f, Cy + 7.f, Amber, 2.f);
		DrawLine(Cx + 16.f, Cy + 16.f, Cx + 7.f, Cy + 7.f, Amber, 2.f);
	}

	if (!Hero->IsAlive())
	{
		DrawText(TEXT("DOWN  -  RESPAWNING"), Amber, Cx - 90.f, Cy + 40.f);
	}
}
