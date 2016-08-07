/**
	Refinery Goal
	A certain amount of oil has to be pumped into the refinery drain.
		
	@author Maikel
*/


#include Library_Goal

local goal_amount;

protected func Initialize()
{
	goal_amount = 0;
	return _inherited(...);
}

public func SetGoalAmount(int amount)
{
	goal_amount = amount;
	return;
}

private func GetPumpedAmount()
{
	var refinery_drain = FindObject(Find_ID(RefineryDrain));
	if (!refinery_drain)
		return 0;
	return refinery_drain->GetOilAmount();
}

public func IsFulfilled()
{
	return GetPumpedAmount() >= goal_amount;
}

public func GetDescription(int plr)
{
	if (IsFulfilled())
		return "$DescriptionCompleted$";
	return Format("$Description$", GetPumpedAmount(), goal_amount);
}


/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
