/* Everyone wins! */

#appendto DynamiteBox
func Initialize()
{
	SetObjDrawTransform(2000, 0, 4000, 0, 2000);
}

func HasNoFadeOut()
{
	if (GetAction() == "Attach") return true;
	return false;
}

func Fall(int from) 
{
	for (var i = 0; i < 7 + Random(2); i++) 
	{
		var dyn = CreateObjectAbove(Dynamite, 0, 0, from);
		dyn->SetController(from);
		dyn->SetXDir(RandomX(-9, 9));
		dyn->SetYDir(RandomX(-5, 8));
		dyn->SetRDir(RandomX(-15, 15));
		dyn->Fuse();
		dyn->MakeFast(70+(i*4));
	}
	RemoveObject();
}

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Scale(), def); // Hide it TODO: Remove if the mesh isn't shown if there is a picture set
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
