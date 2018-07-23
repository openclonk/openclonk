/*--
	Checkpoint
	Author: Maikel, Sven2

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

// TODO: The checkpoints themselves carry (and duplicate) a lot of logic that is
// handled much easier by the parkour goal. The script could be cleaned up to
// make the checkpoints lightweight and just callback to the parkour goal to do
// any logic with cross-checkpoint interaction (finding the next checkpoint, etc.)


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

// particle definition used for the effect around the check point
local checkpoint_particles;

public func SetCPMode(int mode)
{
	// PARKOUR_CP_Start always occurs alone.
	if (mode & PARKOUR_CP_Start) 
	{
		mode = PARKOUR_CP_Start;
		if (cp_con) cp_con->SetIndexedCP(this, 0);
	}
	// PARKOUR_CP_Finish only in combination with PARKOUR_CP_Team.	
	if (mode & PARKOUR_CP_Finish)
	{
		mode = mode & (PARKOUR_CP_Finish | PARKOUR_CP_Team);
		if (cp_con) cp_con->SetIndexedCP(this, GetNextCPNumber());
	}
	// PARKOUR_CP_Ordered must have PARKOUR_CP_Check and a number.
	var had_cp_num;
	if (mode & PARKOUR_CP_Ordered)
	{
		mode = mode | PARKOUR_CP_Check;
		// Set CP number.
		if (!cp_num) SetCPNumber(GetNextCPNumber());
		if (cp_con) cp_con->SetIndexedCP(this, cp_num);
	}
	else
	{
		had_cp_num = cp_num;
		cp_num = 0;
	}
	cp_mode = mode;
	if (had_cp_num) RenumberOrderedCheckpoints();
	DoGraphics();
	UpdateEditorHelp();
	if (cp_con)
	{
		if (mode & PARKOUR_CP_Start) cp_con->SetIndexedCP(this, 0);
		if (mode & PARKOUR_CP_Finish) cp_con->SetIndexedCP(this, ObjectCount(Find_ID(GetID()), Find_Func("GetCPNumber")) + 1);
	}
	return;
}

private func GetNextCPNumber()
{
	// TODO: This should really go through the controller...
	return ObjectCount(Find_ID(GetID()), Find_Func("GetCPNumber")) + 1;
}

public func RenumberOrderedCheckpoints()
{
	// Reassign all CP numbers. Use old numbers where possible
	var cps = FindObjects(Find_ID(ParkourCheckpoint), Find_Func("FindCPMode", PARKOUR_CP_Ordered)), i;
	SortArrayByProperty(cps, "cp_num");
	// If there is no start or finish checkpoint, assign them from the numbered pool
	var cp_start = FindObject(Find_ID(ParkourCheckpoint), Find_Func("FindCPMode", PARKOUR_CP_Start));
	var cp_finish = FindObject(Find_ID(ParkourCheckpoint), Find_Func("FindCPMode", PARKOUR_CP_Finish));
	if (!cp_start && GetLength(cps))
	{
		cp_start = cps[0];
		cps = cps[1:];
	}
	if (!cp_finish && GetLength(cps))
	{
		cp_finish = cps[-1];
		cps = cps[:-1];
	}
	// Re-label start and finish
	if (cp_start)
	{
		cp_start->SetCPNumber(0);
		cp_start->SetCPMode(PARKOUR_CP_Start | cp_start->GetCPMode());
	}
	if (cp_finish)
	{
		cp_finish->SetCPNumber(0);
		cp_finish->SetCPMode(PARKOUR_CP_Finish | cp_finish->GetCPMode());
	}
	// Re-label remaining CPs
	for (var cp in cps)
	{
		cp->SetCPNumber(++i);
		cp->DoGraphics();
		if (cp->GetCPController()) cp->GetCPController()->SetIndexedCP(cp, i);
	}
	return true;
}

public func Destruction()
{
	// CP deleted? Force renumbering without this CP.
	cp_num = 0;
	cp_mode = 0;
	RenumberOrderedCheckpoints();
}

public func GetCPMode() { return cp_mode; }

public func FindCPMode(int mode) { return cp_mode & mode; }


/*-- Checkpoint controller --*/
local cp_con;

public func SetCPController(object con)
{
	cp_con = con;
	UpdateEditorHelp();
	return;
}

public func GetCPController() { return cp_con; }


/*-- Checkpoint number --*/
local cp_num;

public func SetCPNumber(int num)
{
	cp_num = num;
	return;
}

public func GetCPNumber() { return cp_num; }


