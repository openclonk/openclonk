/**
	Lamp Dummy Library
	Include this by all dummy lamps.
	
	@author Clonkonaut
*/

/** The building this dummy is attached to.
*/
local lib_lamp_building;

/** The building's last position.
*/
local lib_lamp_buildpos;

/** The dummy's offset to the building.
*/
local lib_lamp_offset;

/** Attach the dummy to a building.
	@param building The building to attach to.
	@param position An array defining the offet the building: [x, y].
	@param graphics The object whose graphics (mesh) will be used for display.
*/
public func Set(object building, array position, object graphics)
{
	if (!building) return RemoveObject();
	if (!position) position = [0,0];

	if (Contained()) Exit();

	lib_lamp_building = building;
	lib_lamp_buildpos = [building->GetX(), building->GetY()];
	lib_lamp_offset = position;

	SetGraphics(nil,nil,1, GFXOV_MODE_Object, nil,nil, graphics);
	this.Plane = building.Plane + 1;
	SetPosition(lib_lamp_buildpos[0] + lib_lamp_offset[0], lib_lamp_buildpos[1] + lib_lamp_offset[1]);
	AddTimer("LibraryLampDummy_CheckPosition", 1);
}

private func LibraryLampDummy_CheckPosition()
{
	if (lib_lamp_building->GetX() == lib_lamp_buildpos[0])
		if (lib_lamp_building->GetY() == lib_lamp_buildpos[1])
			return;

	lib_lamp_buildpos = [lib_lamp_building->GetX(), lib_lamp_building->GetY()];
	SetPosition(lib_lamp_buildpos[0] + lib_lamp_offset[0], lib_lamp_buildpos[1] + lib_lamp_offset[1]);
}
