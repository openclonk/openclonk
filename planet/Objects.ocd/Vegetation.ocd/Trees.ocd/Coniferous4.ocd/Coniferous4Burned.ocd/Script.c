/*-- Yet Another Additional Burned Coniferous Tree --*/

#include Library_Plant
#include Library_Tree

public func IsBurnedTree()
{
	return true;
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Components = {Wood = 3};