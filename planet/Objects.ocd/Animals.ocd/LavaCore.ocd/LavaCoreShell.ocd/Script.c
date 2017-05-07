/**
	LavaCore Shell
	Protects the lava core and provides a solidmask for the clonk to walk on.

	@author Win, Maikel
*/


local size;

public func InitAttach(object parent)
{
	SetAction("Attach", parent);
	SetSize(BoundBy(parent->GetCon(), parent.SizeLimitMin, parent.SizeLimitMax));
	return;
}

public func SetSize(int size)
{
	// Rotate core to solidmask.
	var r = -70;
	var fsin = Sin(r, 10 * size), fcos = Cos(r, 10 * size);
	SetObjDrawTransform(+fcos, +fsin, 0, -fsin, +fcos, 0);
	// Update solid mask.
	var solid_size = 2 * ((size * 20 / 100 + 2) / 2) + 4;
	solid_size = BoundBy(solid_size, 4, 28);
	var solid_x = (1 + solid_size / 2) * (solid_size - 4);
	SetSolidMask(solid_x, 0, solid_size * 2, solid_size * 2, 28 - solid_size, 28 - solid_size);
	return;
}


/*-- Saving --*/

public func SaveScenarioObject() { return false; }


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 425;

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 0,
		FacetBase = 1,
		NextAction = "Attach",
	}
};