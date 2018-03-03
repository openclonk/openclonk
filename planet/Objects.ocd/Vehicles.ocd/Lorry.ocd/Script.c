/**
	Lorry
	Transportation and storage for up to 50 objects.

	@authors Maikel	
*/

#include Library_ElevatorControl
#include Library_Destructible

local drive_anim;
local tremble_anim;
local wheel_sound;
local rot_wheels;
local tremble;

protected func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold));
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
	Sound("Hits::Materials::Metal::DullMetalHit?");
}


/*-- Content Dumping --*/

public func HoldingEnabled() { return true; }

public func ControlUseStart(object clonk, int x, int y)
{
	var direction = DIR_Left;
	if (x > 0)
		direction = DIR_Right;
	if (!GetEffect("DumpContents", this))
		AddEffect("DumpContents", this, 100, 1, this, nil, direction);
	return true;
}

public func ControlUseHolding(object clonk, int x, int y)
{
	var direction = DIR_Left;
	if (x > 0)
		direction = DIR_Right;
	var effect = GetEffect("DumpContents", this);
	if (effect)
		effect.dump_dir = direction;
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	RemoveEffect("DumpContents", this);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	RemoveEffect("DumpContents", this);
	return true;
}

public func FxDumpContentsStart(object target, proplist effect, int temp, int direction)
{
	if (temp)
		return FX_OK;
	// The time it takes to dump the contents depends on the mass of the lorry.
	effect.dump_strength = BoundBy(1000 / GetMass(), 3, 8);
	effect.dump_dir = direction;
	// Rotate the lorry into the requested direction.
	var rdir = -effect.dump_strength;
	if (effect.dump_dir == DIR_Right)
		rdir = effect.dump_strength;
	SetRDir(rdir);
	// Start dump sounds together.
	Sound("Objects::Lorry::Dump1", false, 100, nil, +1);
	Sound("Objects::Lorry::Dump2", false, 100, nil, +1);
	return FX_OK;
}

public func FxDumpContentsTimer(object target, proplist effect, int time)
{
	// Rotate the lorry into the requested direction.
	var rdir = -effect.dump_strength;
	if (effect.dump_dir == DIR_Right)
		rdir = effect.dump_strength;	
	SetRDir(rdir);
	// Dump one item every some frames if the angle is above 45 degrees. Only do this if the effect is at least active 
	// for 10 frames to prevent an accidental click while holding the lorry to dump some of its contents.
	if (time >= 10 && ((effect.dump_dir == DIR_Left && GetR() < -45) || (effect.dump_dir == DIR_Right && GetR() > 45)))
	{
		if (!Random(3))
		{
			var x = RandomX(6, 8) * Sign(GetR());
			var xdir = RandomX(70, 90) * Sign(GetR());
			var random_content = FindObjects(Find_Container(this), Sort_Random());
			if (GetLength(random_content) >= 1)
			{
				random_content = random_content[0];
				random_content->Exit(x, RandomX(2, 3), Random(360), 0, 0, RandomX(-5, 5));
				random_content->SetXDir(xdir, 100);
				// Assume the controller of the lorry is also the one dumping the contents.
				random_content->SetController(GetController());
				AddEffect("BlockCollectionByLorry", random_content, 100, 8, this);
				if (random_content->~IsLiquid())
				{
					random_content->SetPosition(GetX(), GetY());
					random_content->Disperse(GetR() + 10 * Sign(GetR()));
				}
			}		
		}
	}
	return FX_OK;
}

public func FxDumpContentsStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	// Stop rotating the lorry.
	SetRDir(0);
	// Stop dump sounds.
	Sound("Objects::Lorry::Dump1", false, 100, nil, -1);
	Sound("Objects::Lorry::Dump2", false, 100, nil, -1);
	return FX_OK;
}

public func FxBlockCollectionByLorryTimer() { return FX_Execute_Kill; }


/*-- Contents --*/

local MaxContentsCount = 50;

protected func RejectCollect(id object_id, object obj)
{
	// Collection maybe blocked if this object was just dumped.
	if (!obj->Contained() && GetEffect("BlockCollectionByLorry", obj))
		return true;
	
	// Objects can still be collected.
	if (ContentsCount() < MaxContentsCount)
	{
		Sound("Objects::Clonk");
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
			Sound("Hits::SoftHit*");
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
		// Only if the building requests this action.
		if (container->~LorryEjectionOnEntrance(this))
		{
			// Empty lorry.
			container->GrabContents(this);
		}
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
			Sound("Structures::WheelsTurn", {loop_count = 1});
		wheel_sound = true;
	}
	else if (wheel_sound && !GetXDir())
	{
		Sound("Structures::WheelsTurn", {loop_count = -1});
		wheel_sound = false;
	}
}

// Custom fragments on callback from destructible library.
public func OnDestruction(int change, int cause, int by_player)
{
	// Only exit objects and parts if this lorry is not contained.
	if (!Contained())
	{
		// First eject the contents in different directions.
		for (var obj in FindObjects(Find_Container(this)))
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
			fragment->Incinerate(100, by_player);
		}		
	}
	// Remove the lorry itself, eject possible contents as they might have entered again.
	// Or let the engine eject the contents if it is inside a container.
	RemoveObject(true);
	return true;
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
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;
local Components = {Metal = 2, Wood = 1};
local HitPoints = 100;