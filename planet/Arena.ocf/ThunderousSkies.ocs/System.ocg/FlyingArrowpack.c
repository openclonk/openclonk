/* Everyone wins! */

#appendto Arrow


func Fall(int from) 
{ 
	Sound("Objects::Weapons::Bow::Shoot?");
	for (var i = 0; i < 10; i++ ) 
	{
			var arrow = TakeObject();
			arrow->Launch(Random(200)+80, Random(20)+10, GetCrew(from));
	}
	if (this) RemoveObject();
}
func HasNoFadeOut()
{
	if (GetAction() == "Attach") return true;
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
	Wdt = 4,
	Hgt = 15,
	NextAction = "Attach",
//	Animation = "idle",
	}
}	,def);
}