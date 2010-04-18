/*-- 
		CheckPoint
		Author: Maikel

		The parkour goal uses checkpoints to allow for user defined routes.
		A checkpoint can have different modes, indicated with a bitmask:
			*None - Not a Checkpoint.
			*Start - Start of the parkour.
			*Finish - End of the parkour.
			*Respawn - The clonk can respawn at this CP.
			*Check - This checkpoint must be cleared in order to complete the parkour.
			*Ordered - These checkpoints must be cleared in the right order.
			*Bonus - Player receives a bonus if he cleares this CP.
--*/


/*-- Checkpoint modes --*/
local cp_mode;
static const RACE_CP_None = 0;
static const RACE_CP_Start = 1;
static const RACE_CP_Finish = 2;
static const RACE_CP_Respawn = 4;
static const RACE_CP_Check = 8;
static const RACE_CP_Ordered = 16;
static const RACE_CP_Bonus = 32;

public func SetCPMode(int mode) 
{
	if (mode & RACE_CP_Start) // Start always occurs alone.
		mode = RACE_CP_Start;
	if (mode & RACE_CP_Finish) // Start always occurs alone.
		mode = RACE_CP_Finish;
	if (mode & RACE_CP_Ordered) // Ordered checkpoints must have RACE_CP_Check.
	{
		mode = mode | RACE_CP_Check;
		// Set CP number.
		SetCPNumber(ObjectCount(Find_ID(GetID()), Find_Func("GetCPNumber")) + 1);
	}
	cp_mode = mode; 
	DoGraphics();
	return;
}

public func GetCPMode() { return cp_mode; }

public func FindCPMode(int mode) { return cp_mode & mode; }

/*-- Checkpoint controller --*/
local cp_con;

public func SetCPController(object con) 
{ 
	cp_con = con; 
	return; 
}

/*-- Checkpoint number --*/
local cp_num;

public func SetCPNumber(int num) 
{ 
	cp_num = num; 
	return; 
}

public func GetCPNumber() { return cp_num; }

/*-- Initialize --*/

local aDoneByPlr; // Array to keep track of players which were already here.
local aDoneByTeam; // Array to keep track of teams which were already here.

protected func Initialize()
{
	aDoneByPlr = [];
	aDoneByTeam = [];
	cp_mode = RACE_CP_Check;
	UpdateGraphics();
	AddEffect("IntCheckpoint", this, 100, 1, this);
	return;
}	

/*-- Checkpoint status --*/

public func ClearedByPlr(int plr)
{
	var plrid = GetPlayerID(plr);
	return aDoneByPlr[plrid];
}

public func ClearedByTeam(int team)
{
	return aDoneByTeam[team];
}

public func IsActiveForPlr(int plr)
{
	if (cp_mode & RACE_CP_Finish) // Check all checkpoints.
	{
		for (var cp in FindObjects(Find_ID(GetID())))
			if (cp->GetCPMode() & RACE_CP_Check)
				if (!cp->ClearedByPlr(plr))
					return false;
		return true;
	}
	if (cp_mode & RACE_CP_Ordered) 
	{
		if (GetCPNumber() == 1) // First ordered checkpoint is always active.
			return true;
		for (var cp in FindObjects(Find_ID(GetID()), Find_Func("GetCPNumber")))
			if (cp->GetCPNumber() + 1 == GetCPNumber())
				if (cp->ClearedByPlr(plr))
					return true;
		return false;
	}
	return true;
}

public func IsActiveForTeam(int team)
{
	if (!team)
		return false;
	for (var i = 0; i < GetPlayerCount(); i++)
		if (GetPlayerTeam(GetPlayerByIndex(i)) == team)
			if (IsActiveForPlr(GetPlayerByIndex(i)))
				return true;
	return false;
}

/*-- Checkpoint activity --*/

protected func FxIntCheckpointTimer(object target, int fxnum, int fxtime)
{	
	if (!(fxtime % 5))
		CheckForClonks();
	UpdateGraphics(fxtime);
	return FX_OK;
}

