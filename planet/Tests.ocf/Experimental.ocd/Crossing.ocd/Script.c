/*--
	Cable Crossing
	
	Author: Randrian, Clonkonaut
--*/

#include Library_CableStation

local rotation = 0;

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

local Name = "$Name$";