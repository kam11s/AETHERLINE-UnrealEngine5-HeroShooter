#include "ALGameInstance.h"
#include "ALCommerceSubsystem.h"
#include "Kismet/GameplayStatics.h"
void UALGameInstance::QueuePlaylist(EALPlaylist Playlist)
{
	SelectedPlaylist = Playlist;
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Engine/Maps/Templates/Template_Default")));
}
void UALGameInstance::GrantMatchRewards(bool bWin, int32 Elims)
{
	if (UALCommerceSubsystem* Commerce = GetSubsystem<UALCommerceSubsystem>())
	{
		Commerce->ReportMatchRewards(bWin, Elims);
		Credits = Commerce->Credits;
	}
	BattlePassXP += 40 + Elims * 8 + (bWin ? 30 : 0);
	while (BattlePassXP >= 100 && BattlePassTier < 30) { BattlePassXP -= 100; BattlePassTier += 1; }
}
bool UALGameInstance::OwnsPremiumBattlePass() const
{
	if (const UALCommerceSubsystem* Commerce = GetSubsystem<UALCommerceSubsystem>()) return Commerce->CanUsePremiumTrack();
	return false;
}
