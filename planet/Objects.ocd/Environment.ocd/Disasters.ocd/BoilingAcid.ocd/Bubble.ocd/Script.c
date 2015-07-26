/**
	Acid bubble
	Looks dangerous

	@author Win
*/

local Plane = 500;

local is_explosive = false;

local grow_time = 2;
local grow_timer = 0;

public func Construction()
{
	AddEffect("Move", this, 1, 2, this);
	
	// Sometimes bubbles become darker and explosive
	if (!Random(10))
	{
		is_explosive = true;
		SetGraphics("2");
	}
	return;
}

private func FxMoveTimer(object target, effect fx, int time)
{
	if (!GBackLiquid(0, -3) && !GetEffect("ObjFade", this) || time > 200)
	{
		if (!GBackLiquid(0, -3) && !is_explosive)
			SetGraphics("3");
		if(!Random(20))
			Sound("ef_Bubble*");
		FadeOut(50, true);
	}
	
	grow_timer++;
	
	if (grow_timer >= grow_time)
	{
		DoCon(2);
		grow_timer = 0;
	}
	
	// Causes bubbles to repel each other
	var nearbyBubble = FindObject(Find_Distance(GetCon()/15, 0, 0), Find_ID(GetID()));
	if (nearbyBubble != nil)
	{
		SetXDir(-(nearbyBubble->GetX() - GetX()) / 2);
		SetYDir(-(nearbyBubble->GetY() - GetY()) / 2);
	}
	// Deep green bubbles explode when near a living thing
	if (is_explosive)
	{
		var prey = FindObject(Find_Distance(GetCon()/15, 0, 0), Find_OCF(OCF_Alive));
		if (prey != nil)
			Explode(10);
	}
	
	// Bubble is faster in acid, moves erratically outside
	var speed_up = -3;
	if (GetEffect("ObjFade", this))
	{
		speed_up = 0;
		if (Inside(GetXDir(), -6, 6))
			SetXDir(GetXDir() + RandomX(-6, 6));
	}
	
	
	SetYDir(GetYDir() - RandomX(3, 4) + speed_up - (GetCon() / 50));
	
	return 1;
}

// No need to blow up scenario object list with bubble spam.
func SaveScenarioObject() { return false; }
