/*--
	Power generator
	Author: Maikel
	
	To be included by all objects which are power generators.
	Explanation of the interface see public funcs below.
--*/


// Local variable to keep track of the power level inside the generator.
local power;

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

// Returns whether this object is a power genarator connected to consumer.
// The other three Parameters next, old_line and line_list are only used for recursive purposes.
public func IsPowerGeneratorFor(object consumer, object next, object old_line, array pwr_list)
{
	if (!next) // Initial call to this function.
	{
		next = consumer;
		pwr_list = [];
	}
	for (var line in FindObjects(Find_PowerLine(next))) // Check all lines connected to next.
	{
		if (line == old_line) // Recursive -> Not backwards<->forwards through lines.
			continue;
		//if (!line->IsConnectedWith(next)) // Power line connected with consumer.
		//	continue;
		var end = line -> GetConnectedObject(next); // What is on the line's other end.
		if (!end) // Nothing on the other end.
			continue;		
		if (end == consumer) // End of a recursive loop.
			continue;
		if (GetIndexOf(end, pwr_list) != -1) // We already know this.
			continue;
		if (end == this) // Found this object, i.e. the generator.
			return true;
		pwr_list[GetLength(pwr_list)] = end;
		if (IsPowerGeneratorFor(consumer, end, line, pwr_list)) // This building is not found, continue with next end as next building.
			return true;		
	}
	return false;
}

// Finds all power lines connected to pObject (can be nil in local calls).
private func Find_PowerLine(object line)
{
	if (!line)
		line = this;
	return [C4FO_Func, "IsConnectedTo", line];
}

/*-- Power generation --*/
// Functions that manipulate the power level.

// Returns the current power level of this object.
public func GetPower()
{
	return power;
}

// Sets the current power level of this object.
public func SetPower(int to_power)
{
	power = BoundBy(to_power, 0, GetCapacity());
	return;
}

// Adds to the current power level of this object.
public func DoPower(int do_power)
{
	power = BoundBy(power + do_power, 0, GetCapacity());
	return;
}

/*-- Debug --*/

protected func Initialize()
{
	//AddEffect("ShowPower",this,100,10,this);
	return _inherited(...);
}

private func FxShowPowerTimer(object trg, int num, int time)
{
	Message("P:%d", trg->GetPower());
	return true;
}
