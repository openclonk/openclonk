/**
	Construction
	Certain objects have to be constructed to complete this goal.
	
	@author Maikel	
*/

#include Library_Goal


local construction_list;

protected func Initialize()
{
	construction_list = []; 
	return inherited(...);
}

// Add an object to the list of objects that have to be constructed.
public func AddConstruction(id construction, int count)
{
	PushBack(construction_list, {id = construction, count = count});
	return;
}

// Scenario saving
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	for (var con in construction_list)
		props->AddCall("Goal", this, "AddConstruction", con.id, con.count);
	return true;
}

/*-- Goal interface --*/

// The goal is fulfilled if all the objects have been constructed.
public func IsFulfilled()
{
	var is_fulfilled = true;
	for (var con in construction_list)
	{
		if (ObjectCount(Find_ID(con.id)) < con.count)
		{
			is_fulfilled = false;
			break;
		}
	}	
	return is_fulfilled;
}

// Return the description of this goal.
public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = Format("$MsgGoalFulfilled$");	
	else
		message = Format("$MsgGoalUnFulfilled$", GetConstructionString());
	return message;
}

// Returns the objects that need to be constructed as a string.
private func GetConstructionString()
{
	var str = "";
	for (var con in construction_list)
	{
		var clr = RGB(255, 0, 0);
		if (ObjectCount(Find_ID(con.construction)) >= con.count)
			clr = RGB(0, 255, 0);
		str = Format("%s <c %x>%dx</c> {{%i}}", str, clr, con.count, con.id);
	}
	return str;
}

// Shows or hides a message window with information.
public func Activate(int plr)
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
	var message;
	if (IsFulfilled())
		message = Format("@$MsgGoalFulfilled$");	
	else
		message = Format("@$MsgGoalUnFulfilled$", GetConstructionString());
		
	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}

public func GetShortDescription(int plr)
{
	var msg = "";
	return msg;
}

/*-- Proplist --*/

local Name = "$Name$";

func Definition(def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.construction_list = new EditorBase.IDList {};
	return true;
}


