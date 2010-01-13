/*-- Windmill --*/

#strict 2

#include PWRG

public func GetCapacity() { return 500; }
public func GetGeneratorPriority() { return 256; }

/* Initialisierung */

protected func Initialize()
{
  AnimationPlay("Turn");
	iRot = 0;
  return _inherited(...);
}

local iRot;

func Wind2Turn()
{
	if(!Random(10))
    DoPower(Abs(GetWind()/4));
	iRot += GetWind()/2;
	if(iRot < 0) iRot += 3600;
	if(iRot >= 3600) iRot -= 3600;
  AnimationSetState("Turn", iRot*12000/3600);
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
