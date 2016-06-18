/*-- Hammer --*/

#include Library_Constructor

private func Hit(int x, int y)
{
	StonyObjectHit(x, y);
	return 1;
}

public func GetCarryMode()	{	return CARRY_HandBack;	}
public func GetCarryBone()	{	return "main";	}
public func GetCarryTransform()	{	return Trans_Rotate(-90,1,0,0);	}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

func CanBuild(id construction_plan)
{
	if (!construction_plan) return false;
	if (construction_plan->~IsHammerBuildable()) return true;
	return false;
}

/*-- Properties --*/

func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Rotate(20, 1, 0, 1), def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Components = {Wood = 1, Rock = 1};