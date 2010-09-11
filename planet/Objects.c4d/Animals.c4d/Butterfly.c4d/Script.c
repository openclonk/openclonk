/*
	Butterfly
	Author: Randrian

	A small fluttering being.
*/

protected func Initialize()
{
	// TODO: Fix mesh extents?
	SetObjDrawTransform(20000,0,0,0,20000,-160000);

	SetAction("Fly");
	MoveToTarget();
	return 1;
}
	
/* TimerCall */

private func Activity()
{
	// Underwater
	if (InLiquid()) return SetComDir(COMD_Up);
	// Sitting? wait
	if (GetAction() == "Sit") return 1;
	// New target
	if (!GetCommand() || !Random(5)) MoveToTarget();
	return 1;
}

/* Movement */

private func Flying()
{
	// Change direction
	if (GetXDir() > 0) SetDir(DIR_Right);
	else SetDir(DIR_Left);
	// Change action
	if (!Random(3)) SetAction("Flutter");
	return 1;
}
	
private func Fluttering()
{
	// Change direction
	if (GetXDir() > 0) SetDir(DIR_Right);
	else SetDir(DIR_Left);
	// Change action
	if (!Random(7)) SetAction("Fly");
	return 1;
}

/* Contact */
	
protected func ContactBottom()
{
	SetCommand("None");
	SetComDir(COMD_Up);
	return 1;
}

protected func SitDown()
{
	SetXDir(0);
	SetYDir(0);
	SetComDir(COMD_Stop);
	SetAction("Sit");
	SetCommand("None");
	return 1;
}

protected func TakeOff()
{
	SetComDir(COMD_Up);
	return 1;
}

private func MoveToTarget()
{
	var x = Random(LandscapeWidth());
	var y = Random(GetHorizonHeight(x)-60)+30;
	SetCommand("MoveTo",0,x,y);
	return 1;
}

private func GetHorizonHeight(int x)
{
	var height;
	while ( height < LandscapeHeight() && !GBackSemiSolid(x,height))
		height += 10;
	return height;
}

local ActMap = {

Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	Directions = 2,
	FlipDir = 1,
	Length = 1,
	Delay = 10,
	X = 0,
	Y = 0,
	Wdt = 24,
	Hgt = 24,
	NextAction = "Fly",
	StartCall = "Flying",
	Animation = "Fly",
},
Flutter = {
	Prototype = Action,
	Name = "Flutter",
	Procedure = DFA_FLOAT,
	Directions = 2,
	FlipDir = 1,
	Length = 11,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 24,
	Hgt = 24,
	NextAction = "Flutter",
	StartCall = "Fluttering",
	Animation = "Wait",
},
};
local Name = "Butterfly";

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(20,1,0,0),Trans_Rotate(70,0,1,0)), def);
}
