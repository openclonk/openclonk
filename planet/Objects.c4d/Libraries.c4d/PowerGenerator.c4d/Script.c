/*--
	Power generator
	Author: Maikel
	
	To be included by all objects which are power generators.
	Explanation of the interface see public funcs below.
--*/


// Local variable to keep track of the power level inside the generator.
local iPower;

/*-- Public calls --*/
// Functions that specify object properties, should be overloaded by the generator.

// Maximum amount of power that can be stored in this power generator.
public func GetCapacity() 
{
	return 0;
}

// This object is a power generator.
public func IsPowerGenerator()
{
	return true;
}

// Returns if a power line can be connected to this object.
public func CanPowerConnect() // Other name?
{
	return GetCon() >= 100;
}

// Returns the generator's priority, consumers preferably drain from generators with the highest priority.
public func GetGeneratorPriority()
{
	return 1;
}

// Returns whether this object is a power genarator connected to pConsumer.
// The other three Parameters pNext, pOldLine and aOld are only used for recursive purposes.
public func IsPowerGeneratorFor(object pConsumer, object pNext, object pOldLine, array aOld)
{
	if(!pNext) // Initial call to this function.
	{
		pNext = pConsumer;
		aOld = [];
	}
	for(var pLine in FindObjects(Find_PowerLine(pNext))) // Check all lines connected to pNext.
	{
		if(pLine == pOldLine) // Recursive -> Not backwards<->forwards through lines.
			continue;
		//if(!pLine -> IsConnectedWith(pNext)) // Power line connected with pConsumer.
			//continue;
		var pEnd = pLine -> GetConnectedObject(pNext); // What is on the line's other end.
		if(!pEnd) // Nothing on the other end.
			continue;		
		if(pEnd == pConsumer) // End of a recursive loop.
			continue;
		if(GetIndexOf(pEnd, aOld) != -1) // We already know this
			continue;
		if(pEnd == this) // Found this object, i.e. the generator.
			return true;
		aOld[GetLength(aOld)] = pEnd;
		if(IsPowerGeneratorFor(pConsumer, pEnd, pLine, aOld)) // This building is not found, continue with next pEnd as next building.
			return true;		
	}
	return false;
}

// Finds all power lines connected to pObject (can be nil in local calls).
private func Find_PowerLine(object pObject)
{
	if(!pObject) pObject = this;
	return [C4FO_Func, "IsConnectedTo", pObject];
}

/*-- Power generation --*/
// Functions that manipulate the power level.

// Returns the current power level of this object.
public func GetPower()
{
	return iPower;
}

// Sets the current power level of this object.
public func SetPower(int iSetPower)
{
	iPower = BoundBy(iSetPower, 0, GetCapacity());
	return;
}

// Adds to the current power level of this object.
public func DoPower(int iDoPower)
{
	iPower = BoundBy(iPower + iDoPower, 0, GetCapacity());
	return;
}

/*-- Debug --*/

protected func Initialize() 
{
	//AddEffect("ShowPower",this,100,10,this);
	return _inherited(...);
}

private func FxShowPowerTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	Message("P:%d", pTarget, pTarget->GetPower());
	return true;
}
