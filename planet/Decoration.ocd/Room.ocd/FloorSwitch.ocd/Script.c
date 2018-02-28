/**
	Floor Switch
	A switch which is triggered when sufficient mass lies on top of it.

	@author Maikel
*/

#include Library_Switch


local y_position;
local up_action, down_action;

public func Initialize()
{
	y_position = 0;
	AddTimer("CheckObjects", 5);
	return;
}

public func CheckObjects()
{
	// Find all the objects on the switch.
	var obj_on_switch = FindObjects(Find_InRect(-20, -30, 40, 30), Find_AtRect(-20, -10, 40, 8), Find_NoContainer(), Find_Not(Find_Category(C4D_StaticBack)));
	for (var index = GetLength(obj_on_switch) - 1; index >= 0; index--) 
	{	
		var obj = obj_on_switch[index];
		if (!obj->GetContact(-1, CNAT_Bottom) || obj->Stuck())
			RemoveArrayIndex(obj_on_switch, index);
	}
	// Get the total mass on the switch.
	var total_mass = 0;  
	for (var obj in obj_on_switch)  		
		total_mass += obj->GetMass();
	// Determine desired position.
	var desired_y = 0;
	if (total_mass >= this.SwitchMass)
		desired_y = this.MoveDownDistance;
	// Don't do anything if at desired position.
	if (y_position == desired_y)
		return;
	// Determine movement change and move switch plus object on it.
	var change = BoundBy(desired_y - y_position, -1, 1);
	for (var obj in obj_on_switch)
		obj->SetPosition(obj->GetX(), obj->GetY() + change);
	SetPosition(GetX(), GetY() + change);
	// Do not make objects stuck.
	for (var obj in obj_on_switch)
		if (obj->Stuck())
			obj->SetPosition(obj->GetX(), obj->GetY() - change);
	y_position += change;
	// Do moving of target door or perform user actions.
	if (y_position == desired_y)
	{
		if (desired_y == 0)
		{
			SetSwitchState(false);
			UserAction->EvaluateAction(up_action, this);
		}
		else if (desired_y == this.MoveDownDistance)
		{
			SetSwitchState(true);
			UserAction->EvaluateAction(down_action, this);
		}
	}
	return;
}

public func SetActions(new_up_action, new_down_action)
{
	up_action = new_up_action;
	down_action = new_down_action;
	return true;
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) return false;
	if (up_action || down_action) props->AddCall("Action", this, "SetActions", up_action, down_action);
	if (this.SwitchMass != GetID().SwitchMass)	props->AddSet("SwitchMass", this, "SwitchMass", this.SwitchMass);
	return true;
}


/*-- Editor --*/

public func Definition(proplist def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.up_action = new UserAction.Prop { Name="$UpAction$" };
	def.EditorProps.down_action = new UserAction.Prop { Name="$DownAction$" };
	def.EditorProps.SwitchMass = { Type = "int", Name = "$SwitchMassAction$", EditorHelp = "$SwitchMassActionHelp$" }; 
	return _inherited(def, ...);
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 200;
local SwitchMass = 100;
local MoveDownDistance = 3;
