/*-- Power crystal --*/

#include Library_Flag

local DefaultFlagRadius = 250;

protected func Initialize()
{
	SetCategory(C4D_StaticBack);
	SetR(Random(360));
	SetObjectBlitMode(GFX_BLIT_Additive);
	SetClrModulation(RGB(127 + Random(3)*64, 127 + Random(3)*64, 127 + Random(3)*64));
	return _inherited(...);
}

local Name = "$Name$";
local Description = "$Description$";

global func CheckConstructionSite(structure_id, x, y)
{
	// Construction check: May not construct in power crystal range
	if (!inherited(structure_id, x, y, ...)) return false;
	if (FindObject(Find_ID(PowerCrystals), Find_Distance(PowerCrystals.DefaultFlagRadius, x, y))) return false;
	return true;
}
