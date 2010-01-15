/*-- Muzzle Flash --*/

#strict 2

protected func Initialize()
{
	//Having trouble with EndCall right now. Current work-around.
	Schedule("RemoveObject()", 8);
}

public func Remove() { return RemoveObject(); }

func Definition(def) {
  SetProperty("ActMap", {
Flash = {
Prototype = Action,
Name = "Flash",
Procedure = DFA_ATTACH,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 1,
X = 0,
Y = 0,
Wdt = 16,
Hgt = 16,
FacetBase = 0,
EndCall="Remove",
},  }, def);
  SetProperty("Name", "Muzzle Flash", def);
}
