#include "ALHUD.h"
#include "ALHeroCharacter.h"
#include "ALGameState.h"
#include "Engine/Canvas.h"

void AALHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	const float SX = Canvas->SizeX;
	const float SY = Canvas->SizeY;
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
	if (const AALHeroCharacter* Hero = Cast<AALHeroCharacter>(GetOwningPawn()))
	{
		Health = Hero->GetMaxHealth() > 0.f ? Hero->GetHealth() / Hero->GetMaxHealth() : 0.f;
		Ult = Hero->UltCharge / 100.f;
	}
	Bar(48.f, SY - 72.f, 280.f, 16.f, Health, Dim, Teal);
	Bar(SX - 328.f, SY - 72.f, 280.f, 10.f, Ult, Dim, Amber);
}

void AALHUD::Bar(float X, float Y, float W, float H, float Fill, FLinearColor Back, FLinearColor Front) const
{
	DrawRect(Back, X, Y, W, H);
	DrawRect(Front, X, Y, W * FMath::Clamp(Fill, 0.f, 1.f), H);
}
