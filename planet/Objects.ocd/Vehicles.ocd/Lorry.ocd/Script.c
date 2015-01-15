/*-- Lorry --*/

#include Library_ElevatorControl

local content_menu;

protected func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(13,0,1,0));
}

public func IsLorry() { return true; }
public func IsVehicle() { return true; }
public func IsContainer() { return true; }
public func IsToolProduct() { return true; }

local drive_anim;
local tremble_anim;
local wheel_sound;

protected func Initialize()
{
	drive_anim = PlayAnimation("Drive", 5, Anim_Const(0), Anim_Const(500) /* ignored anyway */);
	tremble_anim = PlayAnimation("Tremble", 5, Anim_Const(0), Anim_Const(500));

	iRotWheels = 0;
	iTremble = 0;
	AddTimer("TurnWheels", 1);
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

func Hit3()
{
	Sound("DullMetalHit?");
}

/*-- Contents --*/

private func MaxContentsCount()
{
	return 50;
}

protected func RejectCollect(id object_id, object obj)
{
	// objects can still be collected
	if (ContentsCount() < this->MaxContentsCount())
	{
		Sound("Clonk");
		return false;
	}
	
	// the object cannot be collected, notify carrier?
	if (obj->Contained())
		Message("$TxtLorryisfull$");
	else
	{
		// if not carried, objects slide over the lorry
		if(Abs(obj->GetXDir()) > 5)
		{
			obj->SetYDir(-2);
			obj->SetRDir(0);
			Sound("SoftHit*");
		}
	}
	// reject collection
	return true;
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
	if (Abs(GetXDir()) > 1 && !wheel_sound)
	{
		if (!wheel_sound) Sound("WheelsTurn", false, nil, nil, 1);
		wheel_sound = true;
	}
	else if (wheel_sound && !GetXDir())
	{
		Sound("WheelsTurn", false, nil, nil, -1);
		wheel_sound = false;
	}
}

protected func Damage(int change, int cause, int by_player)
{
	// Only explode the lorry on blast damage.
	if (cause != FX_Call_DmgBlast)
		return _inherited(change, cause, by_player, ...);
	// Explode the lorry when it has taken to much damage.
	if (GetDamage() > 100)
	{
		// Only exit objects and parts if this lorry is not contained.
		if (!Contained())
		{
			// First eject the contents in different directions.
			for (obj in FindObjects(Find_Container(this)))
			{
				var speed = RandomX(3, 5);
				var angle = Random(360);
				var dx = Cos(angle, speed);
				var dy = Sin(angle, speed);
				obj->Exit(RandomX(-4, 4), RandomX(-4, 4), Random(360), dx, dy, RandomX(-20, 20));		
			}
	
			// Toss around some fragments with particles attached.
			for (var i = 0; i < 6; i++)
			{
				var fragment = CreateObject(LorryFragment, RandomX(-4, 4), RandomX(-4, 4), GetOwner());
				var speed = RandomX(40, 60);
				var angle = Random(360);
				var dx = Cos(angle, speed);
				var dy = Sin(angle, speed);
				fragment->SetXDir(dx, 10);
				fragment->SetYDir(dy, 10);
				fragment->SetR(360);
				fragment->SetRDir(RandomX(-20, 20));
				// Set the controller of the fragments to the one causing the blast for kill tracing.
				fragment->SetController(by_player);
				// Incinerate the fragments.
				fragment->Incinerate();
			}		
		}
		// Remove the lorry itself, eject possible contents as they might have entered again.
		// Or let the engine eject the contents if it is inside a container.
		return RemoveObject(true);	
	}
	return _inherited(change, cause, by_player, ...);
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
local Touchable = 1;
local Rebuy = true;
