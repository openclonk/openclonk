/*--
	Checkpoint
	Author: Maikel

	The parkour goal uses checkpoints to allow for user defined routes.
	A checkpoint can have different modes, indicated with a bitmask:
		*None - Not a Checkpoint.
		*Start - Start of the parkour.
		*Finish - End of the parkour.
		*Respawn - The clonk can respawn at this CP.
		*Check - This checkpoint must be cleared in order to complete the parkour.
		*Ordered - These checkpoints must be cleared in the right order.
		*Team - All players of a team must have cleared this CP.
		*Bonus - Player receives a bonus if he cleares this CP.
--*/


/*-- Checkpoint modes --*/
local cp_mode;
static const PARKOUR_CP_None = 0;
static const PARKOUR_CP_Start = 1;
static const PARKOUR_CP_Finish = 2;
static const PARKOUR_CP_Respawn = 4;
static const PARKOUR_CP_Check = 8;
static const PARKOUR_CP_Ordered = 16;
static const PARKOUR_CP_Team = 32;
static const PARKOUR_CP_Bonus = 64;

public func SetCPMode(int mode)
{
	// PARKOUR_CP_Start always occurs alone.
	if (mode & PARKOUR_CP_Start) 
		mode = PARKOUR_CP_Start;
	// PARKOUR_CP_Finish only in combination with PARKOUR_CP_Team.	
	if (mode & PARKOUR_CP_Finish)
		mode = mode & (PARKOUR_CP_Finish | PARKOUR_CP_Team) ;
	// PARKOUR_CP_Ordered must have PARKOUR_CP_Check and a number.
	if (mode & PARKOUR_CP_Ordered)
	{
		mode = mode | PARKOUR_CP_Check;
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

/*-- Checkpoint size --*/
local cp_size;

public func SetCPSize(int size)
{
	cp_size = BoundBy(size, 10, 100);
	return;
}

public func GetCPSize() { return cp_size; }

/*-- Initialize --*/

local cleared_by_plr; // Array to keep track of players which were already here.

protected func Initialize()
{
	cleared_by_plr = [];
	cp_mode = PARKOUR_CP_Check;
	cp_size = 20;
	UpdateGraphics();
	AddEffect("IntCheckpoint", this, 100, 1, this);
	return;
}

/*-- Checkpoint status --*/

// Returns whether this checkpoint has been cleared by player.
public func ClearedByPlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	return cleared_by_plr[plrid];
}

// Returns whether this checkpoint has been cleared by team.
public func ClearedByTeam(int team)
{
	if (!team)
		return false;
	if (cp_mode & PARKOUR_CP_Team)
	{
		// PARKOUR_CP_Team: Cleared if all players of the team have cleared the checkpoint.
		for (var i = 0; i < GetPlayerCount(); i++)
			if (GetPlayerTeam(GetPlayerByIndex(i)) == team)
				if (!ClearedByPlayer(GetPlayerByIndex(i)))
					return false;
		return true;					
	}
	else
	{
		// Not PARKOUR_CP_Team: Cleared if one player has cleared the checkpoint.
		for (var i = 0; i < GetPlayerCount(); i++)
			if (GetPlayerTeam(GetPlayerByIndex(i)) == team)
				if (ClearedByPlayer(GetPlayerByIndex(i)))
					return true;
	}
	return false;
}

// Whether this checkpoint is active for a player.
public func IsActiveForPlayer(int plr)
{
	// PARKOUR_CP_Finish: Check all PARKOUR_CP_Check checkpoints.
	if (cp_mode & PARKOUR_CP_Finish)
	{
		for (var cp in FindObjects(Find_ID(GetID())))
			if (cp->GetCPMode() & PARKOUR_CP_Check)
				if (!cp->ClearedByPlayer(plr))
					return false;
		return true;
	}
	// PARKOUR_CP_Ordered: Check previous PARKOUR_CP_Ordered checkpoint.
	if (cp_mode & PARKOUR_CP_Ordered)
	{
		// First ordered checkpoint is always active.
		if (GetCPNumber() == 1) 
			return true;
		for (var cp in FindObjects(Find_ID(GetID()), Find_Func("GetCPNumber")))
			if (cp->GetCPNumber() + 1 == GetCPNumber())
			{	
				var team = GetPlayerTeam(plr);		
				if (cp->GetCPMode() & PARKOUR_CP_Team && team)
				{
					if (cp->ClearedByTeam(team))
						return true;
				}				
				else if (cp->ClearedByPlayer(plr))
					return true;
			}
		return false;
	}
	// Other modes are always active.
	return true;
}

// Whether this checkpoint is active for a team.
public func IsActiveForTeam(int team)
{
	if (!team)
		return false;
	// Checkpoint is active for a team if it is active for one of its members.
	for (var i = 0; i < GetPlayerCount(); i++)
		if (GetPlayerTeam(GetPlayerByIndex(i)) == team)
			if (IsActiveForPlayer(GetPlayerByIndex(i)))
				return true;
	return false;
}

/*-- Checkpoint activity --*/

protected func FxIntCheckpointTimer(object target, effect, int fxtime)
{
	// Check every 5 frames.
	if (!(fxtime % 5))
		CheckForClonks();
	UpdateGraphics(fxtime);
	return FX_OK;
}

protected func CheckForClonks()
{
	// Only check if controlled by a parkour goal.
	if (!cp_con)
		return;
	// Loop through all clonks inside the checkpoint.
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember), Find_Distance(cp_size)))
	{
		var plr = clonk->GetOwner();
		var team = GetPlayerTeam(plr);
		var plrid = GetPlayerID(plr);
		// Check whether this CP is already activated for player or its team.
		if (!IsActiveForPlayer(plr) && !IsActiveForTeam(team))
			continue;
		// Check respawn status.
		if (cp_mode & PARKOUR_CP_Respawn)
			cp_con->SetPlayerRespawnCP(plr, this); // Notify parkour goal.
		// If already done by player -> continue.
		if (ClearedByPlayer(plr))
			continue;
		// Check checkpoint status.
		if (cp_mode & PARKOUR_CP_Check)
		{
			var team_clear = !ClearedByTeam(team);
			cleared_by_plr[plrid] = true;
			Sound("Cleared", false, 100, plr);
			cp_con->AddPlayerClearedCP(plr, this); // Notify parkour goal.
			if (ClearedByTeam(team) && team_clear)
				cp_con->AddTeamClearedCP(team, this); // Notify parkour goal.
		}
		// Check finish status.
		if (cp_mode & PARKOUR_CP_Finish)
		{
			Sound("Cleared", false, 100, plr);
			cleared_by_plr[plrid] = true;
			if (team)
			{
				if (ClearedByTeam(team))
					cp_con->PlayerReachedFinishCP(plr, this); // Notify parkour goal.
				else
					cp_con->AddPlayerClearedCP(plr, this); // Notify parkour goal.
			}
			else
			{
				cp_con->PlayerReachedFinishCP(plr, this); // Notify parkour goal.
			}
		}
		// Check bonus.
		if (cp_mode & PARKOUR_CP_Bonus)
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
	if (cp_mode & PARKOUR_CP_Start || cp_mode & PARKOUR_CP_Finish)
	{
		SetGraphics("", ParkourFlag, 1, GFXOV_MODE_Base);
		SetObjDrawTransform(350, 0, 2000, 0, 350, 2000, 1);
		SetClrModulation(RGBa(255, 255, 255, 160) , 1);
	}
	// Ordered, display numbers up to 99.
	if (cp_mode & PARKOUR_CP_Ordered)
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
	CreateParticle("PSpark", Sin(angle, cp_size), -Cos(angle, cp_size), 0, 0, 32, color);
	angle = (angle + 180) % 360;
	var color = GetColorByAngle(angle);
	CreateParticle("PSpark", Sin(angle, cp_size), -Cos(angle, cp_size), 0, 0, 32, color);
	return;
}

