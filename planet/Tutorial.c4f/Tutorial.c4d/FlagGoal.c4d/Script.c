/*-- 
		Script goal
		Author: Maikel
		
		The goal is fulfilled if all crew members have reached the flag.
--*/


#include Library_Goal

local flag; // Pointer to the flag

public func IsFulfilled() 
{ 
	// No flag, goal fulfilled.
	if (!flag)
		return true;
	// Total crew count.
	var crew_count = ObjectCount(Find_OCF(OCF_CrewMember));
	// Crew count near the flag.
	var flag_count = ObjectCount(Find_OCF(OCF_CrewMember), Find_Distance(50, flag->GetX() - GetX(), flag->GetY() - GetY()));
	// If both counts are equal -> fulfilled.
	if (crew_count != 0 && crew_count == flag_count)
		return true;
	// Otherwise unfulfilled.
	return false;
}

public func CreateGoalFlag(int x, int y)
{
	flag = CreateObject(Goal_Flag, 0, 0, NO_OWNER);
	flag->SetPosition(x, y);
}

protected func Activate(int plr)
{
	return MessageWindow(GetDesc(), plr);
}

/*-- Proplist --*/

func Definition(def)
{
	SetProperty("Name", "$Name$", def);
}
