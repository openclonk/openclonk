/* Recharge callback in clonk if ammo has been used up */

#appendto GrenadeLauncher

func Ejection()
{
	var cont = Contained();
	if (cont)
	{
		if (!Contents() && cont->GetAlive()) RechargeIronBomb();
	}
	return _inherited(...);
}

func ContentsDestruction()
{
	var cont = Contained();
	if (cont)
	{
		if (ContentsCount()<=1 && cont->GetAlive()) RechargeIronBomb();
	}
	return _inherited(...);
}

// Recreate ammo - but max one per frame (also to ensure we don't run into endless loops when destructing)
func RechargeIronBomb()
{
	if (!g_homebases)
		return;
	var t = FrameCounter();
	if (this.last_iron_bomb_recharge == t) return nil;
	this.last_iron_bomb_recharge = t;
	return CreateContents(IronBomb);
}
