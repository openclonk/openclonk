/*-- CheckPoint --*/
/* 
	A checkpoint can have different modes, indicated with a bitmask:
		*None - Not a Checkpoint.
		*Start - Start of the race.
		*Finish - End of the race.
		*Respawn - The clonk can respawn at this CP.
		*Check - This checkpoint must be reached in order to complete the goal.
		*Ordered - These checkpoints must be reached in the right order.
		*Bonus - Player receives a bonus if he cleares this CP.
*/

/*-- Checkpoint modes --*/
local CP_Mode;
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
	CP_Mode = mode; 
	UpdateGraphics();
	return;
}

public func GetCPMode() { return CP_Mode; }

public func FindCPMode(int mode) { return CP_Mode & mode; }

/*-- Checkpoint controller --*/
local CP_Con;

public func SetCPController(object con) 
{ 
	CP_Con = con; 
	return; 
}

/*-- Checkpoint number --*/
local CP_Num;

public func SetCPNumber(int num) 
{ 
	CP_Num = num; 
	return; 
}

public func GetCPNumber() { return CP_Num; }

/*-- Initialize --*/

local aDoneByPlr; // Array to keep track of players which were already here.
local aDoneByTeam; // Array to keep track of teams which were already here.

protected func Initialize()
{
	aDoneByPlr = [];
	aDoneByTeam = [];
	CP_Mode = RACE_CP_Check;
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
	if (CP_Mode & RACE_CP_Finish) // Check all checkpoints.
	{
		for (var cp in FindObjects(Find_ID(GetID())))
			if (cp->GetCPMode() & RACE_CP_Check)
				if (!cp->ClearedByPlr(plr))
					return false;
		return true;
	}
	if (CP_Mode & RACE_CP_Ordered) 
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

protected func FxIntCheckpointTimer(object pTarget, int iEffectNumber, int iEffectTime)
{	
	CheckForClonks();
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
		if (CP_Mode & RACE_CP_Respawn) 
			if (CP_Con)
				CP_Con->SetPlrRespawnCP(plr, this); // Notify race goal.
		// If already done by player -> continue.
		if (aDoneByPlr[plrid])
			continue;
		// Check checkpoint status.
		if (CP_Mode & RACE_CP_Check)
		{
			aDoneByPlr[plrid] = true;
			Sound("Cleared", false, nil, plr);
			if (!team)
				if (CP_Con)
					CP_Con->AddPlrClearedCP(plr); // Notify race goal.
			if (team && !aDoneByTeam[team])
			{
				aDoneByTeam[team] = true;
				if (CP_Con)
					CP_Con->AddPlrClearedCP(plr); // Notify race goal.
			}
			UpdateColor();
		}
		// Check finish status.
		if (CP_Mode & RACE_CP_Finish) 
		{
			Sound("Cleared", false, nil, plr);
			aDoneByPlr[plrid] = true;
			if (team)
				aDoneByTeam[team] = true;
			if (CP_Con)
				CP_Con->PlrReachedFinishCP(plr); // Notify race goal.
			UpdateColor();
		}
		// Check bonus.
		if (CP_Mode & RACE_CP_Bonus)
			GameCall("GivePlrBonus", plr, this);
	}
	return;
}

/*-- Checkpoint appearance --*/

// Mode-graphics.
protected func UpdateGraphics()
{
	// Clear all overlays first.
	for (var i = 1; i <= 4; i++)
		SetGraphics(0, 0, i);
	// Start.
	if (CP_Mode & RACE_CP_Start)
		SetGraphics("Start", GetID(), 1, GFXOV_MODE_Base);
	// Finish.
	if (CP_Mode & RACE_CP_Finish)
		SetGraphics("Finish", GetID(), 1, GFXOV_MODE_Base);
	// Respawn.
	if (CP_Mode & RACE_CP_Respawn)
	{
		SetGraphics("Respawn", GetID(), 2, GFXOV_MODE_Base);
		SetObjDrawTransform(1000, 0, -6000, 0, 1000, -3000, 2);
	}
	// Check.
	if (CP_Mode & RACE_CP_Check)
	{
		SetGraphics("Check", GetID(), 3, GFXOV_MODE_Base);
		SetObjDrawTransform(1000, 0, 6000, 0, 1000, -3000, 3);
	}
	// Ordered, display numbers up to 99.
	if (CP_Mode & RACE_CP_Ordered)
	{
		if (GetCPNumber() >= 10)
		{
			SetGraphics(Format("%d", GetCPNumber()/10), NUMB, 5, GFXOV_MODE_Base);
			SetObjDrawTransform(200, 0, -2500, 0, 200, 9000, 5);
			SetClrModulation(RGBa(255, 255, 255, 128) , 5);
		}
		SetGraphics(Format("%d", GetCPNumber()%10), NUMB, 4, GFXOV_MODE_Base);
		SetObjDrawTransform(200, 0, 2500, 0, 200, 9000, 4);
		SetClrModulation(RGBa(255, 255, 255, 128) , 4);
	}
	return;
}

// Player cleared-graphics.
protected func UpdateColor()
{
	// Calculate nr of players that reached this CP.
	var plrcnt = 0;
	for (var i = 0; i < GetLength(aDoneByPlr); i++)
		if (aDoneByPlr[i])
			plrcnt++;
	// Ring color, for players that cleared this CP.
	var ringsec = 0;
	for (var i = 0; i < GetLength(aDoneByPlr); i++)
	{
		if (aDoneByPlr[i])
		{ // Does not work with TeamColors=1.
			SetGraphics("Ring8", GetID(), 6 + i, GFXOV_MODE_Base);
			SetClrModulation(GetPlrColor(GetPlayerByIndex(ringsec)), 6 + i); //WRONG.
			var angle = ringsec * 45;
			var s = Sin(angle, 1000), c = Cos(angle, 1000);
			SetObjDrawTransform(c, s, 0, -s, c, 0, 6 + i);
			ringsec++;
		}
	}
	return;
}

/*-- Proplist --*/

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
