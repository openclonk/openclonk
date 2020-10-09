/*
	Hatch

	@authors: Clonkonaut, Ringwaul (Graphics)
*/

#include Library_Structure

local opened = false;

/*-- Engine Callbacks --*/

func Construction()
{
	SetCategory(C4D_StaticBack);

	return _inherited(...);
}

func Initialize()
{
	// If the hatch has not been properly combined with a basement, we might want to correct that
	if (!GetBasement())
	{
		var basements = FindObjects(Find_AtRect(-1, 0, 2, 4), Find_Func("IsBasement"), Sort_Distance());
		for (var basement in basements)
		{
			if (!basement->GetParent() && basement->GetWidth() == 40)
			{
				SetPosition(basement->GetX(), basement->GetY()-2);
				basement->SetParent(this);
				break;
			}
		}
	}
	return _inherited(...);
}

/*-- Callbacks --*/

public func IsHammerBuildable() { return true; }

public func DoConstructionEffects(object construction_site)
{
	// The hatch sets itself a bit lower after it has been placed with the construction previewer
	SetPosition(GetX(), GetY()+4);
	return false;
}

public func NoConstructionFlip() { return true; }

public func ConstructionCombineWith() { return "IsBasement"; }

public func ConstructionCombineDirection(object other) { return CONSTRUCTION_STICK_Top; }

public func CombineWith(object stick_to)
{
	if (stick_to && stick_to->~IsBasement())
		stick_to->SetParent(this);

	SetPosition(GetX(), GetY()+1);
}

/*-- Library overloads --*/

func MoveOutOfSolidMask()
{
	// Find all objects inside the solid mask which are stuck.
	var lying_around = FindObjects(Find_Or(Find_Category(C4D_Vehicle), Find_Category(C4D_Object), Find_Category(C4D_Living)), Find_InRect(-13, -2, 26, 4), Find_NoContainer());
	// Move up these objects.
	for (var obj in lying_around)
	{
		var x = obj->GetX();
		var y = obj->GetY();
		var delta_y = 0;
		var max_delta = obj->GetObjHeight();
		// Move up object until it is not stuck any more.
		while (obj->Stuck() || obj->GetContact(-1, CNAT_Bottom))
		{
			// Only move up the object by at most its height plus the basements height.
			if (delta_y > max_delta)
			{
				obj->SetPosition(x, y);
				break;
			}
			delta_y++;
			obj->SetPosition(x, y - delta_y);
		}
	}
}

/*-- Basement --*/

public func GetBasementWidth()
{
	return 40;
}

public func SetBasement(object to_basement)
{
	_inherited(to_basement);

	if (!to_basement) return;

	to_basement->ChangeDef(HatchBasement);
}

public func GetBasementOffset() { return [0, -15]; }

/*-- Interaction --*/

public func IsInteractable(object clonk)
{
	if (GetCon() < 100)
		return false;
	if (opened && FindClonkInHatchArea())
		return false;

	if (Hostile(clonk->GetOwner(), GetOwner()))
		return false;

	return true;
}

public func GetInteractionMetaInfo(object clonk)
{
	if (!ActIdle())
		return { Description = "$Close$", IconName = nil, IconID = Icon_Exit, Selected = false };
	return { Description = "$Open$", IconName = nil, IconID = Icon_Enter, Selected = false };
}

public func Interact(object clonk)
{
	if (ActIdle())
		return DoOpen();
	if (GetAction() == "Open")
		return DoClose();
	return false;
}

/*-- Open/Close --*/

public func DoOpen()
{
	SetAction("Open");
	Sound("Structures::DoorOpen?");
	return true;
}

public func DoClose()
{
	if (opened)
	{
		if (FindClonkInHatchArea())
			return false;
		SetAction("Close");
		Sound("Structures::DoorClose?");
		opened = false;
		return true;
	}
	if (GetAction() == "Open")
	{
		var phase = 2 - GetPhase();
		SetAction("Close");
		Sound("Structures::DoorClose?");
		SetPhase(phase);
		return true;
	}
	SetAction("Close");
	Sound("Structures::DoorClose?");
	return true;
}

func FindClonkInHatchArea()
{
	return FindObject(Find_InRect(-16, -12, 32, 24), Find_OCF(OCF_CrewMember), Find_OCF(OCF_Alive), Find_AnyLayer(), Find_NoContainer());
}

func ChangeSolidMask()
{
	if (opened)
		return;
	if (GetAction() == "Close") // Upon closing, the solid mask must reappear instantaneously
	                            // to prevent clonks from getting stuck
		return SetSolidMask(0, 0, 26, 4, 0, 11);

	var phase = GetPhase();
	var width = 26 - 8*phase;

	if (phase == 3) {
		ClosingTimer();
		width -= 2;
	}

	SetSolidMask(0, 0, width, 4, 0, 11);
}

func ClosingTimer()
{
	opened = true;

	ScheduleCall(this, "CheckForClose", 15);
}

func CheckForClose()
{
	if (GetAction() == "Close" || ActIdle())
		return;

	if (!FindClonkInHatchArea())
		return ScheduleCall(this, "CloseForReal", 25);

	ScheduleCall(this, "CheckForClose", 5);
}

func CloseForReal()
{
	// One final check that the hatch is clear
	if (FindClonkInHatchArea())
		return ScheduleCall(this, "CheckForClose", 5);

	DoClose();
}

/*-- Display --*/

func Definition(def)
{
	def.MeshTransformation = Trans_Mul(Trans_Translate(0,-10000), Trans_Rotate(-10, 0, 1, 0), Trans_Rotate(-8, 1, 0, 0));
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,-3000,-5000), Trans_Rotate(-30, 1, 0, 0), Trans_Rotate(30, 0, 1, 0), Trans_Translate(1000, 1, 0)), def);
	return _inherited(def);
}

/*-- Properties --*/

local ActMap = {
	Open = {
		Prototype = Action,
		Name = "Open",
		Length = 3,
		Delay = 5,
		Animation = "Open",
		NextAction = "Hold",
		PhaseCall = "ChangeSolidMask"
	},
	Close = {
		Prototype = Action,
		Name = "Close",
		Length = 3,
		Delay = 5,
		Animation = "Close",
		NextAction = "Idle",
		PhaseCall = "ChangeSolidMask"
	},
};

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 100;
local HitPoints = 70;
local Components = {Wood = 2};
