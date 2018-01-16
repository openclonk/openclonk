/**
	Fast Pump
	If this rule is activate, pumps will pump with an alternative speed.
	This speed can be set in the rule.
	
	@author Maikel
*/


local pump_speed = 50;

public func Initialize()
{
	// Don't do anything if this is not the first rule of this type.
	if (ObjectCount(Find_ID(Rule_FastPump)) > 1) 
		return;
	return;
}

public func Destruction()
{
	// If this is not the last copy of this rule do nothing. 
	if (ObjectCount(Find_ID(Rule_FastPump)) > 1)
		return;
	
	// Reset the pump speed of all pumps.
	for (var pump in FindObjects(Find_ID(Pump)))
		pump.PumpSpeed = Pump.PumpSpeed;
	return;
}

public func OnPumpCreation(object pump)
{
	if (pump_speed != nil)
		pump.PumpSpeed = pump_speed;
	return;
}

public func SetPumpSpeed(int speed)
{
	pump_speed = speed ?? Pump.PumpSpeed;
	pump_speed = Max(0, pump_speed);
	// Adjust the pump speed of existing pumps.
	for (var pump in FindObjects(Find_ID(Pump)))
		pump.PumpSpeed = pump_speed;
	return;
}

public func GetPumpSpeed() { return pump_speed; }

public func Activate(int by_plr)
{
	MessageWindow(this.Description, by_plr);
	return true;
}

// Scenario saving.
public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) return false;
	if (pump_speed != nil)
		props->AddCall("PumpSpeed", this, "SetPumpSpeed", pump_speed);
	return true;
}

// Editor properties.
public func Definition(proplist def)
{
	if (!def.EditorProps)
		def.EditorProps = {};
	def.EditorProps.pump_speed = {Name="$EditorPropPumpSpeed$", Type="int", Min = 0, Step = 10};
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