protected func GetColorByAngle(int angle)
{
	// Get cleared count.
	var cnt = 0;
	for (var i = 0; i < GetPlayerCount(); i++)
		if (ClearedByPlayer(GetPlayerByIndex(i)) || (cp_mode & PARKOUR_CP_Start))
			cnt++;
	if (!cnt)
		return RGBa(255, 255, 255, 192);

	var prt = 360 / cnt;
	var j = 0;
	// Find the right player.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (ClearedByPlayer(plr) || (cp_mode & PARKOUR_CP_Start))
		{
			if (angle >= j * prt && angle < (j + 1) * prt)
				return GetPlayerColor(plr);
			j++;
		}
	}
	
	// Should not happen...
	return RGBa(255, 255, 255, 192);
}

/*-- Misc --*/

// Clears all materials behind a checkpoint.
public func ClearCPBack()
{
	var x = GetX();
	var y = GetY();
	// Approximate by rectangles for now.
	for (var d = 0; d <= cp_size; d++)
	{
		var dx = x - d;
		var dy = y - Sqrt(cp_size**2 - d**2);
		var wdt = 2 * d;
		var hgt = 2 * Sqrt(cp_size**2 - d**2);		
		ClearFreeRect(dx, dy, wdt, hgt);		
	}
	return;
}

/*-- Proplist --*/
local Name = "$Name$";
