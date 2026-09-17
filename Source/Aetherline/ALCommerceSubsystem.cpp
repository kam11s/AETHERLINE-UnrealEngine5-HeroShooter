#include "ALCommerceSubsystem.h"
void UALCommerceSubsystem::RefreshEntitlements() {}
void UALCommerceSubsystem::RequestBattlePassCheckout() {}
void UALCommerceSubsystem::ReportMatchRewards(bool bWin, int32 Elims) { Credits += (bWin ? 120 : 40) + Elims * 10; }
bool UALCommerceSubsystem::CanUsePremiumTrack() const { return bOwnsBattlePass; }
