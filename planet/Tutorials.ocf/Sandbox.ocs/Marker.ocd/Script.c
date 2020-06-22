/**
	Marker
	
	@author K-Pone
*/

local Name = "$Name$";
local Description = "$Description$";

local markerindex;
local respawnmarker = false;

public func Initialize()
{
	this.Visibility = VIS_Owner | VIS_God;
	SetGraphics(nil, Dummy);
}

public func SetIcon(number)
{
	markerindex = number;
	SetGraphics(Format("%d", number), Icon_Number, 1, GFXOV_MODE_Base);
	SetObjDrawTransform(350, 0, 2000, 0, 350, 2000, 1);
	SetClrModulation(RGBa(255, 255, 255, 160) , 1);
}

public func GetIndex()
{
	return markerindex;
}

public func IsIndex(index)
{
	return index == markerindex;
}

public func IsRespawnMarker() { return respawnmarker; }

global func GetMarkerForIndex(int index, int plr)
{
	return FindObject(Find_ID(Marker), Find_Owner(plr), Find_Func("IsIndex", index));
}

global func GetNextFreeMarkerIndex(int plr)
{
	for (var i = 0; i <= 9; i++)
	{
		if (GetMarkerForIndex(i, plr)) continue;
		return i;
	}
	return nil;
}