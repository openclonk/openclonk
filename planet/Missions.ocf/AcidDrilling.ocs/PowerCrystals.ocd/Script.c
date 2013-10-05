/*-- Power crystal --*/

#include Library_Flag

protected func Initialize()
{
	SetCategory(C4D_StaticBack);
	SetR(Random(360));
	SetObjectBlitMode(GFX_BLIT_Additive);
	SetClrModulation(RGB(127+Random(3)*64,127+Random(3)*64,127+Random(3)*64));
	return _inherited(...);
}

local Name = "$Name$";
local Description = "$Description$";
