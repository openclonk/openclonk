/*-- Elevator case --*/

#include Library_Structure
#include Library_PowerConsumer

// if you change the vertices in the defcore make sure to adjust this
static const ElevatorCase_additional_vertex_index_begin = 4;
static const ElevatorCase_normal_vertex_index_begin = 0;
static const ElevatorCase_additional_vertex_count = 4;

// Meshes
local front, back;

local elevator;
local partner, partner_was_synced, is_master;


/*-- Callbacks --*/

// Elevator speeds.
private func GetCaseSpeed() { return 20; }
private func GetAutoSpeed() { return GetCaseSpeed() * 2; }
private func GetDrillSpeed() { return GetCaseSpeed() / 2; }

// Case is not a structure, but uses the library.
public func IsStructure() { return false; }

protected func Initialize()
{
	AddEffect("CheckAutoMoveTo", this, 1, 30, this);
	AddEffect("ElevatorUpperLimitCheck", this, 1, 1, this);
	AddEffect("FetchVehicles", this, 1, 10, this);
	
	partner_was_synced = false;
	
	front = CreateObject(Elevator_Case_Front);
	back = CreateObjectAbove(Elevator_Case_Back, 0, 13, GetOwner());
	
	front->SetAction("Attach", this);
	back->SetAction("Attach", this);
	return _inherited(...);
}

// Called by the elevator
func Connect(object connect_to)
{
	elevator = connect_to;
	SetComDir(COMD_None);
	SetAction("DriveIdle");
}

func Destruction()
{
	if(partner)
		partner->LoseConnection();
	if(elevator)
		elevator->LostCase();
		
	for(var i = 0; i < 3; ++i)
	{
		var wood = CreateObjectAbove(Wood, 0, 0, NO_OWNER);
		wood->Incinerate();
		wood->SetXDir(RandomX(-10, 10));
		wood->SetYDir(RandomX(-2, 0));
	}
	return _inherited(...);
}

func LostElevator()
{
	// die x.x
	RemoveObject();
}

// Called by the elevator in case a partner elevator was constructed
func StartConnection(object case)
{
	partner = case;
	partner_was_synced = false;
	if(case.partner != this)
	{
		case->StartConnection(this);
		is_master = true;
		AddEffect("TryToSync", this, 1, 1, this);
	}
	else // is slave
	{
		is_master = false;
		MoveTo(nil, 0, case);
	}
}

// Called when the other elevator is destroyed or moved
func LoseConnection()
{
	partner = nil;
	is_master = nil;
	partner_was_synced = false;
	if(GetEffect("TryToSync", this))
		RemoveEffect("TryToSync", this);
	SetPartnerVertices(0, 0);
	SetActionData(0);
	
	Halt();
}

// slave loses attach to master?
func AttachTargetLost()
{
	LoseConnection();
}

public func ExecuteSync()
{
	if (!is_master) 
		FatalError("ExecuteSync() called on slave elevator case!");
	partner_was_synced = true;
	partner.partner_was_synced = true;
	ForceSync();
	
	SetPartnerVertices(partner->GetX() - GetX(), partner->GetY() - GetY());
	
	// Reset power usage.
	ResetPowerUsage();
	partner->ResetPowerUsage();
	
	// can now attach partner on one of the new vertices
	partner->SetAction("Attach", this);
	var vertex_data = (ElevatorCase_normal_vertex_index_begin << 8) + ElevatorCase_additional_vertex_index_begin;
	partner->SetActionData(vertex_data);
	
	Sound("Click");
}

// sets additional vertices to partner's position
func SetPartnerVertices(int off_x, int off_y)
{
	var update_mode = 2; // force immediate update and store information
	
	for(var i = 0; i < ElevatorCase_additional_vertex_count; ++i)
	{
		SetVertex(ElevatorCase_additional_vertex_index_begin + i, VTX_X, GetVertex(ElevatorCase_normal_vertex_index_begin + i, VTX_X) + off_x, update_mode);
		SetVertex(ElevatorCase_additional_vertex_index_begin + i, VTX_Y, GetVertex(ElevatorCase_normal_vertex_index_begin + i, VTX_Y) + off_y, update_mode);
	}
}