protected func CheckForClonks()
{
	// Loop through all clonks inside the checkpoint.
	for (var pClonk in FindObjects(Find_OCF(OCF_CrewMember), Find_Distance(20)))
	{		
		var plr = pClonk->GetOwner();
		var team = GetPlayerTeam(plr);
		var plrid = GetPlayerID(plr);
		// Check whether this CP is already activated for player or its team.
		if (!IsActiveForPlr(plr) && !IsActiveForTeam(team))
			continue;
		// Check respawn status.
		if (cp_mode & RACE_CP_Respawn) 
			if (cp_con)
				cp_con->SetPlrRespawnCP(plr, this); // Notify race goal.
		// If already done by player -> continue.
		if (aDoneByPlr[plrid])
			continue;
		// Check checkpoint status.
		if (cp_mode & RACE_CP_Check)
		{
			aDoneByPlr[plrid] = true;
			Sound("Cleared", false, nil, plr);
			if (!team)
				if (cp_con)
					cp_con->AddPlrClearedCP(plr); // Notify parkour goal.
			if (team && !aDoneByTeam[team])
			{
				aDoneByTeam[team] = true;
				if (cp_con)
					cp_con->AddPlrClearedCP(plr); // Notify parkour goal.
			}
		}
		// Check finish status.
		if (cp_mode & RACE_CP_Finish) 
		{
			Sound("Cleared", false, nil, plr);
			aDoneByPlr[plrid] = true;
			if (team)
				aDoneByTeam[team] = true;
			if (cp_con)
				cp_con->PlrReachedFinishCP(plr); // Notify parkour goal.
		}
		// Check bonus.
		if (cp_mode & RACE_CP_Bonus)
			GameCall("GivePlrBonus", plr, this);
	}
	return;
}

/*-- Checkpoint appearance --*/

// Mode graphics.
protected func DoGraphics()
{
	// Clear all overlays first.
	for (var i = 1; i <= 3; i++)
		SetGraphics(0, 0, i);
	// Start & Finish.
	if (cp_mode & RACE_CP_Start || cp_mode & RACE_CP_Finish)
	{
		SetGraphics("", ParkourFlag, 1, GFXOV_MODE_Base);
		SetObjDrawTransform(350, 0, 2000, 0, 350, 2000, 1);
		SetClrModulation(RGBa(255, 255, 255, 160) , 1);
	}
	// Ordered, display numbers up to 99.
	if (cp_mode & RACE_CP_Ordered)
	{
		var shift = 0;
		if (GetCPNumber() >= 10)
		{
			SetGraphics(Format("%d", GetCPNumber()/10), Icon_Number, 3, GFXOV_MODE_Base);
			SetObjDrawTransform(300, 0, -4500, 0, 300, 0, 3);
			SetClrModulation(RGBa(255, 255, 255, 128) , 3);
			shift = 1;
		}
		SetGraphics(Format("%d", GetCPNumber()%10), Icon_Number, 2, GFXOV_MODE_Base);
		SetObjDrawTransform(300, 0, shift * 4500, 0, 300, 0, 2);
		SetClrModulation(RGBa(255, 255, 255, 128) , 2);
	}
	return;
}

// Player graphics.
protected func UpdateGraphics(int time)
{
	// Create two sparks at opposite sides.
	var angle = (time * 10) % 360;
	var color = GetColorByAngle(angle);
	CreateParticle("PSpark", Sin(angle, 20), -Cos(angle, 20), 0, 0, 32, color);
	angle = (angle + 180) % 360;
	var color = GetColorByAngle(angle);
	CreateParticle("PSpark", Sin(angle, 20), -Cos(angle, 20), 0, 0, 32, color);
	return;
}

protected func GetColorByAngle(int angle)
{
	// Get cleared count.
	var cnt = 0;
	for (var i = 0; i < GetPlayerCount(); i++)
		if (ClearedByPlr(GetPlayerByIndex(i)) || (cp_mode & RACE_CP_Start))
			cnt++;
	if (!cnt) 
		return RGBa(255, 255, 255, 192);

	var prt = 360 / cnt;
	var j = 0;
	// Find the right player.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (ClearedByPlr(plr) || (cp_mode & RACE_CP_Start))
		{
			if (angle >= j * prt && angle < (j + 1) * prt)
				return GetPlayerColor(plr);
			j++;
		}
	}
	
	// Should not happen...
	return RGBa(255, 255, 255, 192);
}

/*-- Proplist --*/

protected func Definition(def)
{
	SetProperty("Name", "$Name$", def);
}
