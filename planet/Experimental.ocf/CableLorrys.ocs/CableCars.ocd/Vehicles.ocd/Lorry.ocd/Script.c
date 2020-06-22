/**
	Lorry
	Transportation and storage for up to 50 objects.

	@authors Maikel
*/

#include Library_ElevatorControl

local drive_anim;
local empty_anim;

/*-- Engine Callbacks --*/

public func Construction()
{
	SetProperty("MeshTransformation", Trans_Rotate(13, 0, 1, 0));
}

public func Initialize()
{
	drive_anim = PlayAnimation("Drive", 1, Anim_X(0, 0, GetAnimationLength("Drive"), 30), Anim_Const(1000));
	empty_anim = PlayAnimation("Empty", 2, Anim_Const(0), Anim_Const(0));
}

public func Hit3()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
}

public func RejectCollect(id object_id, object obj)
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
public func Entrance(object container)
{
	// Only in buildings
	if (container->GetCategory() & (C4D_StaticBack | C4D_Structure))
		// Not if the building prohibits this action.
		if (!container->~NoLorryEjection(this))
			// Empty lorry.
			container->GrabContents(this);
}

public func ContactLeft()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

public func ContactRight()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

public func Damage(int change, int cause, int by_player)
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
		return RemoveObject(true);
	}
	return _inherited(change, cause, by_player, ...);
}

/*-- Callbacks --*/

public func IsLorry() { return true; }
public func IsVehicle() { return true; }
public func IsContainer() { return true; }

// Called upon arrival at a cable station
public func DropContents(proplist station)
{
	// Drop everything in a few seconds
	SetAnimationWeight(empty_anim, Anim_Const(1000));
	SetAnimationPosition(empty_anim, Anim_Linear(0, 0, GetAnimationLength("Empty"), 35, ANIM_Hold));

	ScheduleCall(this, "Empty", 35);
}

public func Empty()
{
	// Exit everything at once (as opposed to manual dumping below)
	while (Contents())
	{
		var content = Contents();
		AddEffect("BlockCollectionByLorry", content, 100, 16, this);
		content->Exit(RandomX(-5, 5), RandomX(-2, 2), Random(360));
	}
	SetAnimationWeight(empty_anim, Anim_Linear(1000, 1000, 0, 35, ANIM_Hold));
	SetAnimationPosition(empty_anim, Anim_Linear(GetAnimationLength("Empty"), GetAnimationLength("Empty"), 0, 35, ANIM_Hold));
}

/*-- Usage --*/

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

/*-- Production --*/

public func IsToolProduct() { return true; }

/*-- Actions --*/

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

/*-- Display --*/

public func Definition(proplist def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(-25, 1, 0, 0), Trans_Rotate(40, 0, 1, 0)), def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;
local Components = {Metal = 2, Wood = 1};
local MaxContentsCount = 50;