/*-- Checkpoint size --*/
local cp_size = 20;

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
	checkpoint_particles =
	{
		Size = PV_KeyFrames(0, 0, 0, 250, 10, 500, 0, 750, 10),
		Alpha = PV_KeyFrames(0, 0, 255, 500, 0, 501, 255, 1000, 0),
		BlitMode = GFX_BLIT_Additive,
		Attach = ATTACH_Front
	};
	cleared_by_plr = [];
	cp_mode = PARKOUR_CP_Check;
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

// Reset all cleared status
public func ResetCleared()
{
	cleared_by_plr = [];
	return true;
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
		var is_first_clear = (GetIndexOf(cleared_by_plr, true) < 0);
		// Check checkpoint status.
		if (cp_mode & PARKOUR_CP_Check)
		{
			var team_clear = !ClearedByTeam(team);
			ClearCPForPlr(plr, is_first_clear);
			if (ClearedByTeam(team) && team_clear)
				cp_con->AddTeamClearedCP(team, this); // Notify parkour goal.
		}
		// Check finish status.
		if (cp_mode & PARKOUR_CP_Finish)
		{
			Sound("UI::Cleared", false, 100, plr);
			cleared_by_plr[plrid] = true;
			if (team)
			{
				if (ClearedByTeam(team))
					cp_con->PlayerReachedFinishCP(plr, this, is_first_clear); // Notify parkour goal.
				else
					cp_con->AddPlayerClearedCP(plr, this, is_first_clear); // Notify parkour goal.
			}
			else
			{
				cp_con->PlayerReachedFinishCP(plr, this, is_first_clear); // Notify parkour goal.
			}
		}
		// Check bonus.
		if (cp_mode & PARKOUR_CP_Bonus)
			GameCall("GivePlrBonus", plr, this);
		// User callback
		if (is_first_clear) UserAction->EvaluateAction(on_first_cleared, this, clonk, plr);
		UserAction->EvaluateAction(on_cleared, this, clonk, plr);
	}
	return;
}

// Checkpoint callback if someone respawns here
private func OnPlayerRespawn(object clonk, int plr)
{
	return UserAction->EvaluateAction(on_respawn, this, clonk, plr);
}

// Clear this checkpoint for the player, and possibly its team members.
private func ClearCPForPlr(int plr, bool is_first_clear)
{
	if (!(cp_mode & PARKOUR_CP_Check))	
		return;
	var plrid = GetPlayerID(plr);
	cleared_by_plr[plrid] = true;
	Sound("UI::Cleared", false, 100, plr);
	cp_con->AddPlayerClearedCP(plr, this, is_first_clear); // Notify parkour goal.
	// Also clear for team members if the checkpoint is not PARKOUR_CP_Team.
	var team = GetPlayerTeam(plr);
	if (team && !(cp_mode & PARKOUR_CP_Team))
	{
		for (var test_plr in GetPlayers())
		{
			if (test_plr != plr && GetPlayerTeam(test_plr) == team)
			{
				var test_plr_id = GetPlayerID(test_plr);
				cleared_by_plr[test_plr_id] = true;
				Sound("UI::Cleared", false, 100, test_plr);
				cp_con->AddPlayerClearedCP(test_plr, this, false, true); // Notify parkour goal.
			}
		}
	}	
	return;
}


/*-- Checkpoint appearance --*/

local cp_name = "$Name$"; // auto-adjusted name. May differ from real name if another one is given in editor

