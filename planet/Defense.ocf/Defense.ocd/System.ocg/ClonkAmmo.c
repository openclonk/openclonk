/* Recharge callback in clonk if ammo has been used up */

#appendto Clonk

func Ejection()
{
	if (!Contents() && g_homebases && g_homebases[GetOwner()])
		if (GetAlive() && GetPlayerType(GetOwner()) == C4PT_User)
			g_homebases[GetOwner()]->OnNoAmmo(this);
	return _inherited(...);
}

func ContentsDestruction()
{
	if (ContentsCount() <= 1 && g_homebases && g_homebases[GetOwner()])
		if (GetAlive() && GetPlayerType(GetOwner()) == C4PT_User)
			g_homebases[GetOwner()]->OnNoAmmo(this);
	return _inherited(...);
}
