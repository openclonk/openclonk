/*-- Lorry --*/

#include Library_ItemContainer

local content_menu;

protected func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(13,0,1,0));
}

public func IsLorry() { return 1; }

public func IsToolProduct() { return 1; }

local drive_anim;
local tremble_anim;

protected func Initialize()
{
	drive_anim = PlayAnimation("Drive", 5, Anim_Const(0), Anim_Const(500) /* ignored anyway */);
	tremble_anim = PlayAnimation("Tremble", 5, Anim_Const(0), Anim_Const(500));

	iRotWheels = 0;
	iTremble = 0;
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

private func MaxContentsCount()
{
	return 50;
}

private func MenuOnInteraction() { return false; }
private func MenuOnControlUse() { return true; }
private func MenuOnControlUseAlt() { return false; }

protected func RejectCollect(id object_id, object obj)
{
	if (ContentsCount() < MaxContentsCount())
	{
		Sound("Clonk");
		return false;
	}
	if (obj->Contained())
		return Message("$TxtLorryisfull$");
	return _inherited(...);
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
		SetAnimationPosition(tremble_anim, Anim_Const(iTremble));
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
};

func Definition(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25,1,0,0),Trans_Rotate(40,0,1,0)),def);
}

local Name = "$Name$";
local Description = "$Description$";
