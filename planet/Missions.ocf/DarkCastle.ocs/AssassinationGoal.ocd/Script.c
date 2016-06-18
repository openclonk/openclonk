/*--
		Assassination goal
		Author: Sven2
		
		Target object must die.
--*/


#include Library_Goal

local victim, victim_name;

protected func Initialize()
{
	return inherited(...);
}

func SetVictim(object to_victim)
{
	victim = to_victim;
	if (victim) victim_name = victim->GetName(); else victim_name = "???";
	SetName(Format("$Name2$", victim_name));
	return true;
}

// Scenario saving
public func SaveScenarioObject(props, ...)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Name");
	if (victim) props->AddCall("Goal", this, "SetVictim", victim);
	return true;
}

public func IsFulfilled()
{
	if (!victim) return true; // pushed out of landscape?
	return !victim->GetAlive();
}

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = Format("$MsgGoalFulfilled$", victim_name);
	else
		message = Format("$MsgGoalUnFulfilled$", victim_name);
	return message;
}

public func Activate(int byplr)
{
	if (IsFulfilled())
		ToggleGoalMessage(Format("$MsgGoalFulfilled$", victim_name), byplr);
	else
		ToggleGoalMessage(Format("$MsgGoalUnFulfilled$", victim_name), byplr);
	return;
}

// Shows or hides a message window with information.
private func ToggleGoalMessage(string msg, int plr)
{
	// If goal message open -> hide it.
	if (GetEffect("GoalMessage", this))
	{
		CustomMessage("", nil, plr, nil, nil, nil, nil, nil, MSG_HCenter);
		RemoveEffect("GoalMessage", this);
		return;
	}
	// Otherwise open a new message.
	AddEffect("GoalMessage", this, 100, 0, this);
	CustomMessage(Format("@%s", msg), nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}


public func GetShortDescription(int plr)
{
	return Name;
}

/*-- Proplist --*/
local Name = "$Name$";