func IsMaster() { return partner && is_master && partner_was_synced; }
func IsSlave() { return partner && !is_master && partner_was_synced; }

func FxTryToSyncTimer(object target, effect, int time)
{
	var diff = Abs(partner->GetY() - GetY());
	if(diff > 5) return 1;
	ExecuteSync();
	return -1;
}

func FxCheckAutoMoveToTimer(object target, effect, int time)
{
	if(!elevator) return -1;
	if(IsSlave()) return 1;
	if(!CheckIdle()) return 1;
	if(GetEffect("MoveTo", this)) return 1;
	
	// look for Clonks at shaft
	var additional = 20;
	var x = GetX() - additional;
	var w = GetX() + additional;
	var y = elevator->GetY();
	var h = LandscapeHeight();
	
	if(IsMaster())
	{
		x = Min(x, partner->GetX() - additional);
		w = Max(w, partner->GetX() + additional);
	}
	var clonk, best;

	for (clonk in FindObjects(Find_InRect(x - GetX(), y - GetY(), w - x, h - y), Find_OCF(OCF_CrewMember), Find_OCF(OCF_Alive), Find_NoContainer(), Find_Allied(GetOwner()), Sort_Distance(), Sort_Reverse()))
	{
		var proc = clonk.Action.Procedure;
		if (clonk->GetComDir() != COMD_Stop && !((proc == "SWIM") && Inside(clonk->GetXDir(), -5, 5)))
			continue;
		if (proc != "WALK" && proc != "PUSH" && proc != "SCALE" && proc != "HANGLE" && proc != "SWIM") continue;
		if (clonk->GetY() < GetY() - 7)
			if (!PathFree(GetX(), GetY(), GetX(), clonk->GetY()))
				continue;
		if (clonk->GetY() > GetY() + 7)
			if (!PathFree(GetX(), GetY() + 16, GetX(), clonk->GetY()))
				continue;
		if ((clonk->GetY() > GetY()) && GetContact(-1, CNAT_Bottom)) 
			continue;
		
		// Do not move to very close Clonks.
		if (Abs(GetY() - clonk->GetY()) < 5) 
			continue;
		
		// Priority rules: Cursor is better than no cursor, nearer is better than farer (Sort_Distance() & Sort_Reverse() do this)
		// So unlike in CR's elevator, no distance check has to be done because later cycles are always nearer clonks
		if (!best) 
			best = clonk;
		else if (GetCursor(clonk->GetController()) == clonk)
			best = clonk;
		else if (GetCursor(best->GetController()) != best)
			best = clonk;
	}
	if (best)
		MoveTo(best->GetY(), 10, best);
	return 1;
}

func FxElevatorUpperLimitCheckTimer(target, effect, time)
{
	if (!elevator || IsSlave()) 
		return -1;
		
	var d = GetY() - (elevator->GetY() + 20);
	
	// HOW COULD THIS HAPPEN :C
	if (d <= 0)
	{
		if (GetYDir() < 0)
		{
			SetPosition(GetX(), GetY() - d);
			ForceSync();
			ContactTop();
		}
		else if (GetYDir() == 0)
			SetPosition(GetX(), GetY() - d);
		
		effect.Interval = 1;
		return 1;
	}
	
	// everything okay, adjust timer accordingly
	// check less often if far away from elevator
	// note: d > 0
	var t = BoundBy(d / 3, 1, 20);
	effect.Interval = t;
	return 1;
}

// for vehicle control
func OutOfRange(object vehicle)
{
	if(Abs(GetY() - vehicle->GetY()) > 10) return true;
	
	var min_x = GetX() - 12;
	var max_x = GetX() + 12;
	if(IsMaster())
	{
		min_x = Min(min_x, partner->GetX() - 12);
		max_x = Max(max_x, partner->GetX() + 12);
	}
	
	if(vehicle->GetX() < min_x) return true;
	if(vehicle->GetX() > max_x) return true;
	return false;
}

