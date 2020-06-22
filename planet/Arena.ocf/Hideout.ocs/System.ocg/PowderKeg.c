// Infinite gunpowder for the cannons.

#appendto PowderKeg

protected func Initialize()
{
	AddEffect("NoRemoval", this, 100, nil, this);
	return _inherited(...);
}

protected func FxNoRemovalStop(object target, effect, int reason)
{
	if (reason == 3)
	{
		var container = target->Contained();
		if (container)
		{
			container->CreateContents(PowderKeg);
		}	
	}
	return 1;
}
