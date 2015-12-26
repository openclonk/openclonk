/**
	Ropebridge Post
	
	@author Randrian
*/

local double;

public func Initialize()
{
	// Only create a double if there is not already post at this location.
	if (FindObject(Find_ID(GetID()), Find_Exclude(this), Find_AtPoint()))
		return;
	if (!double)
	{
		double = CreateObject(GetID());
		double.Plane = 600;
		double->SetAction("Attach", this);
		double->SetGraphics("Foreground", GetID());
	}
	return;
}

public func Turn(int dir)
{
	var turn_dir = 1;
	if (dir == DIR_Right)
		turn_dir = -1;
	SetObjDrawTransform(1000 * turn_dir, 0, 0, 0, 1000);
	if (double)
		double->SetObjDrawTransform(1000 * turn_dir, 0, 0, 0, 1000);
	return;
}


/*-- Properties --*/

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		FacetBase = 1,
	},
};
local Name = "$Name$";
local Description = "$Description$";
	
