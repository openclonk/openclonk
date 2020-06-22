/**
	Trajectory
	Calculates and shows the trajectory of projectiles.	
*/


protected func Initialize()
{
	this.Visibility = VIS_Owner;
}


/*-- Interface --*/

// Removes the trajectory for the given object.
public func Remove(object obj)
{
	if (this != Trajectory)
		return;
	// Find the trajectory for the given object and remove it.
	var trajectory = FindObject(Find_ID(Trajectory), Find_ActionTarget(obj));
	if (trajectory) 
		trajectory->RemoveObject();
	return;
}

// Adds the trajectory for the given object.
// x: start x position in global coordinates
// y: start y position in global coordinates
// xdir: direction in x
// ydir: direction in y
// color: color of the trajectory particles (default white)
// spacing: spacing between the particles (default 10 pixels)
public func Create(object obj, int x, int y, int xdir, int ydir, int color, int spacing)
{
	if (this != Trajectory)
		return;
		
	// Do not create trajectories for script players, this will only cause lag.
	var controller = obj->GetController();
	if (controller == NO_OWNER || GetPlayerType(controller) == C4PT_Script)
		return;
	
	// Delete old trajectory.
	Trajectory->Remove(obj);
	
	// Default values with added precision.
	x *= 10000; y *= 10000;
	xdir *= 100; ydir *= 100;
	if (!color)
		color = RGB(255, 255, 255);
	if (!spacing)
		spacing = 20;
	spacing *= 10000;
	
	// Create new helper object
	var trajectory = CreateObject(Trajectory, obj->GetX(), obj->GetY(), controller);
	trajectory->SetAction("Attach", obj);
	
	// Parrticle setup.
	var particles =
	{
		Prototype = Particles_Trajectory(),
		Size = 4,
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
		Alpha = (color >> 24) & 0xff
	};

	var frame_step = 0;
	var oldx = x, oldy = y;
	while (frame_step < 36 * 100)
	{
		// Update coordinates.
		x += xdir;
		y += ydir;
		ydir += GetGravity();
	
		if (Distance(x, y, oldx, oldy) >= spacing)
		{
			// Correct for the fact that the trajectory object is attached to the shooting parent.
			var parent = trajectory->GetActionTarget();
			var off_x = -parent->GetVertex(0, VTX_X) - trajectory->GetX();
			var off_y = -parent->GetVertex(0, VTX_Y) - trajectory->GetY();
			trajectory->CreateParticle("Magic", x / 10000 + off_x, y / 10000 + off_y, 0, 0, 0, particles);
			oldx = x; oldy = y;
		}
		
		if (GBackSolid(x / 10000 - GetX(), y / 10000 - GetY())) 
			break;
	
		frame_step++;
	}	
	return trajectory;
}

public func AttachTargetLost()
{
	RemoveObject();
}

// Don't save in scenarios.
public func SaveScenarioObject() { return false; }


/*-- Properties --*/

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 0,
	},
};
local Name = "$Name$";
