/**
	Stone Door
	A door which can be used in scenarios with lots of bricks.
	
	@authors Ringwaul, Maikel	
*/

protected func Initialize()
{
	SetAction("Door");
	SetComDir(COMD_Stop);
	return;
}

/*-- Movement --*/

public func OpenDoor()
{
	SetComDir(COMD_Up);
	Sound("GateMove");
	return;
}

public func CloseDoor()
{
	SetComDir(COMD_Down);
	Sound("GateMove");
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
	Sound("GateHit");
	return;
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
