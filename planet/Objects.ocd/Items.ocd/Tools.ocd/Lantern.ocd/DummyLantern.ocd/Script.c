/**
	Dummy Lantern
	Visual effect: Gets shown outside of buildings when a lantern is inside.

	@author Clonkonaut
*/

local attach_building; // Building I'm hanging on to
local building_pos; // Last position the building was in
local my_offset; // My offset the building's position

// Attach the dummy to a building, position is an array defining the offet the building: [x, y]
// graphics is the object whose graphics (mesh) will be used for display. Preferably a lantern.
public func Set(object building, array position, object graphics)
{
	if (!building) return RemoveObject();
	if (!position) position = [0,0];

	if (Contained()) Exit();

	attach_building = building;
	building_pos = [building->GetX(), building->GetY()];
	my_offset = position;

	SetGraphics(nil,nil,1, GFXOV_MODE_Object, nil,nil, graphics);
	this.Plane = building.Plane + 1;
	SetPosition(building_pos[0] + my_offset[0], building_pos[1] + my_offset[1]);
	AddTimer("CheckPosition", 1);
}

private func CheckPosition()
{
	if (attach_building->GetX() == building_pos[0])
		if (attach_building->GetY() == building_pos[1])
			return;

	building_pos = [attach_building->GetX(), attach_building->GetY()];
	SetPosition(building_pos[0] + my_offset[0], building_pos[1] + my_offset[1]);
}

local Name = "$Name$";
local Description = "$Description$";