/*-- Bubble --*/

local Plane = 300;

local creator; // The object that has created this bubble. Used to prevent Clonk from breathing from his own bubbles.

global func Bubble(int amount, int x, int y)
{
	if (amount == nil || amount == 0) 
		amount = 3;

	for (var i = 0; i < amount; i++)
	{
		var bubble = CreateObjectAbove(Fx_Bubble, x, y, NO_OWNER);
		if (bubble) bubble.creator = this;
	}
	return;
}

global func CastBubbles(int num, int level, int x, int y)
{
	return CastObjects(Fx_Bubble, num, level, x, y);
}

protected func Initialize()
{
	DoCon(RandomX(25, 100));
	AddEffect("Move", this, 100, 2, this);
	return;
}

public func FxMoveTimer(object target, effect, int time)
{
	if (!GBackLiquid(0, -3) && !GetEffect("Fade", this) || time > 108)
		AddEffect("Fade", target, 100, 1, target);

	// Bubbles burst into smaller bubles
	if (!Random(25) && target->GetCon() > 100)
	{
		for (var i = 0; i < 2; i++)
		{
			var bubble = CreateObjectAbove(Fx_Bubble);
			bubble->SetCon(2 * target->GetCon() / 3);
			bubble->SetYDir(target->GetYDir());
			bubble.creator = this.creator;
		}
		RemoveObject();
		return -1;
	}

	// Jittery movement
	SetYDir(GetYDir() - 3 + Random(7));
	if (Inside(GetXDir(), -6, 6))
		SetXDir(GetXDir() + 2 * Random(2) - 1);

	return 1;
}

public func FxFadeStart(object target, effect, int temporary)
{
	// Store alpha here
	if (temporary == 0)
		effect.alpha = 255;
	return 1;
}

public func FxFadeTimer(object target, effect)
{
	var alpha = effect.alpha;
	if (alpha <= 0)
	{
		RemoveEffect("Move", this);
		RemoveObject();
		return -1;
	}
	SetClrModulation(RGBa(255, 255, 255, alpha));
	effect.alpha = alpha - 5;
	return 1;
}

func OnClonkBreath(object clonk)
{
	// A Clonk is breathing us in
	clonk->DoBreath(GetCon()); // sound would be cool
	RemoveObject();
	return true;
}

// Bubbles can be breathed in by anything but their creator
func CanBeBreathed(object by_clonk) { return !creator || (by_clonk != creator); }

// No need to blow up scenario object list with bubble spam
func SaveScenarioObject() { return false; }
