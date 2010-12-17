/**
	Power generator
	Should be included by all power generators.
	Explanation of the interface see public funcs below.
	
	@author Maikel
*/


// Local variable to keep track of the power level inside the generator.
local power;

/** Determines whether the object is a power generator.
	@return \c true if the object is a power generator and \c false otherwise.
*/
public func IsPowerGenerator()
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

/** Determines the power capacity of the generator, i.e. the maximum amount of power it can store. Should be overloaded by the generator.
	@return the power capacity.
*/
public func GetCapacity()
{
	return 0;
}

/** Determines the generator's priority, consumers preferably drain from generators with the higher priorities.  Should be overloaded by the generator.
	@return the generator's priority.
*/
public func GetGeneratorPriority()
{
	return 1;
}

/*-- Power network --*/
// Functions to check the power network.

/** Determines whether the calling object is a power generator for the specified consumer.
	@param consumer object for which to check if the generator is connected to it.
	@return \c true if the calling generator is connected to the consumer, \c false otherwise.
*/
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

// Finds all power lines connected to line (can be nil in local calls).
private func Find_PowerLine(object line)
{
	if (!line)
		line = this;
	return [C4FO_Func, "IsConnectedTo", line];
}

/*-- Power generation --*/
// Functions that manipulate the power level.

/** Determines the power level.
	@return the current power level of this object.
*/
public func GetPower()
{
	return power;
}

/** Sets the current power level of the calling object.
	@param to_power the new power level.
	@return nil.
*/
public func SetPower(int to_power)
{
	power = BoundBy(to_power, 0, GetCapacity());
	return;
}


/** Changes the current power level of the calling object.
	@param do_power the amount of power to be added.
	@return nil.
*/
public func DoPower(int do_power)
{
	power = BoundBy(power + do_power, 0, GetCapacity());
	return;
}

/*-- Debug --*/

protected func Initialize()
{
	AddEffect("ShowPower", this, 100, 10, this);
	return _inherited(...);
}

private func FxShowPowerTimer(object target, int num, int time)
{
	Message("Power:%d", target->GetPower());
	return true;
}
