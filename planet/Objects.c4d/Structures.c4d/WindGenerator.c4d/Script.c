/*-- Windmill --*/

#strict 2

#include PWRG

public func GetCapacity() { return 500; }
public func GetGeneratorPriority() { return 256; }

/* Initialisierung */

protected func Initialize()
{
  SetAction("Turn");
  return _inherited(...);
}

func Wind2Turn()
{
  DoPower(Abs(GetWind()/4));
  if(Abs(GetWind()) < 10) this["ActMap"]["Turn"]["Delay"] = 0;
  else this["ActMap"]["Turn"]["Delay"] = BoundBy(5-Abs(GetWind())/10, 1, 5);
}

func Definition(def) {
  SetProperty("ActMap", {
Turn = {
Prototype = Action,
Name = "Turn",
Procedure = DFA_NONE,
Length = 40,
Delay = 1,
X = 0,
Y = 0,
Wdt = 70,
Hgt = 90,
NextAction = "Turn",
//Animation = "Turn",
},  }, def);
  SetProperty("Name", "$Name$", def);
}