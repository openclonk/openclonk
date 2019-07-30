/*-- Yet Another Burned Deciduous Tree --*/

#include Library_Plant
#include Library_Tree

public func IsBurnedTree()
{
	return true;
}

public func Incineration() {
	SetClrModulation(RGB(100, 100, 100));
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;