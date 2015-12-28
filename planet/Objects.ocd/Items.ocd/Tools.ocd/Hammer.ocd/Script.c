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


/*-- Properties --*/

func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Rotate(20, 1, 0, 1), def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
