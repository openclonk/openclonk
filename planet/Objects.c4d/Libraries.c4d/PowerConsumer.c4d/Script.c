/**
	Power consumer
	Should be included by all power consumers. The consumer can check and consume power by calling
	the function CheckPower(int power_check, bool no_substract), see below for an explanation.
	
	@author Maikel
*/


/** Determines whether the object is a power consumer.
	@return \c true if the object is a power consumer and \c false otherwise.
*/
public func IsPowerConsumer()
{
	return true;
}

/** Determines whether a power line can be connected.
	@return \c true if a power line can be connected to this object, \c false otherwise.
*/
public func CanPowerConnect() // Other name?
{
	return GetCon() >= 100;
}

/*-- Power consumption --*/

/** Inspects the network connected to the calling consumer for power.
	@param power_check amount of power to extract from the network.
	@param no_substract determines whether the check should substract power_check.
	@return \c true if the amount of power was available in the network, \c false otherwise.
*/
public func CheckPower(int power_check, bool no_substract)
{
	// Power consumer only need energy if the rule is active.
	if (!FindObject(Find_ID(Rule_EnergyNeed)))
		return true;
	// Check all power generators connected to this consumer and sort them according to priority.
	// TODO: Maybe substract partial amounts from high priority generators.
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

// Finds all power generators connected to consumer (can be nil in local calls).
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

private func FxEnergyNeedStart(object target, int fxnum, int temp)
{
	// Start showing energy need symbol.
	target->SetGraphics(nil, Library_PowerConsumer, GFX_Overlay, GFXOV_MODE_Base);
	target->SetObjDrawTransform(1000, 0, 0, 0, 1000, -500 * GetID()->GetDefCoreVal("Height", "DefCore"), GFX_Overlay);
	fxnum.var0 = true; // Effect is showing symbol.
	return 1;
}

private func FxEnergyNeedTimer(object target, int fxnum, int time)
{
	// Alternate showing of the symbol: timer interval of AddEffect determines alternation time.
	if (fxnum.var0) // Effect was showing symbol.
	{
		// Do not show symbol.
		target->SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
		fxnum.var0 = false;
	}
	else // Effect was not showing symbol.
	{
		// Do show symbol.
		target->SetGraphics(nil, Library_PowerConsumer, GFX_Overlay, GFXOV_MODE_Base);
		target->SetObjDrawTransform(1000, 0, 0, 0, 1000, -500 * GetID()->GetDefCoreVal("Height", "DefCore"), GFX_Overlay);
		fxnum.var0 = true;
	}
	return 1;
}

private func FxEnergyNeedStop(object target, int fxnum, int iReason, bool temp)
{
	// Stop showing energy need symbol.
	target->SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
	return 1;
}

private func FxEnergyNeedEffect(string name, object target)
{
	// Only one energy need effect per consumer.
	if (name == "EnergyNeed")
		return -1;
	return 1;
}


