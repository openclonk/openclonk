/*
	Hatch
	Author: Ringwaul
*/

local graphic;

protected func Initialize()
{
	SetAction("Hatch");
	graphic = CreateObject(Hatch_Graphic);
	graphic->SetAction("Attach", this);
	graphic->SetHatchParent(this);
	return 1;
}

func ControlUp(object clonk)
{
	if(GetPhase() != 0)
	{
		graphic->Anim("Close");
		SetPhase(0);
		SetSolidMask(0,0,26,26,0,0);
	}
}

func ControlDown(object clonk)
{
	if(GetPhase() != 1)
	{
		graphic->Anim("Open");
		SetPhase(1);
		SetSolidMask(26,0,26,26,0,0);
	}
}

protected func Definition(def)
{
		SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,-3000,-5000), Trans_Rotate(-30,1,0,0), Trans_Rotate(30,0,1,0), Trans_Translate(1000,1,0)),def);
}

local Name = "$Name$";
local Touchable = 2;
local ActMap = {
	Hatch = {
	Prototype = Action,
	Name = "Hatch",
	Procedure = DFA_NONE,
	Directions = 1,
	FlipDir = 0,
	Length = 2,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 1,
	Hgt = 1,
},
};