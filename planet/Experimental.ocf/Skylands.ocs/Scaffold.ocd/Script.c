/*-- Scaffold --*/

#include Library_Structure

/* Initialization */

func Initialize()
{
	return _inherited(...);
}

func Destruction()
{
	RemoveWalls();
	return _inherited(...);
}

func Incineration()
{
	RemoveWalls();
	return _inherited(...);
}

func RemoveWalls()
{
	if (wall_left)    wall_left->RemoveObject();
	if (wall_right)   wall_right->RemoveObject();
	if (wall_top)     wall_top->RemoveObject();
	if (wall_bottom)  wall_bottom->RemoveObject();
	if (wall_right2)  wall_right2->RemoveObject();
	if (wall_bottom2) wall_bottom2->RemoveObject();
	return true;
}

/* Interaction */

local wall_left, wall_right, wall_top, wall_bottom;

local wall_right2, wall_bottom2; // Hack to make Skylands work for now...

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
		if (wall_right2)
		{
			wall_right->RemoveObject();
			wall_right2->RemoveObject();
		}
		else
			(wall_right2 = CreateObject(ScaffoldWall,0,0,GetOwner()))->SetRight2(this);
	else
		(wall_right = CreateObject(ScaffoldWall,0,0,GetOwner()))->SetRight(this);
	Sound("DullWoodHit1");
	return true;
}

func ControlDown()
{
	if (wall_bottom)
		if (wall_bottom2)
		{
			wall_bottom->RemoveObject();
			wall_bottom2->RemoveObject();
		}
		else
			(wall_bottom2 = CreateObject(ScaffoldWall,0,0,GetOwner()))->SetBottom2(this);
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
local HitPoints = 30;
local Plane = 120;
local Touchable = 1;
