/*-- Elevator case --*/

local elevator;

func Connect(object connect_to)
{
	elevator = connect_to;
	SetComDir(COMD_None);
	SetAction("Drive");
}

/* Movement behaviour */

local move_to, // Y-coordinate to move to on its own
      move_to_delay, // Delay before moving
      move_to_target; // Target to move to

func Movement() // TimerCall
{
	// No elevator?!
	if (!elevator)
	{
		// Elevator crash, oh the horror!
		if (!ActIdle()) SetAction("Idle");
		return;
	}
	// Check for power
	if (!elevator->CheckPower(1, true))
	{
		// Stop movement if moving
		Halt();
		movement = 0;
		ClearMoveTo();
		if (drill)
		{
			SetAction("Drive");
			Sound("ElevatorDrilling", nil, nil, nil, -1);
			drill = false;
		}
		return;
	}

	// Fetch vehicles
	for (var vehicle in FindObjects(Find_InRect(-5, -5, 10, 10), Find_Category(C4D_Vehicle), Find_NoContainer(), Find_Func("FitsInElevator")))
	{
		if (GetEffect("ElevatorControl", vehicle)) continue;

		vehicle->SetPosition(GetX(), GetY() + 12 - vehicle->GetObjHeight()/2 );
		vehicle->SetSpeed();
		vehicle->SetR();
		AddEffect("ElevatorControl", vehicle, 30, 5, nil, nil, this);
	}

	// Start or stop drilling
	if (drill && GetAction() == "Drive")
	{
		SetAction("Drill");
		Sound("ElevatorDrilling", nil, nil, nil, 1);
	}
	if (!drill && GetAction() == "Drill")
	{
		SetAction("Drive");
		Sound("ElevatorDrilling", nil, nil, nil, -1);
	}

	// Stop if at upmost position
	if (GetY() - 20 <= elevator->GetY() && movement < 0)
	{
		if (GetYDir() < 0) Halt();
		movement = 0;
		SetPosition(GetX(), elevator->GetY() + 20);
		ClearMoveTo();
		return;
	}

	// Move or stop
	if (movement && GetYDir() != this.Speed * movement)
	{
		if (movement < 0 && GetContact(-1) & CNAT_Top)
		{
			movement = 0;
		} else if (movement > 0 && GetContact(-1) & CNAT_Bottom)
		{
			movement = 0;
		} else
		{
			if (Abs(GetYDir()) == 1) elevator->StartEngine();
			SetYDir(GetYDir() + movement);
			// Elevator consumes 1 power per frame
			elevator->CheckPower(1);
			return;
		}
	}
	if (!movement && !move_to && GetYDir())
	{
		Halt();
	}

	// Idle?
	if (!CheckIdle()) return;

	// Move-to job?
	if (move_to)
	{
		if (move_to_delay)
		{
			move_to_delay--;
			if (move_to_target)
				if (move_to_target->GetComDir() != COMD_Stop || AbsX(GetX() - move_to_target->GetX()) > 20)
					return ClearMoveTo();
			if (!move_to_delay) move_to_target = nil;
			return;
		}
		if (Inside(move_to, GetY()-2, GetY()+2))
		{
			ClearMoveTo();
			Halt();
			return;
		}
		if (move_to < GetY() && GetYDir() > -this.RushSpeed)
			SetYDir(GetYDir() - 1);
		if (move_to > GetY() && GetYDir() < this.RushSpeed)
			SetYDir(GetYDir() + 1);
		if (Abs(GetYDir()) == 1) elevator->StartEngine();
		elevator->CheckPower(1);
		return;
	}

	// Search for waiting clonks
	var clonk, best;
	for (clonk in FindObjects(Find_InRect(-20, AbsY(elevator->GetY()), 40, LandscapeHeight() - elevator->GetY()), Find_OCF(OCF_CrewMember), Find_OCF(OCF_Alive), Find_NoContainer(), Find_Allied(GetOwner()), Sort_Distance(), Sort_Reverse()))
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
		if (clonk->GetY() > GetY() && GetContact(-1) & CNAT_Bottom) continue;
		// Priority rules: Cursor is better than no cursor, nearer is better than farer (Sort_Distance() & Sort_Reverse() do this)
		// So unlike in CR's elevator, no distance check has to be done because later cycles are always nearer clonks
		if (!best) best = clonk;
		else if (GetCursor(clonk->GetController()) == clonk)
			best = clonk;
		else if (GetCursor(best->GetController()) != best)
			best = clonk;
	}
	if (best) return MoveTo(best->GetY(), 35, best);

	// Stop, why do you move?
	Halt();
}

func Halt()
{
	if (!elevator) return;
	if (GetYDir()) elevator->StopEngine();
	SetYDir();
}

func ContactTop()
{
	Halt();
}
func ContactBottom()
{
	Halt();
	movement = 0;
	drill = false;
	ClearMoveTo();
}

// Checks whether the elevator should not move because someone's holding it
// Returns true if idle
func CheckIdle()
{
	for (var pusher in FindObjects(Find_InRect(-13, -13, 26, 26), Find_Action("Push")))
	{
		if (pusher->GetActionTarget() == this) return false;
		if (GetEffect("ElevatorControl", pusher->GetActionTarget()) && GetEffect("ElevatorControl", pusher->GetActionTarget()).case == this) return false;
	}
	return true;
}

// Moves the case to the specific y-coordinate
// delay in frames, so the elevator does not freak out
// target will be checked again for COMD_Stop and distance after delay run out
func MoveTo(int y, int delay, object target)
{
	// Not idle?
	if (!CheckIdle()) return false;
	Halt();
	move_to = BoundBy(y, elevator->GetY() + 20, LandscapeHeight());
	move_to_delay = delay;
	move_to_target = target;
	return true;
}

func ClearMoveTo()
{
	move_to = nil;
	move_to_target = nil;
	move_to_delay = nil;
}

/* Controls */

local movement, drill;

func ControlUseStart(object clonk) // Drilling
{
	ClearMoveTo();
	drill = true;
	movement = 1;
	return true;
}
func ControlUseStop(object clonk)
{
	drill = false;
	movement = 0;
	return true;
}

func ControlDown(object clonk)
{
	ClearMoveTo();
	if (!drill)
		movement = 1;
}
func ControlUp(object clonk)
{
	ClearMoveTo();
	if (!drill)
		movement = -1;
}

func ControlStop(object clonk, int control)
{
	if (control == CON_Up || control == CON_Down)
	{
		if (!drill)
			movement = 0;
		return true;
	}
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
			NextAction = "Drive"
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
			NextAction = "Drill",
			DigFree = 1
		}
};

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 2;
local Speed = 15;
local RushSpeed = 20; // When moving on its own