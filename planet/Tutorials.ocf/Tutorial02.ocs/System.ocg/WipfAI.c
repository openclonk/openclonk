// Artificial intelligence for the wipf.

#appendto Wipf

public func EnableTutorialControl()
{
	RemoveEffect("IntActivity", this);
	AddEffect("TutorialWipf", this, 1, 5, this);
	return;
}

public func DisableTutorialControl()
{
	RemoveEffect("TutorialWipf", this);
	AddEffect("IntActivity", this, 1, 10, this);
	return;
}

public func HadFood() 
{ 
	var effect = GetEffect("TutorialWipf", this);
	if (effect)
		return effect.had_food;
	return false; 
}

protected func FxTutorialWipfStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	effect.sequence = "WaitForFood";
	effect.had_food = false;
	this.Collectible = false;	
	return FX_OK;
}

protected func FxTutorialWipfTimer(object target, proplist effect, int time)
{
	// Wait for some food to appear.
	var food = FindObject(Find_Func("NutritionalValue"), Find_Distance(16), Find_NoContainer());
	if (effect.sequence == "WaitForFood" && food)
	{
		Collect(food, true);
		Eat(food);
		effect.had_food = true;	
		SetCommand("MoveTo", nil, 796, 524);
		effect.sequence = "MoveToBridge";	
	}
	// Move to the bridge.
	if (effect.sequence == "MoveToBridge")
	{
		var clonk = FindObject(Find_OCF(OCF_CrewMember), Find_InRect(AbsX(744), AbsY(480), 80, 48));
		if (Inside(GetX(), 760, 816) && Inside(GetY(), 496, 528) && clonk)
		{
			SetCommand("MoveTo", nil, 992, 524);
			effect.sequence = "MoveToSettlement";
		}
	}
	// Move to settlement.
	if (effect.sequence == "MoveToSettlement")
	{
		
	}
	return FX_OK;
}

protected func FxTutorialWipfStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	this.Collectible = true;
	return FX_OK;
}