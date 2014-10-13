#appendto Club

func ApplyWeaponBash(clonk, amount, angle)
{
	var r = inherited(clonk, amount, angle, ...);
	if (this.IsAIWeapon)
	{
		// Ogre bash!
		Sound("OgreClubHit");
		var speed = 600;
		clonk->Fling(Sin(angle, speed), Cos(angle, speed), 10, true);
	}
	return r;
}
