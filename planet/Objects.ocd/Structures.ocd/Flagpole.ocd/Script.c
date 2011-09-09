/*-- Flagpole --*/

#include Library_Flag

local Name = "$Name$";
local Description = "$Description$";
 
protected func Initialize()
{
	SetCategory(C4D_StaticBack);
	return _inherited(...);
}

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Translate(0,5250,0));
	return _inherited(...);
}