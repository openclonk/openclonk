
/*

	Marker
	
	@author: K-Pone

*/

local Name = "$Name$";
local Description = "$Description$";

local markerindex;
local respawnmarker = false;

func Initialize()
{
	this.Visibility = VIS_Owner | VIS_God;
	SetGraphics(nil, Dummy);
}

func SetIcon(number)
{
	markerindex = number;
	SetGraphics(Format("%d", number), Icon_Number, 1, GFXOV_MODE_Base);
	SetObjDrawTransform(350, 0, 2000, 0, 350, 2000, 1);
	SetClrModulation(RGBa(255, 255, 255, 160) , 1);
}

func GetIndex()
{
	return markerindex;
}

func IsIndex(index)
{
	return index == markerindex;
}

func IsRespawnMarker() { return respawnmarker; }

global func GetMarkerForIndex(index, plr)
{
	return FindObject(Find_ID(Marker), Find_Owner(plr), Find_Func("IsIndex", index));
}

global func GetNextFreeMarkerIndex(plr)
{
	for (var i = 0; i <= 9; i++)
	{
		if (GetMarkerForIndex(i, plr)) continue;
		return i;
	}
	return nil;
}