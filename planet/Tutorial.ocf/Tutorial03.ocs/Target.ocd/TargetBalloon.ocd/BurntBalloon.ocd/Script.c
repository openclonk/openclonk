/*-- Burnt Target Balloon --*/

protected func Initialize()
{
	SetRDir(-2+Random(4));
	SetAction("Fall");
	SetComDir(COMD_None);
	AddEffect("Fade",this,1,1,this);
}

func FxFadeTimer(object target, effect, int time)
{
	if(GetYDir()<10) SetYDir(GetYDir()+1);
	SetClrModulation(RGBa(255-(time/2),255-(time/2),-(time/2),255-time));
	if(time>=255)
	{
		RemoveObject();
		return -1;
	}
	return 1;
}

func Definition(def) {
	SetProperty("Name", "Burnt Balloon", def);
	SetProperty("ActMap", {

Fall = {
	Prototype = Action,
	Name = "Fall",
	Procedure = DFA_FLOAT,
	Directions = 1,
	FlipDir = 0,
	Length = 1,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 64,
	Hgt = 64,
	NextAction = "Fall",
},
}, def);}
