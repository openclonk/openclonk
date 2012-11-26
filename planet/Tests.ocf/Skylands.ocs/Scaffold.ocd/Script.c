/*-- Scaffold --*/

#include Library_Structure

/* Initialization */

func Initialize()
{
	return _inherited(...);
}

/* Interaction */

local wall_left, wall_right, wall_top, wall_bottom;

func ControlUp()
{
	if (wall_top)
		wall_top->RemoveObject();
	else
		(wall_top = CreateObject(ScaffoldWall,0,0,GetOwner()))->SetTop(this);
	Sound("DullWoodHit1");
	return true;
}

func ControlLeft()
{
	if (wall_left)
		wall_left->RemoveObject();
	else
		(wall_left = CreateObject(ScaffoldWall,0,0,GetOwner()))->SetLeft(this);
	Sound("DullWoodHit1");
	return true;
}

func ControlRight()
{
	if (wall_right)
		wall_right->RemoveObject();
	else
		(wall_right = CreateObject(ScaffoldWall,0,0,GetOwner()))->SetRight(this);
	Sound("DullWoodHit1");
	return true;
}

func ControlDown()
{
	if (wall_bottom)
		wall_bottom->RemoveObject();
	else
		(wall_bottom = CreateObject(ScaffoldWall,0,0,GetOwner()))->SetBottom(this);
	Sound("DullWoodHit1");
	return true;
}

/* Destruction */

local ActMap = {
		Default = {
			Prototype = Action,
			Name = "Default",
			Procedure = DFA_NONE,
			Directions = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 1,
			NextAction = "Default",
		},
};

func Definition(def) {
	
}

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 100;
local HitPoints = 70;
local Plane = 120;
local Touchable = 1;
