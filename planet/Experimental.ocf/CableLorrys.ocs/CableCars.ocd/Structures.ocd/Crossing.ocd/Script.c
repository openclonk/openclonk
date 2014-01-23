/*--
	Cable Crossing
	
	Author: Randrian, Clonkonaut
--*/

#include Library_CableStation

local rotation = 0;

protected func Initialize()
{
	  SetAction("Wait");
	  SetGraphics(nil, GetID(), 1, GFXOV_MODE_Base);
	  return _inherited(...);
}

protected func TurnWheel()
{
	  rotation -= 4;
	  var fsin = Sin(rotation, 1000), fcos=Cos(rotation, 1000);
	  var xoff = 0;
	  var yoff = 0;
	  // set matrix values
	  SetObjDrawTransform (
	    +fcos, +fsin, (1000-fcos)*xoff - fsin*yoff,
	    -fsin, +fcos, (1000-fcos)*yoff + fsin*xoff, 1
	  );
}

// Check whether I am a railway station
// If so, set up new graphics
// If not, disable graphics if needed
private func CheckRailStation()
{
	if (GetLength(FindObjects(Find_Func("IsConnectedTo", this))) == 1 || bManualSetting)
	{
		if (!is_station)
		{
			SetGraphics("Station", Library_CableStation, 2, GFXOV_MODE_Base);
			is_station = true;
		}
	}
	else if (is_station)
	{
		SetGraphics(nil, nil, 2, GFXOV_MODE_Base);
		is_station = false;
	}
}

public func IsInteractable()
{
	return true;
}

local ActMap = {
	Active = {
		Prototype = Action,
		Name = "Active",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 1,
		FacetBase = 0,
		NextAction = "Active",
		StartCall = "TurnWheel",
	},
	Wait = {
		Prototype = Action,
		Name = "Wait",
		Procedure = DFA_NONE,
		Length = 1,
		Delay = 0,
		FacetBase = 0,
		NextAction = "Wait",
	},
};

local Name = "$Name$";
local BlastIncinerate = 50;