protected func FxFetchVehiclesTimer(object target, proplist effect, int time)
{
	if (!elevator) 
		return -1;
	if (IsSlave()) 
		return 1;
	
	// look for vehicles
	var additional = -5;
	var x = GetX() - 12 - additional;
	var w = GetX() + 12 + additional;
	var y = GetY() - 12;
	var h = GetY() + 15;
	
	if (IsMaster())
	{
		x = Min(x, partner->GetX() - 12 - additional);
		w = Max(w, partner->GetX() + 12 + additional);
	}
	
	// Fetch vehicles
	for (var vehicle in FindObjects(Find_InRect(x - GetX(), y - GetY(), w - x, h - y), Find_Category(C4D_Vehicle), Find_NoContainer(), Find_Func("FitsInElevator")))
	{
		if (GetEffect("ElevatorControl", vehicle)) continue;
		vehicle->SetPosition(vehicle->GetX(), GetY() + 12 - vehicle->GetObjHeight()/2);
		vehicle->SetSpeed();
		vehicle->SetR();
		AddEffect("ElevatorControl", vehicle, 30, 5, vehicle, nil, this);
	}
	
	return 1;
}

/*-- Power Consumption --*/

// Keeps track of whether the elevator is currently powered.
local has_power;

private func GetNeededPower()
{
	if (partner_was_synced)
		return 2 * Elevator_needed_power;
	return Elevator_needed_power;
}

// The elevator is the actual power consumer.
public func GetActualPowerConsumer()
{
	return elevator;
}

// Elevator has a high priority.
public func GetConsumerPriority() { return 100; }

// Public wrapper for resetting the usage of the slave elevator.
public func ResetPowerUsage()
{
	UnregisterPowerRequest();
	has_power = false;
	return;
}

public func OnNotEnoughPower()
{
	has_power = false;
	
	if (GetYDir())
		StoreMovementData();
	
	if (GetAction() != "DriveIdle")
		Halt(false, true);
	return _inherited(...);
}

public func OnEnoughPower()
{
	has_power = true;
	
	RestoreMovementData();
	return _inherited(...);
}

protected func FxHasPowerStart()
{
	return 1;
}

private func StoreMovementData(int y_dir, string action, bool user_requested)
{
	if (y_dir == nil)
	{
		if (GetYDir() < 0)
			y_dir = COMD_Up;
		if (GetYDir() > 0)
			y_dir = COMD_Down;
		else
			y_dir = COMD_Stop;
	}
	action = action ?? GetAction();
	user_requested = user_requested ?? !CheckIdle();
	var effect = GetEffect("StoredMovementData", this);
	if (!effect) 
		effect = AddEffect("StoredMovementData", this, 1, 0, this);
	effect.y_dir = y_dir;
	effect.action = action;
	effect.user_requested = user_requested;
	return;
}

private func RestoreMovementData()
{
	var effect = GetEffect("StoredMovementData", this);
	if (!effect) 
		return;
	var drill = false;
	if (effect.action == "Drill")
		drill = true;
	// This function is only called when enough power is available, so then call
	// the movement function with has_power equal to true.
	SetMoveDirection(effect.y_dir, effect.user_requested, drill);
	RemoveEffect(nil, this, effect);
	return;
}

