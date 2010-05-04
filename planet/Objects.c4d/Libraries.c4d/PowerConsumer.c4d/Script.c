/*--
	Power consumer
	Author: Maikel
	
	To be included by all objects that need a supply of energy. 
	The object then can check and consume power by calling it's
	function CheckPower(int power_check, bool no_substract),
	see below for an explanation.	
--*/


/*-- Public calls --*/
// Functions that specify object properties, should be overloaded by the consumer.

// Returns whether the object is a power consumer.
public func IsPowerConsumer() 
{
	return true;
}

// Returns if a power line can be connected to this object.
public func CanPowerConnect() // Other name?
{
	return GetCon() >= 100;
}

/*-- Power consumption --*/

// Checks whether there is enough power to sustain powerNeed.
// returns true and substracts powerNeed if there is enough power, otherwise false.
// fSubstract determines whether the check substracts powerNeed.
// If false it starts showing the power need object.
// If true it stops showing the power need object.
public func CheckPower(int power_check, bool no_substract) 
{
	if (!FindObject(Find_ID(Rule_NeedEnergy))) // Rule: Consumers do not need power.
		return true;
	// Check all power generators connected to this consumer and sort them according to priority.
	for (var generator in FindObjects(Find_PowerGenerator(), Sort_GeneratorPriority())) 
	{
		var power = generator->GetPower();
		if (power > power_check)
		{
			if (!no_substract)
				generator->DoPower(-power_check);
			RemoveEffect("EnergyNeed", this);
			return true;
		}
	}
	AddEffect("EnergyNeed", this, 100, 12, this);
	return false;
}

// Finds all power generators connected to pConsumer (can be nil in local calls).
private func Find_PowerGenerator(object consumer)
{
	if (!consumer)
		consumer = this;
	return [C4FO_Func, "IsPowerGeneratorFor", consumer];
}

// Sorts power generators according to GetGeneratorPriority(), highest return value -> first in array.
private func Sort_GeneratorPriority()
{
	return [C4SO_Reverse, [C4SO_Func, "GetGeneratorPriority"]];
}

/*-- Effect to show energy need --*/

private func FxEnergyNeedStart(object trg, int fxnum, int temp)
{
	// Start showing energy need symbol.
	trg->SetGraphics(nil, Library_PowerConsumer, GFX_Overlay, GFXOV_MODE_Base);
	trg->SetObjDrawTransform(1000, 0, 0, 0, 1000, -500 * GetID()->GetDefCoreVal("Height", "DefCore"), GFX_Overlay);
	EffectVar(0, trg, fxnum) = true; // Effect is showing symbol.
	return 1;
}

private func FxEnergyNeedStop(object trg, int fxnum, int iReason, bool temp)
{
	// Stop showing energy need symbol.
	trg->SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
	return 1;
}

private func FxEnergyNeedEffect(string name, object trg, int fxnum, int fxnum_new)
{
	// Only one effect per object.
	return -1;
}

private func FxEnergyNeedTimer(object trg, int fxnum, int time)
{
	// Alternate showing of the symbol: timer interval of AddEffect determines alternation time.
	if (EffectVar(0, trg, fxnum)) // Effect was showing symbol.
	{
		// Do not show symbol.
		trg->SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
		EffectVar(0, trg, fxnum) = false;
	} 
	else // Effect was not showing symbol.
	{	
		// Do show symbol.
		trg->SetGraphics(nil, Library_PowerConsumer, GFX_Overlay, GFXOV_MODE_Base);
		trg->SetObjDrawTransform(1000, 0, 0, 0, 1000, -500 * GetID()->GetDefCoreVal("Height", "DefCore"), GFX_Overlay);
		EffectVar(0, trg, fxnum) = true;
	}
	return 1;
}
