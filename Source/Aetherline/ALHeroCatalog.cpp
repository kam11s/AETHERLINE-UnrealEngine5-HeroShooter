#include "ALHeroCatalog.h"
FALHeroDef UALHeroCatalog::Get(EALHero Hero)
{
	FALHeroDef D; D.Id = Hero;
	switch (Hero)
	{
	case EALHero::Bastion: D.DisplayName = TEXT("Bastion"); D.MaxHealth = 275.f; D.MoveSpeed = 520.f; D.FireRate = 1.2f; D.Damage = 70.f; break;
	case EALHero::Pulse: D.DisplayName = TEXT("Pulse"); D.MaxHealth = 200.f; D.FireRate = 11.f; D.Damage = 12.f; break;
	default: D.DisplayName = TEXT("Wraith"); D.MaxHealth = 200.f; D.MoveSpeed = 650.f; D.FireRate = 9.f; D.Damage = 17.f; break;
	}
	return D;
}