// Mode graphics.
protected func DoGraphics()
{
	// Clear all overlays first.
	for (var i = 1; i <= 3; i++)
		SetGraphics(nil, nil, i);
	// Start & Finish.
	if (cp_mode & PARKOUR_CP_Start || cp_mode & PARKOUR_CP_Finish)
	{
		var gfx;
		if (cp_mode & PARKOUR_CP_Start) gfx = "Start"; else gfx = "";
		SetGraphics(gfx, ParkourFlag, 1, GFXOV_MODE_Base);
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
	// Name unless it has a custom overload
	if (GetName() == cp_name)
	{
		if (cp_mode & PARKOUR_CP_Ordered)
			cp_name = Format("$Name$ %02d", cp_num);
		else if (cp_mode & PARKOUR_CP_Start)
			cp_name = "$NameStart$";
		else if (cp_mode & PARKOUR_CP_Finish)
			cp_name = "$NameFinish$";
		else
			cp_name = "$Name$";
		SetName(cp_name);
	}
	return;
}

// Player graphics.
protected func UpdateGraphics(int time)
{
	// Create two sparks at opposite sides.
	var angle = (time * 10) % 360;
	var color = GetColorByAngle(angle);
	checkpoint_particles.R = (color >> 16) & 0xff;
	checkpoint_particles.G = (color >>  8) & 0xff;
	checkpoint_particles.B = (color >>  0) & 0xff;
	CreateParticle("SphereSpark", Sin(angle, cp_size), -Cos(angle, cp_size), 0, 0, 18 * 5, checkpoint_particles);
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

// Checkpoint order check
public func IsOrderedBefore(object other_cp)
{
	// Start before everything else. Finish after.
	if (cp_mode & PARKOUR_CP_Start) return true;
	if (cp_mode & PARKOUR_CP_Finish) return false;
	if (other_cp.cp_mode & PARKOUR_CP_Start) return false;
	if (other_cp.cp_mode & PARKOUR_CP_Finish) return true;
	// Ordered checkpoints in order and before unordered checkpoints
	if (cp_mode & PARKOUR_CP_Ordered)
	{
		if (!(other_cp.cp_mode & PARKOUR_CP_Ordered)) return true;
		return cp_num < other_cp.cp_num;
	}
}

// Storing checkpoints in Objects.c
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	var v;
	// Force dependencies to ensure proper checkpoint order
	for (var other_cp in FindObjects(Find_ID(GetID()), Find_Func("IsOrderedBefore", this))) other_cp->AddScenarioSaveDependency();
	// Checkpoints without a goal? Use regular saving.
	if (!cp_con)
	{
		if (GetCPMode()) props->AddCall("Checkpoint", this, "SetCPMode", GetBitmaskNameByValue(GetCPMode(), "PARKOUR_CP_"));
		if (GetCPNumber()) props->AddCall("Checkpoint", this, "SetCPNumber", GetCPNumber());
		return true;
	}
	else
	{
		// Special checkpoints
		props->RemoveCreation();
		if (cp_mode & PARKOUR_CP_Start)
			props->AddCall(SAVEOBJ_Creation, cp_con, "SetStartpoint", GetX(), GetY());
		else if (cp_mode & PARKOUR_CP_Finish)
			props->AddCall(SAVEOBJ_Creation, cp_con, "SetFinishpoint", GetX(), GetY(), !!(cp_mode & PARKOUR_CP_Team));
		else
		{
			var other_cp_modes = cp_mode & (~PARKOUR_CP_Finish) & (~PARKOUR_CP_Start);
			props->AddCall(SAVEOBJ_Creation, cp_con, "AddCheckpoint", GetX(), GetY(), GetBitmaskNameByValue(other_cp_modes, "PARKOUR_CP_"));
		}
	}
	// Checkpoint properties
	v = GetCPSize();
	if (v != GetID().cp_size) props->AddCall("Checkpoint", this, "SetCPSize", v);
	if (GetName() == cp_name) props->Remove("Name"); // Do not store name if given automatically
	return true;
}

/* Editor */

// Editor action callbacks
local on_cleared, on_first_cleared, on_respawn;

public func SetOnCleared(v) { on_cleared=v; return true; }
public func SetOnFirstCleared(v) { on_first_cleared=v; return true; }
public func SetOnRespawn(v) { on_respawn=v; return true; }

// Inividual mode getting/setting functions (for editor)
public func SetCPRespawn(bool to_val) { return SetCPMode((GetCPMode() & ~PARKOUR_CP_Respawn) | (PARKOUR_CP_Respawn * !!to_val)); }
public func SetCPCheck(bool to_val) { return SetCPMode((GetCPMode() & ~PARKOUR_CP_Check) | (PARKOUR_CP_Check * !!to_val)); }
public func SetCPOrdered(bool to_val) { return SetCPMode((GetCPMode() & ~PARKOUR_CP_Ordered) | (PARKOUR_CP_Ordered * !!to_val)); }
public func SetCPTeam(bool to_val) { return SetCPMode((GetCPMode() & ~PARKOUR_CP_Team) | (PARKOUR_CP_Team * !!to_val)); }

public func GetCPRespawn() { return GetCPMode() & (PARKOUR_CP_Respawn | PARKOUR_CP_Start); }
public func GetCPCheck() { return GetCPMode() & PARKOUR_CP_Check; }
public func GetCPOrdered() { return GetCPMode() & PARKOUR_CP_Ordered; }
public func GetCPTeam() { return GetCPMode() & PARKOUR_CP_Team; }

// Placement in editor: Auto-assign checkpoint mode
public func EditorInitialize()
{
	// Auto-assign controller
	SetCPController(FindObject(Find_ID(Goal_Parkour)));
	// Default checkpoint mode
	var new_mode;
	if (!FindObject(Find_ID(GetID()), Find_Func("FindCPMode", PARKOUR_CP_Start)))
		new_mode = PARKOUR_CP_Start;
	else
		new_mode = PARKOUR_CP_Finish;
	// Change old finish point to numbered checkpoint
	var cp = FindObject(Find_ID(GetID()), Find_Func("FindCPMode", PARKOUR_CP_Finish));
	if (cp) cp->SetCPMode(PARKOUR_CP_Check | PARKOUR_CP_Ordered | PARKOUR_CP_Respawn);
	SetCPMode(new_mode);
	UpdateEditorHelp();
	return this;
}

public func UpdateEditorHelp()
{
	// EditorHelp: Include a warning if no goal has been created
	EditorHelp = Description;
	if (!cp_con) EditorHelp = Format("%s|%s", EditorHelp, "$NoGoalWarning$");
	return true;
}

local EditorActions = {
	SelectPrev = { Name="$SelectPrev$", EditorHelp="$SelectPrevHelp$", Command="SelectOther(-1)", Select=true },
	SelectNext = { Name="$SelectNext$", EditorHelp="$SelectNextHelp$", Command="SelectOther(+1)", Select=true },
	ReorderPrev = { Name="$ReorderPrev$", EditorHelp="$ReorderPrevHelp$", Command="MoveOrder(-1)" },
	ReordertNext = { Name="$ReorderNext$", EditorHelp="$ReorderNextHelp$", Command="MoveOrder(+1)" }
};

public func SelectOther(int direction)
{
	// Traverse through numbered checkpoints or start/finish
	var cps = FindObjects(Find_ID(GetID()), Find_Func("GetCPNumber"));
	var ncps = GetLength(cps);
	var next_num;
	if (cp_mode & PARKOUR_CP_Finish)
		next_num = ncps + 1;
	else
		next_num = cp_num;
	next_num += direction;
	if (next_num <= 0) return FindObject(Find_ID(GetID()), Find_Func("FindCPMode", PARKOUR_CP_Start));
	SortArrayByProperty(cps, "cp_num");
	if (next_num-1 >= ncps) return FindObject(Find_ID(GetID()), Find_Func("FindCPMode", PARKOUR_CP_Finish));
	return cps[next_num-1];
}

public func MoveOrder(int direction)
{
	// Valid swap command?
	var is_start = (cp_mode & PARKOUR_CP_Start);
	var is_finish = (cp_mode & PARKOUR_CP_Finish);
	if (!cp_num && !is_start && !is_finish) return false;
	var other = SelectOther(direction);
	if (other == this) return false;
	// Swap them!
	var order_flags = PARKOUR_CP_Start | PARKOUR_CP_Finish | PARKOUR_CP_Ordered;
	var swap_num = cp_num, swap_mode = cp_mode;
	cp_num = other.cp_num;
	cp_mode = (cp_mode & ~order_flags) | (other.cp_mode & order_flags);
	other.cp_num = swap_num;
	other.cp_mode = (other.cp_mode & ~order_flags) | (swap_mode & order_flags);
	// Force update
	SetCPMode(cp_mode);
	other->SetCPMode(other.cp_mode);
	return true;
}

public func Definition(def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.respawn = { Type="bool", Name="$Respawn$", EditorHelp="$RespawnHelp$", AsyncGet="GetCPRespawn", Set="SetCPRespawn" };
	def.EditorProps.check = { Type="bool", Name="$Check$", EditorHelp="$CheckHelp$", AsyncGet="GetCPCheck", Set="SetCPCheck" };
	def.EditorProps.ordered = { Type="bool", Name="$Ordered$", EditorHelp="$OrderedHelp$", AsyncGet="GetCPOrdered", Set="SetCPOrdered" };
	def.EditorProps.team = { Type="bool", Name="$Team$", EditorHelp="$TeamHelp$", AsyncGet="GetCPTeam", Set="SetCPTeam" };
	def.EditorProps.on_cleared = new UserAction.Prop { Name="$OnCleared$", EditorHelp="$OnClearedHelp$", Set="SetOnCleared", Save="Checkpoint" };
	def.EditorProps.on_first_cleared = new UserAction.Prop { Name="$OnFirstCleared$", EditorHelp="$OnFirstClearedHelp$", Set="SetOnFirstCleared", Save="Checkpoint" };
	def.EditorProps.on_respawn = new UserAction.Prop { Name="$OnRespawn$", EditorHelp="$OnRespawnHelp$", Set="SetOnRespawn", Save="Checkpoint" };
}


/*-- Proplist --*/



local Name = "$Name$";
local Description = "$Description$";
local EditorHelp = "$Description$";


