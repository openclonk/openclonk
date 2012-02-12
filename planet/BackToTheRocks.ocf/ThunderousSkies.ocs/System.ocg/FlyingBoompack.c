/* Everyone wins! */

#appendto Boompack


func Fall(int from) 
{
 	SetOwner(from); 
 	Launch(RandomX(-10,10)+180);
}

func HasNoFadeOut()
{
	if(GetAction() == "Attach") return true;
	return false;
}

func Definition(def) {
SetProperty("ActMap", {

Attach = {
	Prototype = Action,
	Name = "Attach",
	Procedure = DFA_ATTACH,
	Directions = 1,
	FlipDir = 0,
	Length = 00,
	X = 0,
	Y = 0,
	Wdt = 7,
	Hgt = 8,
	NextAction = "Attach",
//	Animation = "idle",
	}
}	,def);
}