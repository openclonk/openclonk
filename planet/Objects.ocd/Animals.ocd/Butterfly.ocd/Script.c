/*
	Butterfly
	Author: Randrian/Ringwaul

	A small fluttering being.
*/

func Construction()
{
	StartGrowth(15);
}

local flightanim;
local rotanim;

protected func Initialize()
{
	flightanim = PlayAnimation("Fly", 5, Anim_Linear(0,0, GetAnimationLength("Fly"), 10, ANIM_Loop), Anim_Const(1000));
	rotanim = PlayAnimation("Rotate", 5, Anim_Const(1), Anim_Const(1000));
	AddEffect("ButterflyTurn", this, 1, 1, this);
	SetAction("Fly");
	SetComDir(COMD_None);
	MoveToTarget();
	AddTimer("Activity");
	
	// Make butterflies a bit more colorful.
	SetClrModulation(HSL(Random(256), 255, 100 + Random(60)));
	
	return 1;
}

func FxButterflyTurnTimer(object target, int num, int timer)
{
	TurnButterfly();
}
	
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

func TurnButterfly()
{
	var angle = Normalize(Angle(0,0,GetXDir(), GetYDir()));
	SetAnimationPosition(rotanim, Anim_Const(angle * 10));
	return;
}

private func FlyingStart()
{
	TurnButterfly();
	if (!Random(3))
	{
		SetAction("Flutter");
		SetComDir(COMD_None);
	}
	return 1;
}
	
private func Fluttering()
{
	if (!Random(7))
	{
		SetAction("Fly");
		SetComDir(COMD_None);
	}
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
	SetCommand("MoveTo",nil,x,y);
	return 1;
}

private func GetHorizonHeight(int x)
{
	var height;
	while ( height < LandscapeHeight() && !GBackSemiSolid(x,height))
		height += 10;
	return height;
}

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("ClrModulation"); // randomized in Initialize
	SaveScenarioObjectAction(props);
	return true;
}

local ActMap = {

Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	Speed = 200,
	Accel = 16,
	Decel = 16,
	Directions = 2,
	FlipDir = 1,
	Length = 1,
	Delay = 10,
	X = 0,
	Y = 0,
	Wdt = 24,
	Hgt = 24,
	NextAction = "Fly",
	StartCall = "FlyingStart",
},
Flutter = {
	Prototype = Action,
	Name = "Flutter",
	Procedure = DFA_FLOAT,
	Speed = 200,
	Accel = 16,
	Decel = 16,
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
local MaxEnergy = 40000;
local MaxBreath = 125;
local Placement = 2;
local NoBurnDecay = 1;

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(20,1,0,0),Trans_Rotate(70,0,1,0)), def);
}

