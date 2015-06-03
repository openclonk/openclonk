/**
	Lorry
	Transportation and storage for up to 50 objects.

	@authors Maikel	
*/

#include Library_ElevatorControl

local content_menu;
local drive_anim;
local tremble_anim;
local wheel_sound;
local rot_wheels;
local tremble;

protected func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation", Trans_Rotate(13, 0, 1, 0));
}

protected func Initialize()
{
	drive_anim = PlayAnimation("Drive", 5, Anim_Const(0), Anim_Const(500) /* ignored anyway */);
	tremble_anim = PlayAnimation("Tremble", 5, Anim_Const(0), Anim_Const(500));

	rot_wheels = 0;
	tremble = 0;
	AddTimer("TurnWheels", 1);
}

public func IsLorry() { return true; }
public func IsVehicle() { return true; }
public func IsContainer() { return true; }
public func IsToolProduct() { return true; }

protected func Hit3()
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
	// Objects can still be collected.
	if (ContentsCount() < this->MaxContentsCount())
	{
		Sound("Clonk");
		return false;
	}
	
	// The object cannot be collected, notify carrier?
	if (obj->Contained())
		Message("$TxtLorryisfull$");
	else
	{
		// If not carried, objects slide over the lorry.
		if (Abs(obj->GetXDir()) > 5)
		{
			obj->SetYDir(-2);
			obj->SetRDir(0);
			Sound("SoftHit*");
		}
	}
	// Reject collection.
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

public func TurnWheels()
{
	// TODO: Use Anim_X(Dir), keep from timer=1
	// TODO: Could also use GetAnimationPosition() instead of these local variables...
	rot_wheels += GetXDir() * 20;
	while (rot_wheels < 0) 
		rot_wheels += 2000;
	while (rot_wheels > 2000) 
		rot_wheels -= 2000;
	SetAnimationPosition(drive_anim, Anim_Const(rot_wheels));
	if (Random(100) < Abs(GetXDir()))
	{
		tremble += 100;
		if (tremble < 0) tremble += 2000;
		if (tremble > 2000) tremble -= 2000;
		SetAnimationPosition(tremble_anim, Anim_Const(tremble));
	}
	if (Abs(GetXDir()) > 1 && !wheel_sound)
	{
		if (!wheel_sound) 
			Sound("WheelsTurn", false, nil, nil, 1);
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
				obj->SetController(by_player);	
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


/*-- Properties --*/

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

public func Definition(proplist def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(-25, 1, 0, 0), Trans_Rotate(40, 0, 1, 0)), def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local Rebuy = true;
