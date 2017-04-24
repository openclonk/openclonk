/**
	Acid bubble
	Looks dangerous

	@author Win
*/


public func Construction()
{
	var effect = AddEffect("Move", this, 1, 2, this);
	// Sometimes bubbles become darker and explosive.
	if (!Random(10))
	{
		effect.is_explosive = true;
		SetGraphics("2");
	}
	return;
}

private func FxMoveTimer(object target, effect fx, int time)
{
	// Fade out bubble if outside liquid or time is up.
	if ((!GBackLiquid(0, -3) || time > 200) && !GetEffect("ObjFade", this))
	{
		if (!GBackLiquid(0, -3) && !fx.is_explosive)
			SetGraphics("3");
		Sound("Liquids::Bubble*");
		FadeOut(50, true);
	}
	
	// Grow bubble over time.
	if ((time % 6) == 0)
		DoCon(2);
	
	// Causes bubbles to repel each other.
	var nearby_bubble = FindObject(Find_Distance(GetCon() / 15), Find_ID(GetID()));
	if (nearby_bubble)
	{
		SetXDir(-(nearby_bubble->GetX() - GetX()) / 2);
		SetYDir(-(nearby_bubble->GetY() - GetY()) / 2);
	}
	// Deep green bubbles explode when near a living thing.
	if (fx.is_explosive)
	{
		var prey = FindObject(Find_Distance(GetCon() / 15), Find_OCF(OCF_Alive), Find_Not(Find_Property("CorrosionResist")));
		if (prey)
		{
			Explode(10);
			return FX_OK;	
		}
	}
	
	// Bubble is faster in acid, moves erratically outside.
	var speed_up = -3;
	if (GetEffect("ObjFade", this))
	{
		speed_up = 0;
		if (Inside(GetXDir(), -6, 6))
			SetXDir(GetXDir() + RandomX(-6, 6));
	}
	
	SetYDir(GetYDir() - RandomX(3, 4) + speed_up - (GetCon() / 50));
	return FX_OK;
}

// No need to blow up scenario object list with bubble spam.
func SaveScenarioObject() { return false; }

/*-- Properties --*/

local Plane = 500;
