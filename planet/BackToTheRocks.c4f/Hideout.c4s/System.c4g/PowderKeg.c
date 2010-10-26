// Infinite gunpowder for the cannons.

#appendto PowderKeg

protected func Initialize()
{
	AddEffect("NoRemoval", this, 100, nil, this);
	return _inherited(...);
}

protected func FxNoRemovalStop(object target, int num, int reason)
{
	if (reason == 3)
	{
		var container = target->Contained();
		if (!container)
			return 1;
		container->CreateContents(PowderKeg);		
	}
	return 1;
}