/*-- Elevator case --*/

#include Library_Structure
#include Library_PowerConsumer

static const ElevatorCase_move_speed = 20;
static const ElevatorCase_up = -1;
static const ElevatorCase_down = 1;

// if you change the vertices in the defcore make sure to adjust this
static const ElevatorCase_additional_vertex_index_begin = 4;
static const ElevatorCase_normal_vertex_index_begin = 0;
static const ElevatorCase_additional_vertex_count = 4;

// Meshes
local front, back;

local elevator;
local partner, partner_was_synced, is_master;

// can be changed from outside
// standard: ElevatorCase_move_speed, 2 * ElevatorCase_move_speed
local case_speed; // when user controls case
local case_speed_automatic; // when case goes to user

/* Callbacks */

func GetDrillSpeed()
{
	return ElevatorCase_move_speed / 2;
}

func Initialize()
{
	AddEffect("CheckAutoMoveTo", this, 1, 30, this);
	AddEffect("ElevatorUpperLimitCheck", this, 1, 1, this);
	AddEffect("FetchVehicles", this, 1, 10, this);
	
	case_speed = ElevatorCase_move_speed;
	case_speed_automatic = 2 * case_speed;
	
	partner_was_synced = false;
	
	front = CreateObject(Elevator_Case_Front, 0,13, GetOwner());
	back = CreateObject(Elevator_Case_Back, 0,13, GetOwner());
	
	front->SetAction("Attach", this);
	back->SetAction("Attach", this);
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
		var wood = CreateObject(Wood, 0, 0, NO_OWNER);
		wood->Incinerate();
		wood->SetXDir(RandomX(-10, 10));
		wood->SetYDir(RandomX(-2, 0));
	}
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

