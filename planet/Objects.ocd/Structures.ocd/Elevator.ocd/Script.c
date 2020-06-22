/*-- Elevator --*/

#include Library_Structure
#include Library_Ownable

// used in the elevator case
static const Elevator_needed_power = 20;

local case, rope;
local partner, slave;

/* Editing helpers */

// Frees a rectangle for the case
public func CreateShaft(int length)
{
	// Move the case out of the way
	case->SetPosition(case->GetX(), GetY()-10);
	ClearFreeRect(GetX() + 7 - 38*GetDir(), GetY() + 20, 24, length + 13);
	// Move the case back
	case->SetPosition(case->GetX(), GetY()+20);
}

// Move case to specified absolute y position
public func SetCasePosition(int y)
{
	if (case) return case->SetPosition(case->GetX(), y);
	return false;
}

// Overloaded to reposition the case
public func SetDir(new_dir, ...)
{
	var r = inherited(new_dir, ...);
	// Update position of child objects on direction change
	if (case) case->SetPosition(GetX() -19 * GetCalcDir(), case->GetY());
	if (rope) rope->SetPosition(GetX() -19 * GetCalcDir(), rope->GetY());
	
	// Set mesh transformation so that the rope on the mesh fits the rope from the elevator case.
	if (new_dir == DIR_Left)
	{
		this.MeshTransformation = Trans_Rotate(-44, 0, 1, 0);
	}
	else
	{
		this.MeshTransformation = Trans_Rotate(-47, 0, 1, 0);
	}
	return r;
}

// Forward config to case
public func SetNoPowerNeed(bool to_val)
{
	if (case) return case->SetNoPowerNeed(to_val);
	return false;
}

private func EditCursorMoved()
{
	// Move case and rope along with elevator in editor mode
	if (case) case->SetPosition(GetX() + GetCaseXOff(), case->GetY());
	if (rope) rope->SetPosition(GetX() + GetCaseXOff(), GetY() - 13);
	return true;
}

// return default horizontal offset of case/rope to elevator
public func GetCaseXOff() { return -19 * GetCalcDir(); }

/* Initialization */

private func Construction()
{
	// Set default mesh transformation.
	SetDir(DIR_Left);
	SetAction("Default");
	wheel_anim = PlayAnimation("winchSpin", 1, Anim_Const(0), Anim_Const(1000));

	return _inherited(...);
}

public func IsHammerBuildable() { return true; }

private func Initialize()
{
	SetCategory(C4D_StaticBack);
	CreateCase();
	CreateRope();

	if (partner)
	{
		if (Inside(partner->GetY(), GetY()-3, GetY()+3))
		{
			partner->LetsBecomeFriends(this);
			SetPosition(GetX(), partner->GetY());
		}
		else
			partner = nil;
	}
	return _inherited(...);
}

private func CreateCase()
{
	case = CreateObjectAbove(ElevatorCase, GetCaseXOff(), 33, GetOwner());
	if (case) case->Connect(this);
}

public func GetCase()
{
	return case;
}

private func CreateRope()
{
	rope = CreateObjectAbove(ElevatorRope, GetCaseXOff(), -11, GetOwner());
	if (rope) rope->SetAction("Be", case.back);
}

/* Destruction */

private func Destruction()
{
	if (rope) rope->RemoveObject();
	if (case) case->LostElevator();
	if (partner) partner->LoseCombination();
}

public func LostCase()
{
	if (partner) partner->LoseCombination();
	if (rope) rope->RemoveObject();

	StopEngine();

	// for now: the elevator dies, too
	Incinerate();
}

/* Effects */

local wheel_anim, case_speed;

public func StartEngine(int direction, bool silent)
{
	if (!case) return;

	if (!silent)
	{
		Sound("Structures::Elevator::Start", {custom_falloff_distance = 400});
		ScheduleCall(this, "EngineLoop", 34);
	}
	if (wheel_anim == nil) // If for some reason the animation has stopped
		wheel_anim = PlayAnimation("winchSpin", 1, Anim_Const(0), Anim_Const(1000));

	var begin, end;
	if (direction == COMD_Up) // Either that or COMD_Down
	{
		begin = GetAnimationLength("winchSpin");
		end = 0;
	}
	else
	{
		begin = 0;
		end = GetAnimationLength("winchSpin");
	}

	case_speed = Abs(case->GetYDir());
	var speed = 80 - case_speed * 2;
	SetAnimationPosition(wheel_anim, Anim_Linear(GetAnimationPosition(wheel_anim), begin, end, speed, ANIM_Loop));
}

