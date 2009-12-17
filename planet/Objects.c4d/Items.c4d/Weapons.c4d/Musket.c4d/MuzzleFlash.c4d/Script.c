/*-- Muzzle Flash --*/

#strict 2

protected func Initialize()
{
	SetAction("Flash");
}

func Definition(def) {
  SetProperty("ActMap", {
Flash = {
Prototype = Action,
Name = "Flash",
Procedure = DFA_FLIGHT,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 1,
X = 0,
Y = 0,
Wdt = 16,
Hgt = 16,
FacetBase = 0,
EndCall="RemoveObject",
},  }, def);
  SetProperty("Name", "Muzzle Flash", def);
}
