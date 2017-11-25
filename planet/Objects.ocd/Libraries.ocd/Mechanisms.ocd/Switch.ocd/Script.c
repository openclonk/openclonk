/**
	Switch
	
	Library for switches. Contains basic functionality:
	- setting the object that gets operated
	- switching the object on/off as a basic term for describing the operation
	
	Needs to call _inherited in the following functions:
	* Construction()
	
	Additionally, if you define editor actions, define them in the Definition()
	call. Otherwise, you will overwrite the editor actions that this library
	defines, namely:
	* connect nearest switch target
	
	@author Marky
*/


local lib_switch;

// Legacy function, so that possible errors are avoided for now
private func SetStoneDoor(object target)
{
	SetSwitchTarget(target);
	return true;
}


/*-- Engine callbacks --*/

public func Construction(object by_object)
{
	_inherited(by_object, ...);
	lib_switch = {
		switch_target = nil,
		invert_signal = false,  // setting this to true inverts the signal
	};
}

/*-- Public Interface --*/

// Sets the object that is operated by this switch
public func SetSwitchTarget(object target)
{
	lib_switch.switch_target = target;
}


// Gets the object that is operated by this switch
public func GetSwitchTarget()
{
	return lib_switch.switch_target;
}


/*
  Switches the object on or off. Does nothing if the object
  is not connected to a switch.

  Forwards the user = the object that is controlling the switch
  and the switch to the switch target. 
*/
public func SetSwitchState(bool state, object by_user)
{
	if (GetSwitchTarget())
	{
		// Invert the state?
		var actual_state = state != lib_switch.invert_signal;
		// Forward to the target
		GetSwitchTarget()->SetInputSignal(by_user, this, actual_state);
	}
}


/*
 Determines whether the switch signal should be inverted.
 
 @par invert true: logic is inverted. SetSwitchState(true) switches the target off.
             false: logic is as usual. SetSwitchState(true) switches the target on.
 */
public func SetInvertSwitchState(bool invert)
{
	lib_switch.invert_signal = invert;
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) return false;
	if (GetSwitchTarget()) props->AddCall("Target", this, "SetSwitchTarget", GetSwitchTarget());
	if (lib_switch.invert_signal) props->AddCall("Invert", this, "SetInvertSwitchState", lib_switch.invert_signal);
	return true;
}


/*-- Editor --*/

public func Definition(proplist def)
{
	// Properties
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.switch_target = { Name = "$SwitchTarget$", Type = "object", Filter = "IsSwitchTarget" };
	def.EditorProps.invert_signal = { Name = "$InvertSignal$", EditorHelp="$InvertSignalDesc$", Type="bool", Set="SetInvertSwitchState" };
	// Actions
	if (!def.EditorActions) def.EditorActions = {};
	def.EditorActions.ConnectClosestSwitchTarget = { Name = "$ConnectClosestSwitchTarget$", Command = "ConnectClosestSwitchTarget()" };
	return _inherited(def, ...);
}

func ConnectClosestSwitchTarget()
{
	// EditCursor helper command: Connect to nearest switch target. Return connected target.
	var target = FindObject(Find_Func("IsSwitchTarget"), Sort_Distance());
	if (target) SetSwitchTarget(target);
	return target;
}
