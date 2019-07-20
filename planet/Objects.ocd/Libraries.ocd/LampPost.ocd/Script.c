/**
	Lamp Post Library
	Include this by all structures that may have a lamp (light source) hanging outside.
	Lamps must include the Lamp Library.
	Dummies must include the Lamp Dummy Library.
	
	@author Clonkonaut
*/

/** Whenever a lamp enters the building, a dummy lamp is created to visualise the effect.
*/
local lib_lamp_overlay;
local lib_lamp_lamp;
local lib_lamp_offset;

/** Return a 2-value array with the (offset) position of the dummy lamp.
*/
public func LampPosition(id def) { return [0, 0]; }

/** This is a lamp post
*/
public func LampPost() { return true; }

private func RejectCollect(id item, object obj)
{
	// At least one lamp can be collected, overriding other restrictions
	if (obj->~IsLamp() && !lib_lamp_lamp) return false;
	return _inherited(item, obj, ...);
}

private func Collection2(object obj)
{
	// Attach lamp on collection
	if (obj->~IsLamp() && !lib_lamp_lamp) CreateLampDummy(obj);
	return _inherited(obj, ...);
}

private func CreateLampDummy(object lamp)
{
	var def = lamp->DummyDefinition();
	var lamp_offset = lamp->~LampOffset();
	if (!lamp_offset) lamp_offset = [0, 0];
	// In theory, LampPosition can distinguish between certain types of lamps
	lib_lamp_offset = LampPosition(def)[:]; // create copy in case LampPosition overloads by returning a variable
	lib_lamp_offset[0] += lamp_offset[0];
	lib_lamp_offset[1] += lamp_offset[1];
	// Attach it as overlay, since this works for all combinations of meshes and bitmap graphics for buildings and lamps
	lib_lamp_overlay = this->GetLampOverlayID(lamp);
	if (!lib_lamp_overlay) return false; // Building rejected this lamp?
	SetGraphics(nil, lamp->GetID(), lib_lamp_overlay, GFXOV_MODE_Object, nil, nil, lamp);
	SetObjDrawTransform(1000, 0, lib_lamp_offset[0] * 1000, 0, 1000, lib_lamp_offset[1] * 1000, lib_lamp_overlay);
	// TODO: Could add an optional mode where lamp is attached as mesh instead (e.g. to attach it to wind generator wings)
	// But not needed for now since no animated structure does this.
	// Update light position of lamp
	if (lib_lamp_offset[0] || lib_lamp_offset[1])
	{
		if (!lamp.LightOffset) lamp.LightOffset = [0, 0];
		lamp.LightOffset[0] += lib_lamp_offset[0];
		lamp.LightOffset[1] += lib_lamp_offset[1];
	}
	lib_lamp_lamp = lamp;
	return true;
}

// Overlay index to be used by lamp - overload if this ID is used by something else in the object
public func GetLampOverlayID() { return 13; }

/** Called when a lamp leaves the building.
*/
public func LampDeparture(object lamp)
{
	if (lamp != lib_lamp_lamp || !lib_lamp_overlay) return;
	SetGraphics(nil, nil, lib_lamp_overlay);
	if (lamp.LightOffset)
	{
		lamp.LightOffset[0] -= lib_lamp_offset[0];
		lamp.LightOffset[1] -= lib_lamp_offset[1];
	}
	lib_lamp_lamp = nil;
	lib_lamp_overlay = nil;
}

/** Called when a lamp inside the building is destroyed. Defaults to LampDeparture().
*/
public func LampDestruction(object lamp)
{
	LampDeparture(lamp);
}