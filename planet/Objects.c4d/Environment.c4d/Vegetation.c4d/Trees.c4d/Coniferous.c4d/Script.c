/*-- Tree --*/

protected func Initialize()
{
	SetAction("Rotate");
	SetPhase(RandomX(1,90));
	return 1;
}

func Definition(def) {
	SetProperty("ActMap", {
Rotate = {
	Prototype = Action,
	Name = "Rotate",
	Procedure = DFA_NONE,
	Length = 90,
	Delay = 0,
	NextAction = "Hold",
	Animation = "Rotate",
},
}, def);
//	SetProperty("Name", "$Name$", def);
}