private func SetMoveDirection(int dir, bool user_requested, bool drill)
{
	if (IsSlave()) 
		return partner->SetMoveDirection(dir, user_requested, drill, has_power);
		
	// no change?
	if (dir == COMD_Up && (GetYDir() < 0)) return;
	if (dir == COMD_Down && (GetYDir() > 0)) return;
	
	// already reached top/bottom?
	if (GetContact(-1, CNAT_Bottom) && dir == COMD_Down && !drill)
		return;
	if (GetContact(-1, CNAT_Top) && dir == COMD_Up)
		return;
	if (dir == COMD_Stop) 
		return Halt();
	
	var speed = GetCaseSpeed();
	// Note: can not move down with full speed because of solidmask problem.
	if (!user_requested && dir == COMD_Up) 
		speed = GetAutoSpeed();
	
	var action = "Drive";
	if (drill)
	{
		action = "Drill";
		speed = GetDrillSpeed();
	}
	
	if (has_power)
	{
		if (dir == COMD_Down)
			SetYDir(speed);
		else if (dir == COMD_Up)
			SetYDir(-speed);
		SetAction(action);
		SetComDir(COMD_None);
		ForceSync();
		elevator->StartEngine();
	}
	else
	{
		StoreMovementData(dir, action, user_requested);
		RegisterPowerRequest(GetNeededPower());
	}
	return;
}

private func Halt(bool user_requested, bool power_out)
{
	if (IsSlave())
		return;

	// Stop the engine if it was still moving.	
	if (GetYDir())
		if(elevator)
			elevator->StopEngine();

	// Clear speed.
	SetAction("DriveIdle");
	SetYDir();
	ForceSync();
	
	// Unregister the power request and stop automatic movement.
	if (user_requested || !power_out)
	{
		StopAutomaticMovement();
		UnregisterPowerRequest();
		has_power = false;
	}
	return;
}

public func ForceSync()
{
	if (!IsMaster()) 
		return;
	// Clear rounding errors.
	SetPosition(GetX(), GetY());
	// Adjust partner.
	partner->SetPosition(partner->GetX(), GetY());
	partner->SetYDir(0);
}

protected func ContactTop()
{
	Halt();
	Sound("WoodHit*");
	return;
}

protected func ContactBottom()
{
	// try to dig free
	if (GetAction() == "Drill")
	{
		Drilling();
		
		// wee!
		if (!GetContact(-1, CNAT_Bottom))
		{
			SetYDir(GetDrillSpeed());
			return;
		}
	}
	Halt();
	Sound("WoodHit*");
	return;
}

// Checks whether the elevator should not move because someone's holding it, returns true if idle.
private func CheckIdle()
{
	// I have no mind of my own
	if (IsSlave()) 
		return false;

	var in_rect = Find_InRect(-13, -13, 26, 26);
	if (IsMaster())
	{
		if (partner->GetX() < GetX())
			in_rect = Find_InRect(-39, -13, 52, 26);
		else
			in_rect = Find_InRect(-13, -13, 52, 26);
	}
	for (var pusher in FindObjects(in_rect, Find_Action("Push")))
	{
		if (pusher->GetActionTarget() == this) 
			return false;
		if (GetEffect("ElevatorControl", pusher->GetActionTarget()) && GetEffect("ElevatorControl", pusher->GetActionTarget()).case == this) 
			return false;
		
		if (IsMaster())
		{
			if (pusher->GetActionTarget() == partner) 
				return false;
			if (GetEffect("ElevatorControl", pusher->GetActionTarget()) && GetEffect("ElevatorControl", pusher->GetActionTarget()).case == partner) 
				return false;
		}
	}
	return true;
}

private func StopAutomaticMovement()
{
	if (GetEffect("MoveTo", this))
	{
		RemoveEffect("MoveTo", this);
		Halt();
	}
	return;
}

// Moves the case to the specific y-coordinate
// delay in frames, so the elevator does not freak out
// target will be checked again for COMD_Stop and distance after delay run out
public func MoveTo(int y, int delay, object target, bool user_requested)
{
	// Not idle?
	if (!CheckIdle() && !user_requested) 
		return;
	Halt();
	var effect = AddEffect("MoveTo", this, 1, 2, this);
	effect.delay = delay;
	effect.move_to_y = y;
	effect.target = target;
	effect.user_requested = user_requested;
	return;
}

