/**
	Mechanism
	
	A mechanism is something that can be controlled via input signals.
	* the input signal is sent by another object, which is usually a switch
	* for simplicity, we currently allow one input signal
	* for simplicity, we currently allow only boolean input signal
	
	Needs to call _inherited in the following functions:
	* Construction()
	
	@author Marky
*/


local lib_mechanism;

/*-- Engine callbacks --*/

public func Construction(object by_object)
{
	_inherited(by_object, ...);
	lib_mechanism = {
		set_plr_view = true,	// sets the player view to this object when the input signal changes
		temp_light = nil,		// temporary light, so that the player can see something when the object is being operated
	};
}

/*-- Public Interface --*/

/*
 Sets the input signal of this mechanism.
 
 This function should handle everything that happens when the input
 signal is set (on the generic level).
 For the object-specific functionality this function issues a callback to the object:
 * OnSetInputSignal(operator, sender, value)
 
 Use the callback to activate or deactivate certain functions in the mechanism.
 For example, the stone door opens if you pass 'true', and closes if you pass 'false'
 
 That callback happens after the generic handling is done, so that the object
 can safely be deleted in the callback.

 @par operator this object is operating the mechanism or the object that sent the signal-
 @par sender this object sent the signal - this is usually a switch.
 @par value this value is sent.
*/
public func SetInputSignal(object operator, object sender, bool value)
{
	// Show the object being operated
	if (operator && lib_mechanism.set_plr_view)
	{
		SetPlrView(operator->GetController(), this);
		if (lib_mechanism.temp_light)
		{
			lib_mechanism.temp_light->RemoveObject();
		}
		lib_mechanism.temp_light = Global->CreateLight(this->GetX(), this->GetY() + this->~GetFloorOffset(), 30, Fx_Light.LGT_Temp, operator->GetController(), 30, 50);
	}

	// Callback: current signal
	this->~OnSetInputSignal(operator, sender, value);
}


/*
 Determines whether the object should show that it is
 being operated when the signal changes.
 
 By default this is set to true.
 */
public func SetPlrViewOnSignalChange(bool show)
{
	lib_mechanism.set_plr_view = show;
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) return false;
	if (lib_mechanism.set_plr_view) props->AddCall("PlrView", this, "SetPlrViewOnSignalChange", lib_mechanism.set_plr_view);
	return true;
}

/*-- Editor --*/

public func Definition(proplist def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.set_plr_view =  { Name = "$SetPlrView$", EditorHelp="$SetPlrViewDesc$", Type="bool", Set="SetPlrViewOnSignalChange" };
	return _inherited(def, ...);
}

/*-- Properties --*/

public func IsSwitchTarget() { return true; }
