/**
	Lamp Library
	Include this by all lamps that should hang outside buildings.
	
	@author Clonkonaut
*/

/** Must define a dummy definition (i.e. the object which is shown outside the building)
*/
public func DummyDefinition() {}

/** Whether or not the lamp is on (true then).
*/
local lib_lamp_lit;

/** This is a lamp.
*/
public func IsLamp() { return true; }

/** Lamp property: How far it shines.
*/
public func GetLampRange() { return 160; }

/** Lamp property: How far it shines into material.
*/
public func GetLampFadeRange() { return 160; }

/** Lamp property: Which color it shines in.
*/
public func GetLampColor() { return FIRE_LIGHT_COLOR; }

/** Overload as needed to define a custom offset by the lamp.
*/
public func LampOffset() {}

/*-- Convenient calls --*/

/** Standard turning on procedure. Overload as needed.
	Default behaviour: lib_lamp_lit to true, light range 80, 60, inherited
*/
public func TurnOn()
{
	if (lib_lamp_lit) return false;
	_inherited(...);
	TurnLightOn();
	lib_lamp_lit = true;
	return true;
}

func TurnLightOn()
{
	SetLightRange(this->GetLampRange(), this->GetLampFadeRange());
	SetLightColor(this->GetLampColor());
}

/** Standard turning off procedure. Overload as needed.
	Default behaviour: lib_lamp_lit to false, light range 0, 0, inherited
*/
public func TurnOff()
{
	if (!lib_lamp_lit) return false;
	_inherited(...);
	TurnLightOff();
	lib_lamp_lit = false;
	return true;
}

func TurnLightOff()
{
	SetLightRange(0, 0);
}

// Returns whether the lamp currently is a source of light.
public func IsLightSource()
{
	return lib_lamp_lit;
}

/*-- Usage --*/

/** Standard control procedure. Overload as needed.
	Default behaviour: If clonk is ready, either turn on or off.
*/
public func ControlUse(object clonk)
{
	// Only do something if the clonk can do an action.
	if (!clonk->HasHandAction())
		return true;
	// Turn on or off
	if (lib_lamp_lit) {
		TurnOff();
	} else {
		TurnOn();
	}
	Sound("UI::Click2");
	return true;
}

/*-- Further stuff --*/

/** Calls LampDeparture() in the departed object.
*/
public func Departure(object container)
{
	container->~LampDeparture(this);
}

/** Calls LampDestruction() in a container if contained.
*/
public func Destruction()
{
	if (Contained()) Contained()->~LampDestruction(this);
}

/** Scenario saving: Store whether lamp is on
*/
public func SaveScenarioObject(props, ...)
{
	if (!_inherited(props, ...)) return false;
	if (lib_lamp_lit) props->AddCall("Lamp", this, "TurnOn");
	return true;
}
