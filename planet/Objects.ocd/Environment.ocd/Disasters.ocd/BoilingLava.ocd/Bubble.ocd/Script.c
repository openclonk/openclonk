/**
	Lava Bubble
	Looks dangerous

	@author Win
*/

local Name = "Lava Bubble";
local Plane = 500;

public func Construction()
{
	if (GBackSolid(0, 0))
	{
		RemoveObject();
		return;
	}
	
	AddEffect("Move", this, 1, 2, this);
	Sound("Liquids::Bubble*");
	return;
}

private func FxMoveTimer(object target, effect, int time)
{
	if ((!GBackLiquid(0, -3) || time > 108) && !GetEffect("ObjFade", this))
		FadeOut(50, true);

	// Bubbles burst into smaller bubbles
	if (!Random(25) && target->GetCon() > 100)
	{
		for (var i = 0; i < 2; i++)
		{
			var bubble = CreateObjectAbove(GetID());
			// Bubble could have been removed in its construction call.
			if (bubble)
			{
				bubble->SetCon(2 * target->GetCon() / 3);
				bubble->SetYDir(target->GetYDir());
			}
		}
		RemoveObject();
		return FX_Execute_Kill;
	}

	// Jittery movement
	SetYDir(GetYDir() - 3 + Random(7));
	if (Inside(GetXDir(), -6, 6))
		SetXDir(GetXDir() + 2 * Random(2) - 1);
	
	// Explodes near living things.
	var prey = FindObject(Find_Distance(GetCon() / 15), Find_OCF(OCF_Alive), Find_Not(Find_Property("CorrosionResist")));
	if (prey)
		Explode(10);
	return FX_OK;
}

// No need to blow up scenario object list with bubble spam.
func SaveScenarioObject() { return false; }
