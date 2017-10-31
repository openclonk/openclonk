/**
	Hammer
	Basic construction tool.
*/

// Usage is handled by this library
#include Library_Constructor

#include Library_Flammable

/*-- Engine Callbacks --*/

func Hit(int x, int y)
{
	StonyObjectHit(x, y);
	return 1;
}

/*-- Usage --*/

// Used by the constructor library
func CanBuild(id construction_plan)
{
	if (!construction_plan) return false;
	if (construction_plan->~IsHammerBuildable()) return true;
	return false;
}

/*-- Production --*/

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle)
{
	if (!idle)
		return CARRY_HandBack;
	else
		return CARRY_Belt;
}

public func GetCarryTransform(object clonk, bool idle)
{
	if (!idle) return Trans_Rotate(-90,1,0,0);
}

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Rotate(20, 1, 0, 1), def);
}

/*-- Properties --*/

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local Components = {Wood = 1, Rock = 1};
local BlastIncinerate = 30;
local MaterialIncinerate = true;
local BurnDownTime = 140;