protected func FxMoveToTimer(object target, proplist effect, int time)
{
	if (time < effect.delay) return 1;
	// what would take more than 10 seconds?
	if ((time - effect.delay) / 36 > 10) return -1;
	
	var y = effect.move_to_y;
	if (effect.target) 
		y = effect.target->GetY();
	
	// Target dead? Don't move and remove effect.
	if (y == nil)
	{
		Halt();
		return -1;
	}
	
	// Target moves away from elevator shaft, finish movement but stop following
	if (effect.target)
		if(Abs(GetX() - effect.target->GetX()) > 100)
		{
			effect.move_to_y = effect.target->GetY();
			effect.target = nil;
		}
	
	// Destination reached? Stop effect and movement.
	if (Abs(GetY() - y) < 5)
	{
		Halt();
		return -1;
	}
	
	var dir = COMD_Up;
	if (y > GetY()) 
		dir = COMD_Down;
	SetMoveDirection(dir, effect.user_requested, false);
	return 1;
}

protected func Drilling()
{
	var additional_y = 1;
	var rect = Rectangle(GetX() - 12, GetY() - 13 - additional_y, GetX() + 12, GetY() + 13 + additional_y);
	if (IsMaster())
	{
		rect.x = Min(rect.x, partner->GetX() - 12);
		rect.y = Min(rect.y, partner->GetY() - 13 - additional_y);
		rect.w = Max(rect.w, partner->GetX() + 12);
		rect.h = Max(rect.h, partner->GetY() + 13 + additional_y);
	}
	DigFreeRect(rect.x, rect.y, rect.w - rect.x, rect.h - rect.y);
	return;
}


/*-- Controls --*/

// Send elevator to the clicked position.
public func ControlUseStart(object clonk, int x, int y)
{
	if (IsSlave()) 
		return Control2Master("ControlUseStart", clonk, x, y);
	MoveTo(GetY() + y, 0, nil, true);
	Sound("Click", nil, nil, clonk->GetOwner());
	// Do not trigger a UseStop-callback.
	return false;
}

public func ControlDown(object clonk)
{
	if (IsSlave()) 
		return Control2Master("ControlDown", clonk);
	
	// Pressing down when already on ground results in drilling.
	var drill = !!GetContact(-1, CNAT_Bottom);
	
	StopAutomaticMovement();
	SetMoveDirection(COMD_Down, true, drill);
	return true;
}

public func ControlUp(object clonk)
{
	if (IsSlave()) 
		return Control2Master("ControlUp", clonk);
	
	// what is that player even doing
	if (GetY() <= elevator->GetY() + 20)
	{
		Sound("Click", nil, nil, clonk->GetOwner());
		return true;
	}
	
	StopAutomaticMovement();
	SetMoveDirection(COMD_Up, true, false);
	return true;
}

public func ControlStop(object clonk, int control)
{
	if (IsSlave()) 
		return Control2Master("ControlStop", clonk, control);
	
	if (control == CON_Up && GetYDir() <= 0)
		Halt(true);
	else if (control == CON_Down && GetYDir() >= 0)
		Halt(true);
	
	return true;
}

public func Control2Master(string call, object clonk)
{
	if (!IsSlave()) 
		return false;
	return partner->Call(call, clonk, ...);
}


/*-- Scenario saving --*/

func SaveScenarioObject() { return false; }


/*-- Properties --*/

local ActMap = {
		Drive = {
			Prototype = Action,
			Name = "Drive",
			Procedure = DFA_FLOAT,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 24,
			Hgt = 26,
			NextAction = "Drive",
		},
		DriveIdle = {
			Prototype = Action,
			Name = "DriveIdle",
			Procedure = DFA_FLOAT,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 24,
			Hgt = 26,
			NextAction = "DriveIdle",
		},
		Drill = {
			Prototype = Action,
			Name = "Drill",
			Procedure = DFA_FLOAT,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 24,
			Hgt = 26,
			Delay = 1,
			Length = 1,
			PhaseCall = "Drilling",
			NextAction = "Drill",
			Sound = "ElevatorDrilling",
			DigFree = 1
		},
		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 24,
			Hgt = 26,
			NextAction = "Attach"
		}
};

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 2;
local HitPoints = 50;
local Plane = 250;
