/*-- Lorry --*/


local content_menu;

protected func Construction()
{
	//PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold));
	SetProperty("MeshTransformation",Trans_Rotate(13,0,1,0));
}

public func IsLorry() { return true; }

public func IsToolProduct() { return true; }

local drive_anim;
local tremble_anim;

local tilt_anim;

protected func Initialize()
{
	drive_anim = PlayAnimation("Drive", 1, Anim_Const(0), Anim_Const(500));
	//tremble_anim = PlayAnimation("Tremble", 5, Anim_Const(0), Anim_Const(500));

	tilt_anim = PlayAnimation("Empty", 1, Anim_Const(0), Anim_Const(100));

	iRotWheels = 0;
	AddTimer("TurnWheels", 1);
	iTremble = 0;
	return _inherited(...);
}

private func Disengaged()
{
	SetAction("Drive");
}

/*-- Movement --*/

protected func ContactLeft()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

protected func ContactRight()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

/*-- Contents --*/

local MaxContentsCount = 50;

protected func RejectCollect(id object_id, object obj)
{
	if (ContentsCount() < MaxContentsCount)
	{
		Sound("Objects::Clonk");
		return false;
	}
	if (obj->Contained())
		return Message("$TxtLorryisfull$");
	return _inherited(object_id, obj, ...);
}

// Automatic unloading in buildings.
protected func Entrance(object container)
{
	// Only in buildings
	if (container->GetCategory() & (C4D_StaticBack | C4D_Structure))
		// Not if the building prohibits this action.
		if (!container->~NoLorryEjection(this))
			// Empty lorry.
			container->GrabContents(this);
}

local iRotWheels;
local iTremble;

func TurnWheels()
{
	// TODO: Use Anim_X(Dir), keep from timer=1
	// TODO: Could also use GetAnimationPosition() instead of these local variables...
	iRotWheels += GetXDir()*2000/100; // Einmal rum (20 frames mal 10fps) nach 10 m
	while(iRotWheels < 0) iRotWheels += 2000;
	while(iRotWheels > 2000) iRotWheels -= 2000;
	SetAnimationPosition(drive_anim, Anim_Const(iRotWheels));
	if(Random(100) < Abs(GetXDir()))
	{
		iTremble += 100;
		if(iTremble < 0) iTremble += 2000;
		if(iTremble > 2000) iTremble -= 2000;
		//SetAnimationPosition(tremble_anim, Anim_Const(iTremble));
	}
}

local ActMap = {
		Drive = {
			Prototype = Action,
			Name = "Drive",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			Length = 20,
			Delay = 2,
			X = 0,
			Y = 0,
			Wdt = 22,
			Hgt = 16,
			NextAction = "Drive",
			//Animation = "Drive",
		},
		OnRail = {
			Prototype = Action,
			Name = "OnRail",
			Procedure = DFA_FLOAT,
			Directions = 2,
			FlipDir = 1,
			Length = 1,
			Delay = 1,
			X = 0,
			Y = 0,
			Wdt = 22,
			Hgt = 16,
			StartCall="OnRail",
			NextAction = "OnRail",
			//Animation = "Drive",
		},
};

func Definition(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25,1,0,0),Trans_Rotate(40,0,1,0)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;