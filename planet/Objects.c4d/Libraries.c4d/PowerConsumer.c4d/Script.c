/*-- Power consumer --*/

#strict 2

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

// Checks whether there is enough power to sustain iPowerNeed.
// returns true and substracts iPowerNeed if there is enough power, otherwise false.
// fSubstract determines whether the check substracts iPowerNeed.
// If false it starts showing the power need object.
// If true it stops showing the power need object.
public func CheckPower(int iPowerCheck, bool fNoSubstract) 
{
	if(!FindObject(Find_ID(ENRG))) // Rule: Consumers do not need power.
		return true;
	// Check all power generators connected to this consumer and sort them according to priority.
	for(var pGenerator in FindObjects(Find_PowerGenerator(), Sort_GeneratorPriority())) 
	{
		var iPower =  pGenerator -> GetPower();
		if(iPower > iPowerCheck)
		{
			if(!fNoSubstract)
				pGenerator -> DoPower(-iPowerCheck);
			RemoveEffect("EnergyNeed", this);
			return true;
		}
	}
	AddEffect("EnergyNeed", this, 100, 12, this);
	return false;
}

// Finds all power generators connected to pConsumer (can be nil in local calls).
private func Find_PowerGenerator(object pConsumer)
{
  if(!pConsumer) pConsumer = this;
  return [C4FO_Func, "IsPowerGeneratorFor", pConsumer];
}

// Sorts power generators according to GetGeneratorPriority(), highest return value -> first in array.
private func Sort_GeneratorPriority()
{
	return [C4SO_Reverse, [C4SO_Func, "GetGeneratorPriority"]];
}

/*-- Effect to show energy need --*/

private func FxEnergyNeedStart(object pTarget, int iEffectNumber, int iTemp)
{
	// Start showing energy need symbol.
	pTarget->SetGraphics(nil, PWRC, GFX_Overlay, GFXOV_MODE_Base);
	pTarget->SetObjDrawTransform(1000, 0, 0, 0, 1000, -500*GetID()->GetDefCoreVal("Height", "DefCore"), GFX_Overlay);
	EffectVar(0, pTarget, iEffectNumber) = true; // Effect is showing symbol.
	return 1;
}

private func FxEnergyNeedStop(object pTarget, int iEffectNumber, int iReason, bool fTemp)
{
	// Stop showing energy need symbol.
	pTarget->SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
	return 1;
}

private func FxEnergyNeedEffect(string szNewEffectName, object pTarget, int iEffectNumber, int iNewEffectNumber)
{
	// Only one effect per object.
	return -1;
}

private func FxEnergyNeedTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	// Alternate showing of the symbol: iTimerInterval of AddEffect determines alternation time.
	if(EffectVar(0, pTarget, iEffectNumber)) // Effect was showing symbol.
	{
		// Do not show symbol.
		pTarget->SetGraphics(nil, nil, GFX_Overlay, GFXOV_MODE_Base);
		EffectVar(0, pTarget, iEffectNumber) = false;
	} 
	else // Effect was not showing symbol.
	{	
		// Do show symbol.
		pTarget->SetGraphics(nil, PWRC, GFX_Overlay, GFXOV_MODE_Base);
		pTarget->SetObjDrawTransform(1000, 0, 0, 0, 1000, -500*GetID()->GetDefCoreVal("Height", "DefCore"), GFX_Overlay);
		EffectVar(0, pTarget, iEffectNumber) = true;
	}
	return 1;
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
	
