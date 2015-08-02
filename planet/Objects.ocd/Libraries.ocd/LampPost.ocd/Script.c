/**
	Lamp Post Library
	Include this by all structures that may have a lamp (light source) hanging outside.
	Lamps must include the Lamp Library.
	Dummies must include the Lamp Dummy Library.
	
	@author Clonkonaut
*/

/** Whenever a lamp enters the building, a dummy lamp is created to visualise the effect.
*/
local lib_lamp_dummy;

local lib_lamp_lamp;

/** Return a 2-value array with the (offset) position of the dummy lamp.
*/
public func LampPosition(id def) { return [0,0]; }

/** This is a lamp post
*/
public func LampPost() { return true; }

private func RejectCollect(id item, object obj)
{
	// At least one lamp can be collected
	if (obj->~IsLamp())
		if (!lib_lamp_dummy)
		{
			CreateLampDummy(obj);
			return false;
		}
	return _inherited(item, obj, ...);
}

private func CreateLampDummy(object lamp)
{
	var def = lamp->DummyDefinition();
	var lamp_offset = lamp->~LampOffset();
	if (!lamp_offset) lamp_offset = [0,0];
	// In theory, LampPosition can distinguish between certain types of lamps
	var my_offset = LampPosition(def);
	my_offset[0] += lamp_offset[0];
	my_offset[1] += lamp_offset[1];

	lib_lamp_dummy = CreateObject(def,0,0, GetOwner());
	lib_lamp_dummy->Set(this, my_offset, lamp);
	lib_lamp_lamp = lamp;
}

/** Called when a lamp leaves the building.
*/
public func LampDeparture(object lamp)
{
	if (lamp != lib_lamp_lamp || !lib_lamp_dummy) return;
	lib_lamp_dummy->RemoveObject();
	lib_lamp_lamp = nil;
	lib_lamp_dummy = nil;
}

/** Called when a lamp inside the building is destroyed. Defaults to LampDeparture().
*/
public func LampDestruction(object lamp)
{
	LampDeparture(lamp);
}