public func EngineLoop()
{
	Sound("Structures::Elevator::Moving", {loop_count = 1, custom_falloff_distance = 400});
}

public func StopEngine(bool silent)
{
	if (!silent)
	{
		Sound("Structures::Elevator::Moving", {loop_count = -1});
		ClearScheduleCall(this, "EngineLoop");
		Sound("Structures::Elevator::Stop", {custom_falloff_distance = 400});
	}

	if (wheel_anim == nil) return;

	case_speed = nil;
	SetAnimationPosition(wheel_anim, Anim_Const(GetAnimationPosition(wheel_anim)));
}

// Adjusting the turning speed of the wheel to the case's speed
private func UpdateTurnSpeed()
{
	if (!case) return;
	if (case_speed == nil || wheel_anim == nil) return;

	if (Abs(case->GetYDir()) != case_speed)
	{
		var begin, end;
		if (case->GetYDir() < 0) // Either that or COMD_Down
		{
			begin = GetAnimationLength("winchSpin");
			end = 0;
		}
		else
		{
			begin = 0;
			end = GetAnimationLength("winchSpin");
		}
		case_speed = Abs(case->GetYDir());
		var speed = 80 - case_speed * 2;
		SetAnimationPosition(wheel_anim, Anim_Linear(GetAnimationPosition(wheel_anim), begin, end, speed, ANIM_Loop));
	}
}

/* Construction preview */

// Definition call by the construction previewer
public func ConstructionPreview(object previewer, int overlay, int dir)
{
	if (GetType(this) != C4V_Def) return;

	previewer->SetGraphics(nil, Elevator_Case_Front, overlay, GFXOV_MODE_Base);
	previewer->SetObjDrawTransform(1000, 0, -19000 * (dir*2-1), 0, 1000, 17000, overlay);
	return true;
}

// Sticking to other elevators
public func ConstructionCombineWith() { return "CanCombineElevator"; }
public func ConstructionCombineDirection(object other)
{
	if (!other) return CONSTRUCTION_STICK_Left | CONSTRUCTION_STICK_Right;

	// Only combine when facing correctly
	if (other->GetDir() == DIR_Left)
		return CONSTRUCTION_STICK_Right;
	return CONSTRUCTION_STICK_Left;
}

// Called to determine if sticking is possible
public func CanCombineElevator(object previewer)
{
	if (!previewer) return true;

	if (GetDir() == DIR_Left)
	{
		if (previewer.direction == DIR_Right && previewer->GetX() > GetX()) return true;
	}
	else
	{
		if (previewer.direction == DIR_Left && previewer->GetX() < GetX()) return true;
	}
	return false;
}

// Called when the elevator construction site is created
public func CombineWith(object other)
{
	// Save for use in Initialize
	partner = other;
}

// Special requirements for the basement of the elevator.
public func GetBasementWidth() { return 36; }
public func GetBasementOffset() { return [11 * (2 * GetDir() - 1), 0]; }


/* Combination */

// Called by a new elevator next to this one
// The other elevator will be the slave
public func LetsBecomeFriends(object other)
{
	partner = other;
	other.slave = true; // Note: This is liberal slavery
	if (case) case->StartConnection(other.case);
}

// Partner was destroyed or moved
public func LoseCombination()
{
	partner = nil;
	slave = false;
	if (case) case->LoseConnection();
}

// Called by our case because the case has a timer anyway
public func CheckSlavery()
{
	// Check if somehow we moved away from our fellow
	if (ObjectDistance(partner) > 62 || !Inside(partner->GetY(), GetY()-1, GetY()+1))
	{
		LoseCombination();
		partner->LoseCombination();
	}
}

/* Scenario saving */

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Category");
	if (partner && slave)
	{
		props->AddCall("Friends", partner, "LetsBecomeFriends", this);
	}
	if (case && case->GetY() > GetY() + 20)
	{
		props->AddCall("Shaft", this, "CreateShaft", case->GetY() - GetY() - 20);
		props->AddCall("Shaft", this, "SetCasePosition", case->GetY());
	}
	return true;
}

local ActMap = {
		Default = {
			Prototype = Action,
			Name = "Default",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			Length = 1,
			Delay = 3,
			FacetBase = 1,
			NextAction = "Default",
			EndCall = "UpdateTurnSpeed",
		},
};

private func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(-20, 1, 0), Trans_Rotate(-20, 0, 1, 0)));
	return _inherited(def, ...);
}
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 100;
local HitPoints = 70;
local Plane = 249;
local Components = {Wood = 3, Metal = 1};
