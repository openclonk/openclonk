/*-- Bubble --*/


global func Bubble(int amount, int x, int y)
{
	if (amount==nil || amount==0) 
		amount=3;

	for (var i = 0; i < amount; i++)
		CreateObject(Fx_Bubble, x, y, NO_OWNER);
	return;
}

protected func Initialize()
{
	DoCon(RandomX(25, 100));
	AddEffect("Move", this, 100, 1, this);
	return;
}

public func FxMoveTimer(object target, effect, int time)
{
	if (!GBackLiquid(0, -3) && !GetEffect("Fade", this) || time > 108)
		AddEffect("Fade", target, 100, 1, target);

	// Bubbles burst into smaller bubles
	if (!Random(30) && target->GetCon() > 100)
	{
		for (var i = 0; i < 3; i++)
		{
			var bubble = CreateObject(Fx_Bubble);
			bubble->SetCon(10 * target->GetCon() / 15);
			bubble->SetYDir(target->GetYDir());
		}
		RemoveObject();
		return -1;
	}

	// Jittery movement
	SetYDir(GetYDir() - 2 + Random(5));
	if (Inside(GetXDir(), -6, 6))
		SetXDir(GetXDir() + 2 * Random(2) - 1);

	return 1;
}

public func FxFadeStart(object target, effect, int temporary)
{
	// Store alpha here
	if (temporary == 0)
		effect.var0 = 255;
	return 1;
}

public func FxFadeTimer(object target, effect)
{
	var alpha = effect.var0;
	if (alpha <= 0)
	{
		RemoveEffect("Move", this);
		RemoveObject();
		return -1;
	}
	SetClrModulation(RGBa(255, 255, 255, alpha));
	effect.var0 = alpha - 5;
	return 1;
}