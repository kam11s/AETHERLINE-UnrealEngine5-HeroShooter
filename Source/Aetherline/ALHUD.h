#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ALHUD.generated.h"

UCLASS()
class AETHERLINE_API AALHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
protected:
	void Bar(float X, float Y, float W, float H, float Fill, FLinearColor Back, FLinearColor Front) const;
};
