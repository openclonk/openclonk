/*-- CheckPoint --*/
/* 
	A checkpoint can have different modes, indicated with a bitmask:
		*None - Not a Checkpoint.
		*Start - Start of the race.
		*Finish - End of the race.
		*Respawn - The clonk can respawn at this CP.
		*Check - This checkpoint must be reached in order to complete the goal.
		*Ordered - These checkpoints must be reached in the right order.
*/

/*-- Checkpoint modes --*/
local CP_Mode;
static const RACE_CP_None = 0;
static const RACE_CP_Start = 1;
static const RACE_CP_Finish = 2;
static const RACE_CP_Respawn = 4;
static const RACE_CP_Check = 8;
static const RACE_CP_Ordered = 16;

public func SetCPMode(int iMode) 
{
	if (iMode & RACE_CP_Start) // Start always occurs alone.
		iMode = RACE_CP_Start;
	if (iMode & RACE_CP_Finish) // Start always occurs alone.
		iMode = RACE_CP_Finish;
	if (iMode & RACE_CP_Ordered) // Ordered checkpoints must have RACE_CP_Check.
	{
		iMode | RACE_CP_Check;
		// Set CP number.
		SetCPNumber(ObjectCount(Find_ID(GetID()), Find_Func("GetCPNumber")) + 1);
	}
	CP_Mode = iMode; 
	UpdateGraphics();
	return;
}

public func GetCPMode() { return CP_Mode; }

/*-- Checkpoint controller --*/
local CP_Con;

public func SetCPController(object pCon) 
{ 
	CP_Con = pCon; 
	return; 
}

/*-- Checkpoint number --*/
local CP_Num;

public func SetCPNumber(int iNum) 
{ 
	CP_Num = iNum; 
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

public func ClearedByPlr(int iPlr)
{
	return aDoneByPlr[iPlr];
}

public func ClearedByTeam(int iTeam)
{
	return aDoneByTeam[iTeam];
}

public func IsActiveForPlr(int iPlr)
{
	if (CP_Mode & RACE_CP_Finish) // Check all checkpoints.
	{
		for (var cp in FindObjects(Find_ID(GetID())))
			if (cp->GetCPMode() & RACE_CP_Check)
				if (!cp->ClearedByPlr(iPlr))
					return false;
		return true;
	}
	if (CP_Mode & RACE_CP_Ordered) 
	{
		if (GetCPNumber() == 1) // First ordered checkpoint is always active.
			return true;
		for (var cp in FindObjects(Find_ID(GetID()), Find_Func("GetCPNumber")))
			if (cp->GetCPNumber() + 1 == GetCPNumber())
				if (cp->ClearedByPlr(iPlr))
					return true;
		return false;
	}
	return true;
}

public func IsActiveForTeam(int iTeam)
{
	if (!iTeam)
		return false;
	for (var i = 0; i < GetPlayerCount(); i++)
		if (GetPlayerTeam(GetPlayerByIndex(i)) == iTeam)
			if (IsActiveForPlr(GetPlayerByIndex(i)))
				return true;
	return false;
}

/*-- Checkpoint activity --*/

protected func FxIntCheckpointTimer()
{	
	CheckForClonks();
	return FX_OK;
}

protected func CheckForClonks()
{
	// Loop through all clonks inside the checkpoint.
	for (var pClonk in FindObjects(Find_OCF(OCF_CrewMember), Find_Distance(30)))
	{
		var iPlr = pClonk->GetOwner();
		var iTeam = GetPlayerTeam(iPlr);
		// Check whether this CP is already activated for iPlr or its team.
		if (!IsActiveForPlr(iPlr))
			if (!IsActiveForTeam(iTeam))
				continue;
		// Check respawn status.
		if (CP_Mode & RACE_CP_Respawn) 
			if (CP_Con)
				CP_Con->SetPlrRespawnCP(iPlr, this); // Notify race goal.
		// If already done by player -> continue.
		if (aDoneByPlr[iPlr])
			continue;
		// Check checkpoint status.
		if (CP_Mode & RACE_CP_Check)
		{
			aDoneByPlr[iPlr] = true;
			if (!iTeam)
				if (CP_Con)
					CP_Con->AddPlrClearedCP(iPlr); // Notify race goal.
			if (iTeam && !aDoneByTeam[iTeam])
			{
				aDoneByTeam[iTeam] = true;
				if (CP_Con)
					CP_Con->AddPlrClearedCP(iPlr); // Notify race goal.
			}
			UpdateColor();
		}
		// Check finish status.
		if (CP_Mode & RACE_CP_Finish) 
		{
			aDoneByPlr[iPlr] = true;
			if (CP_Con)
				CP_Con->PlayerHasReachedFinish(iPlr); // Notify race goal.
			UpdateColor();
		}
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
	// Ordered.
	if (CP_Mode & RACE_CP_Ordered)
	{
		SetGraphics(Format("%d", GetCPNumber()), NUMB, 4, GFXOV_MODE_Base);
		SetObjDrawTransform(200, 0, 0, 0, 200, 9000, 4);
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
		{
			SetGraphics("Ring8", GetID(), 5 + i, GFXOV_MODE_Base);
			SetClrModulation(GetPlrColor(i), 5 + i);
			var angle = ringsec * 45;
			var s = Sin(angle, 1000), c = Cos(angle, 1000);
			SetObjDrawTransform(c, s, 0, -s, c, 0, 5 + i);
			ringsec++;
		}
	}
	return;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