func ExecuteSync()
{
	if(!is_master) FatalError("ExecuteSync() called on slave elevator case!");
	partner_was_synced = true;
	partner.partner_was_synced = true;
	ForceSync();
	
	SetPartnerVertices(partner->GetX() - GetX(), partner->GetY() - GetY());
	
	// reset power usage
	UnmakePowerConsumer();
	partner->UnmakePowerConsumer();
	
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
		if ((clonk->GetY() > GetY()) && GetContact(-1, CNAT_Bottom)) continue;
		
		// do not move to very close Clonks
		if(Abs(GetY() - clonk->GetY()) < 5) continue;
		
		// Priority rules: Cursor is better than no cursor, nearer is better than farer (Sort_Distance() & Sort_Reverse() do this)
		// So unlike in CR's elevator, no distance check has to be done because later cycles are always nearer clonks
		if (!best) best = clonk;
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
	if(!elevator) return -1;
	if(IsSlave()) return -1;
	
	var d = GetY() - (elevator->GetY() + 20);
	
	// HOW COULD THIS HAPPEN :C
	if(d <= 0)
	{
		if(GetYDir() < 0)
		{
			SetPosition(GetX(), GetY() - d);
			ForceSync();
			ContactTop();
		}
		else
			if(GetYDir() == 0)
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

func FxFetchVehiclesTimer(target, effect, time)
{
	if(!elevator) return -1;
	if(IsSlave()) return 1;
	
	// look for vehicles
	var additional = -5;
	var x = GetX() - 12 - additional;
	var w = GetX() + 12 + additional;
	var y = GetY() - 12;
	var h = GetY() + 15;
	
	if(IsMaster())
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

/* Energy */

func GetNeededPower()
{
	var p = Elevator_needed_power;
	if(partner_was_synced) p = 2 * p;
	return p;
}


// for the position
func GetActualPowerConsumer()
{
	return elevator;
}

// the lift may not need power when not used
func QueryWaivePowerRequest()
{
	// no clonk on elevator? must be automatic
	if(CheckIdle()) return 20;
	return 0;
}

func OnNotEnoughPower()
{
	_inherited(...); // on purpose before the rest
	
	if(GetYDir())
		StoreMovementData();
	else; // already has data stored
	
	if(GetAction() != "DriveIdle")
		Halt(false, true);
}

func OnEnoughPower()
{
	_inherited(...); // on purpose before the rest
	RestoreMovementData();
}

func StoreMovementData(int y_dir, string action, bool user_requested)
{
	y_dir = y_dir ?? GetYDir();
	action = action ?? GetAction();
	user_requested = user_requested ?? !CheckIdle();
	var e = GetEffect("StoredMovementData", this);
	if(!e) e = AddEffect("StoredMovementData", this, 1, 0, this);
	e.y_dir = y_dir;
	e.action = action;
	e.user_requested = user_requested;
}

func RestoreMovementData()
{
	var e = GetEffect("StoredMovementData", this);
	if(!e) return;
	var drill = false;
	if(e.action == "Drill")
		drill = true;
	SetMoveDirection(BoundBy(e.y_dir, -1, 1), e.user_requested, drill);
	RemoveEffect(nil, this, e);
}

func SetMoveDirection(int dir, bool user_requested, bool drill)
{
	if(IsSlave()) 
		return partner->SetMoveSpeed(dir, user_requested, drill);
	
	if(user_requested) StopAutomaticMovement();
	
	var e;
	if(e = GetEffect("StopPowerConsumption", this))
		RemoveEffect(nil, this, e);
	
	// no change?
	if((dir < 0) && (GetYDir() < 0)) return;
	if((dir > 0) && (GetYDir() > 0)) return;
	
	// already reached top/bottom?
	if(GetContact(-1, CNAT_Bottom) && (dir > 0) && !drill)
		return;
	if(GetContact(-1, CNAT_Top) && (dir < 0))
		return;
	if(dir == 0) return Halt();
	
	var speed = case_speed;
	// note: can not move down with full speed because of solidmask problem
	// todo..
	if(!user_requested && dir < 0) speed = case_speed_automatic;
	
	var action = "Drive";
	if(drill)
	{
		action = "Drill";
		speed = GetDrillSpeed();
	}
	
	if(CurrentlyHasPower())
	{
		SetYDir(dir * speed);
		SetAction(action);
		ForceSync();
		
		Sound("ElevatorStart");
		elevator->StartEngine();
	}
	else
	{
		StoreMovementData(dir * speed, action);
		MakePowerConsumer(GetNeededPower());
	}
}

func Halt(bool user_requested, bool power_out)
{
	if(IsSlave()) return;
	
	StopAutomaticMovement();
	
	if(GetYDir())
	{
		if(elevator)
			elevator->StopEngine();
		Sound("ElevatorStop");
	}
	
	// clear speed
	SetAction("DriveIdle");
	SetYDir();
	ForceSync();
	
	if(user_requested)
	{		
		UnmakePowerConsumer();
	}
	else
	{
		// if not stopped because of lack of power, stop consuming power
		if(!power_out)
			AddEffect("StopPowerConsumption", this, 1, 40, this);
	}
}

func FxStopPowerConsumptionTimer(object target, effect, int time)
{
	UnmakePowerConsumer();
	return -1;
}

func ForceSync()
{
	if(!IsMaster()) return;
	// clear rounding errors
	SetPosition(GetX(), GetY());
	// adjust partner
	partner->SetPosition(partner->GetX(), GetY());
	partner->SetYDir(0);
}

func ContactTop()
{
	Halt();
	Sound("WoodHit*");
}

func ContactBottom()
{
	// try to dig free
	if(GetAction() == "Drill")
	{
		Drilling();
		
		// wee!
		if(!GetContact(-1, CNAT_Bottom))
		{
			SetYDir(GetDrillSpeed());
			return;
		}
	}
	Halt();
	Sound("WoodHit*");
}

// Checks whether the elevator should not move because someone's holding it
// Returns true if idle
func CheckIdle()
{
	// I have no mind of my own
	if(IsSlave()) return;

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
		if (pusher->GetActionTarget() == this) return false;
		if (GetEffect("ElevatorControl", pusher->GetActionTarget()) && GetEffect("ElevatorControl", pusher->GetActionTarget()).case == this) return false;
		
		if (IsMaster())
		{
			if (pusher->GetActionTarget() == partner) return false;
			if (GetEffect("ElevatorControl", pusher->GetActionTarget()) && GetEffect("ElevatorControl", pusher->GetActionTarget()).case == partner) return false;
		}
	}
	return true;
}

func StopAutomaticMovement()
{
	var done = false;
	if(GetEffect("MoveTo", this))
	{
		RemoveEffect("MoveTo", this);
		done = true;
	}
	
	// todo: check if sensible
	if(done) Halt();
}

// Moves the case to the specific y-coordinate
// delay in frames, so the elevator does not freak out
// target will be checked again for COMD_Stop and distance after delay run out
func MoveTo(int y, int delay, object target, bool user_requested)
{
	// Not idle?
	if (!CheckIdle() && !user_requested) return false;
	Halt();
	var e = AddEffect("MoveTo", this, 1, 2, this);
	e.delay = delay;
	e.move_to_y = y;
	e.target = target;
	e.user_requested = user_requested;
	return true;
}

func FxMoveToTimer(target, effect, time)
{
	if(time < effect.delay) return 1;
	// what would take more than 10 seconds?
	if((time - effect.delay) / 36 > 10) return -1;
	
	var y = effect.move_to_y;
	if(effect.target) y = effect.target->GetY();
	
	// target dead?
	if(y == nil)
	{
		Halt();
		return -1;
	}
	
	// target moves away from elevator shaft, finish movement but stop following
	if(effect.target)
	if(Abs(GetX() - effect.target->GetX()) > 100)
	{
		effect.move_to_y = effect.target->GetY();
		effect.target = nil;
	}
	
	// destination reached
	if(Abs(GetY() - y) < 5)
	{
		Halt();
		return -1;
	}
	var dir = ElevatorCase_up;
	if(y > GetY()) dir = ElevatorCase_down;
	SetMoveDirection(dir, effect.user_requested, false);
	return 1;
}

func StartDrilling()
{
	SetAction("Drill");
}

func StopDrilling()
{
	SetAction("Drive");
}

func Drilling()
{
	var additional_y = 1;
	var rect = Rectangle(GetX() - 12, GetY() - 13 - additional_y, GetX() + 12, GetY() + 13 + additional_y);
	if(IsMaster())
	{
		rect.x = Min(rect.x, partner->GetX() - 12);
		rect.y = Min(rect.y, partner->GetY() - 13 - additional_y);
		rect.w = Max(rect.w, partner->GetX() + 12);
		rect.h = Max(rect.h, partner->GetY() + 13 + additional_y);
	}
	DigFreeRect(rect.x, rect.y, rect.w - rect.x, rect.h - rect.y);
}

/* Controls */

func ControlUseStart(object clonk, int x, int y) // send elevator to position
{
	if (IsSlave()) return Control2Master("ControlUseStart", clonk, x, y);
	MoveTo(GetY() + y, 60, nil, true);
	Sound("Click", nil, nil, clonk->GetOwner());
	// does not want UseStop-callback
	return false;
}


func ControlDown(object clonk)
{
	if (IsSlave()) return Control2Master("ControlDown", clonk);
	
	// pressing down when already on ground results in drilling
	var drill = !!GetContact(-1, CNAT_Bottom);
	
	SetMoveDirection(ElevatorCase_down, true, drill);
	return true;
}
func ControlUp(object clonk)
{
	if (IsSlave()) return Control2Master("ControlUp", clonk);
	
	// what is that player even doing
	if(GetY() <= elevator->GetY() + 20)
	{
		Sound("Click", nil, nil, clonk->GetOwner());
		return true;
	}
	
	SetMoveDirection(ElevatorCase_up, true, false);
	return true;
}

func ControlStop(object clonk, int control)
{
	if (IsSlave()) return Control2Master("ControlStop", clonk, control);
	
	if(control == CON_Up && GetYDir() <= 0)
		Halt(true);
	else if(control == CON_Down && GetYDir() >= 0)
		Halt(true);
	
	return true;
}

func Control2Master(string call, object clonk)
{
	if (!IsSlave()) return false;
	return partner->Call(call, clonk, ...);
}

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
			Sound = "ElevatorMoving"
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
