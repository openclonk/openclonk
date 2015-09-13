// Artificial intelligence for the wipf.

#appendto Wipf

local had_food = false;

protected func Initialize()
{
	_inherited(...);
	
	RemoveEffect("Activity", this);
	
	AddEffect("TutorialWipf", this, 1, 5, this);
	return;
}

public func HadFood() { return had_food; }

protected func FxTutorialWipfStart(object target, proplist effect, int temp)
{
	if (temp)
		return;
		
	effect.Sequence = "WaitForFood";	
	return FX_OK;
}

protected func FxTutorialWipfTimer(object target, proplist effect, int time)
{
	// Wait for some food to appear.
	var food = FindObject(Find_Func("NutritionalValue"), Find_Distance(16), Find_NoContainer());
	if (effect.Sequence == "WaitForFood" && food)
	{
		Collect(food, true);
		had_food = true;	
		//SetCommand("Follow", FindObject(Find_OCF(OCF_CrewMember)));
		effect.Sequence = "MoveToLoam";
	}
	// Move down to the loam.
	if (effect.Sequence == "MoveToLoam" )
	{
		SetCommand("MoveTo", nil, 488, 620);
		if (Inside(GetX(), 472, 520) && Inside(GetY(), 600, 632) && PathFree(484, 616, 544, 602))
			effect.Sequence = "MoveToBridge";
	}
	// Move to the bridge.
	if (effect.Sequence == "MoveToBridge")
	{
		SetCommand("MoveTo", nil, 796, 524);
		var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(AbsX(744), AbsY(480), 80, 48));
		if (Inside(GetX(), 760, 816) && Inside(GetY(), 496, 528) && clonk)
			effect.Sequence = "MoveToSettlement";
	}
	// Move to settlement.
	if (effect.Sequence == "MoveToSettlement")
	{
		SetCommand("MoveTo", nil, 992, 524);
	}
	return 1;
}