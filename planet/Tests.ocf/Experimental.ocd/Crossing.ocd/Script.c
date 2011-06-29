/*--
	Cable Crossing
	
	Author: Randrian, Clonkonaut
--*/

#include Library_CableStation

local rotation = 0;

protected func Initialize()
{
	  SetAction("Wait");
	  SetGraphics(0, GetID(), 1, GFXOV_MODE_Base);
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