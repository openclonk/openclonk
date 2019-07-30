/**
	Stone Door
	A door which can be used in scenarios with lots of bricks.
	
	@authors Ringwaul, Maikel	
*/

#include Library_SwitchTarget

protected func Initialize()
{
	SetAction("Door");
	SetComDir(COMD_Stop);
	return;
}

/*-- Movement --*/

public func OpenDoor()
{
	ForceDigFree();
	SetComDir(COMD_Up);
	Sound("Structures::StoneGate::GateMove");
	return;
}

public func CloseDoor()
{
	ForceDigFree();
	SetComDir(COMD_Down);
	Sound("Structures::StoneGate::GateMove");
	return;
}

private func IsOpen()
{
	if (GetContact(-1) & CNAT_Top)
	 	return true;
	return false;
}

private func IsClosed()
{
	if (GetContact(-1) & CNAT_Bottom)
	 	return true;
	return false;
}

protected func Hit()
{
	Sound("Structures::StoneGate::GateHit");
	return;
}

// Digs away earth behind the door. Needs to temporarily disable the solid mask, though.
private func ForceDigFree()
{
	SetSolidMask();
	DigFreeRect(GetX() - 4, GetY() - 20, 8, 40, true);
	SetSolidMask(0, 0, 8, 40);
}

/*-- Switch control --*/

// Reaction to operation by a switch: if open_door is true the door opens, otherwise it closes
public func OnSetInputSignal(object operator, object switch, bool open_door)
{
	if (open_door)
	{
		OpenDoor();
	}
	else
	{
		CloseDoor();
	}

	_inherited(operator, switch, open_door, ...);
}

/*-- Automatic movement --*/

// Overrules owner control and only let's the team through.
public func SetAutoControl(int team)
{
	var effect = AddEffect("AutoControl", this, 100, 3, this);
	effect.Team = team;
	return;
}

protected func FxAutoControlTimer(object target, effect, int time)
{
	var d = 0;
	if (IsOpen())
		d = 30;
	var owner = GetOwner();
	var team = effect.Team;
	var open_door = false;
	// Team control
	if (team != nil)
		for (var clonk in FindObjects(Find_OCF(OCF_CrewMember), Find_InRect(-50, d - 30, 100, 60)))
		{
			var plr = clonk->GetOwner();
			var plr_team = GetPlayerTeam(plr);
			if (team == 0 || plr_team == team)
				open_door = true;			
		}
	// Player control
	else
		if (FindObject(Find_OCF(OCF_CrewMember), Find_InRect(-50, d - 30, 100, 60), Find_Allied(owner)))
			open_door = true;
	
	// Keep door closed if hostile?
	// TODO?
	
	if (open_door && IsClosed())
		OpenDoor();
	if (!open_door && IsOpen())
		CloseDoor();
	
	return 1;
}

func FxAutoControlSaveScen(obj, fx, props)
{
	props->AddCall("AutoControl", obj, "SetAutoControl", fx.team);
	return true;
}

/*-- Destruction --*/

private func GetStrength() { return 180; }

protected func Damage()
{
	// Destroy if damage above strength.
	if (GetDamage() > GetStrength())
	{
		var particles = 
		{
			Size = PV_KeyFrames(0, 0, 0, 100, PV_Random(3, 5), 1000, 3),
			R = PV_Random(230, 250),
			G = PV_Random(210, 230),
			B = PV_Random(190, 210),
			Alpha = PV_Linear(255, 0),
			ForceY = PV_Gravity(100),
			CollisionVertex = 0
		};
		CreateParticle("SmokeDirty", PV_Random(-4, 4), PV_Random(-18, 18), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 60), particles, 300);
		CastObjects(Rock, 5, 20);
		return RemoveObject();
	}
	// Change appearance.
	DoGraphics();
	return;
}

private func DoGraphics()
{
	// Change appearance according to damage and strength.
	if (GetDamage() > 3 * GetStrength() / 4)
		SetGraphics("Cracked3");
	else if (GetDamage() > GetStrength() / 2)
		SetGraphics("Cracked2");
	else if (GetDamage() > GetStrength() / 4)
		SetGraphics("Cracked1");
	else
		SetGraphics("");
	return;
}

public func GetFloorOffset()
{
	// Searches downwards from the lowest vertex to the floor
	var y_off;
	for (y_off = 0; !GBackSolid(0, 20 + y_off); ++y_off)
		if (y_off > 20) break; // max range
	return y_off;
}


/* Editor */

local EditorActions = {
	OpenDoor = { Name = "$DoorUp$", Command = "OpenDoor()" },
	CloseDoor = { Name = "$DoorDown$", Command = "CloseDoor()" }
};

public func Definition(def, ...)
{
	UserAction->AddEvaluator("Action", "Structure", "$DoorUp$", "$DoorUpDesc$", "open_door", [def, def.EvalAct_OpenDoor], { }, UserAction->GetObjectEvaluator("IsDoor", "$Door$", "$DoorTargetHelp$"), "Door");
	UserAction->AddEvaluator("Action", "Structure", "$DoorDown$", "$DoorDownDesc$", "close_door", [def, def.EvalAct_CloseDoor], { }, UserAction->GetObjectEvaluator("IsDoor", "$Door$", "$DoorTargetHelp$"), "Door");
	return _inherited(def, ...);
}

private func EvalAct_OpenDoor(props, context)
{
	var door = UserAction->EvaluateValue("Object", props.Door, context);
	if (door) door->~OpenDoor();
}

private func EvalAct_CloseDoor(props, context)
{
	var door = UserAction->EvaluateValue("Object", props.Door, context);
	if (door) door->~CloseDoor();
}

/* Properties */

public func IsDoor() { return true; }

local ActMap = {
	Door = {
		Prototype = Action,
		Name = "Door",
		Procedure = DFA_FLOAT,
		Speed = 150,
		Accel = 12,
		Decel = 12,
		Length = 1,
		Delay = 1,
		X = 0,
		Y = 0,
		Wdt = 8,
		Hgt = 40,
		NextAction = "Door",
	},
};

local Name = "$Name$";
local Plane = 200;
local Components = {Rock = 6};


