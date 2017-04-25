/**
	Relaunch Rule
	This rule enables and handles relaunches.
	@author Maikel, Sven2, Fulgen
*/

// Determines whether the inventory of the crew member is transfered upon respawn.
local inventory_transfer = false;
// Determines whether a crew member needs to be bought.
local free_crew = true;
// Determines whether the clonk will be respawned at the base.
local respawn_at_base = false;
// Determines whether only the last clonk gets respawned.
local respawn_last_clonk = false;

local default_relaunch_count = nil;
local relaunches = [];

local clonk_type = Clonk;

local disable_last_weapon = false;
local last_used_player_weapons = [];
local relaunch_time = 36 * 10;
local hold = false;
local allow_restart_player = false;
local respawn_script_players = false;
local perform_restart = true;

public func Activate(int plr)
{
	// Only restart player if enabled unless this is a definition call.
	if (this != Rule_Relaunch && !allow_restart_player)
		return MessageWindow(this.Description, plr);
	// Notify scenario.
	if (GameCall("OnPlayerRestart", plr))
		return;
	// Remove the player's clonk, including contents.
	var clonk = GetCrew(plr);
	if (clonk && clonk->GetCrewEnabled())
	{
		clonk->Kill(clonk, true);
		clonk->RemoveObject();
	}
}

protected func Initialize()
{
	ScheduleCall(this, this.CheckDescription, 1, 1);
	if (GetScenarioVal("Mode", "Game") == "Melee")
		default_relaunch_count = 5;
	return true;
}

private func CheckDescription()
{
	// If neutral flagpoles exist, update name and description.
	if(respawn_at_base)
	{
		if(ObjectCount(Find_ID(Flagpole), Find_Func("IsNeutral")))
		{
			SetName("$Name2$");
			this.Description = "$Description2$";
		}
		else
		{
			SetName("$Name3$");
			this.Description = "$Description3$";
		}
	}
	else
	{
		SetName("$Name$");
		this.Description = "$Description$";
	}
	return true;
}

public func SetInventoryTransfer(bool transfer)
{
	inventory_transfer = transfer;
	return this;
}

public func GetInventoryTransfer()
{
	return inventory_transfer;
}

public func SetFreeCrew(bool free)
{
	free_crew = free;
	return this;
}

public func GetFreeCrew()
{
	return free_crew;
}

public func SetRespawnDelay(int delay)
{
	relaunch_time = delay * 36;
	return this;
}

public func GetRespawnDelay()
{
	return relaunch_time / 36;
}

public func SetHolding(bool b)
{
	hold = b;
	return this;
}

public func GetHolding()
{
	return hold;
}

public func SetLastWeaponUse(bool use)
{
	this.disable_last_weapon = !use;
	return this;
}

public func GetLastWeaponUse()
{
	return disable_last_weapon;
}

public func SetBaseRespawn(bool set)
{
	respawn_at_base = set;
	return this;
}

public func GetBaseRespawn()
{
	return respawn_at_base;
}

public func SetDefaultRelaunches(int r)
{
	default_relaunch_count = r;
}

public func SetLastClonkRespawn(bool b)
{
	respawn_last_clonk = b;
	return this;
}

public func AllowPlayerRestart()
{
	allow_restart_player = true;
	return this;
}

public func DisallowPlayerRestart()
{
	allow_restart_player = false;
	return this;
}

public func GetLastClonkRespawn()
{
	return respawn_last_clonk;
}

public func SetPerformRestart(bool on)
{
	perform_restart = on;
}

public func InitializePlayer(int plr)
{
	_inherited(plr, ...);
	// Scenario script callback.
	relaunches[plr] = default_relaunch_count;
	if(!GameCallEx("OnPlayerRelaunch", plr, false)) return DoRelaunch(plr, nil, nil, true);
}

public func OnClonkDeath(object clonk, int killer)
{
	if (!clonk || !perform_restart)
		return;
	var plr = clonk->GetOwner();
	if (plr == NO_OWNER || (!respawn_script_players && GetPlayerType(plr) == C4PT_Script)) return;
	
	if (default_relaunch_count != nil)
	{
		relaunches[plr]--;
		if(relaunches[plr] < 0)
		{
			EliminatePlayer(plr);
			return;
		}
	}
	
	GameCall("OnPlayerRelaunch", plr, true);
	
	return DoRelaunch(plr, clonk, nil);
}

private func RespawnAtBase(object clonk)
{
	var base = GetRelaunchBase(clonk);
	if	(base)
		return [base->GetX(), base->GetY() + base->GetDefHeight() / 2];
}

private func TransferInventory(object from, object to)
{
	if(!from || !to) return;
	// Drop some items that cannot be transferred (such as connected pipes and dynamite igniters)
	var i = from->ContentsCount(), contents;
	while (i--)
	{
		if (contents = from->Contents(i))
		{
			if (contents->~IsDroppedOnDeath(from))
			{
				contents->Exit();
			}
			else
			{
				// The new clonk doesn't burn. To be consistent, also extinguish contents
				contents->Extinguish();
			}
		}
	}
	return to->GrabContents(from);
}

private func GetRelaunchBase(object clonk)
{
	var plr = clonk->GetOwner();
	// Neutral flagpoles are preferred respawn points, because they are used as the only respawn points in missions.
	var base = clonk->FindObject(Find_ID(Flagpole), Find_Func("IsNeutral"), clonk->Sort_Distance());
	// If there are no neutral flagpoles, find closest base owned by the player (or team) and try to buy a clonk.
	if (!base)
		base = clonk->FindObject(Find_Func("IsBaseBuilding"), Find_Allied(plr), clonk->Sort_Distance());
	return base;
}

public func DoRelaunch(int plr, object clonk, array position, bool no_creation)
{
	if (!GetPlayerName(plr))
		return;
	if (respawn_last_clonk && GetCrewCount(plr) >= 1)
		return;
	
	if (respawn_at_base)
		position = RespawnAtBase(clonk);
	position = position ?? GameCallEx("RelaunchPosition", plr, GetPlayerTeam(plr));
	position = position ?? FindRelaunchPos(plr);
	
	var spawn;
	// Position array either has the form [x, y] or [[x, y], [x, y], ...].
	if (GetType(position) == C4V_Array)
	{
		if (GetType(position[0]) == C4V_Array)
		{
			spawn = position[Random(GetLength(position))];
		}
		else spawn = position;
	}
	// If no spawn has been found set it to the middle of the landscape, this should not happen.
	spawn = spawn ?? [LandscapeWidth() / 2, LandscapeHeight() / 2];
	
	var new_clonk;
	if (!no_creation)
	{
		if (free_crew)
		{
			new_clonk = CreateObjectAbove(clonk_type, spawn[0], spawn[1], plr);
			if (!new_clonk)
				return;
			new_clonk->MakeCrewMember(plr);
		}
		else
		{
			var base = GetRelaunchBase();
			if (!base) return;
			// Try to buy a crew member at the base.
			var pay_plr = base->GetOwner();
			// Payment in neutral bases by clonk owner.
			if (pay_plr == NO_OWNER) 
			pay_plr = plr;
			new_clonk = base->~DoBuy(clonk_type, plr, pay_plr, clonk);
			if (new_clonk)
			{
				new_clonk->Exit();
			}
		}
	}
	else
	{
		new_clonk = GetCrew(plr);
		if (!new_clonk)
			return;
	}
	
	if (inventory_transfer)
		TransferInventory(clonk, new_clonk);
	
	new_clonk->SetPosition(spawn[0], spawn[1], plr);
	
	if (!GetCursor(plr) || GetCursor(plr) == clonk)
		SetCursor(plr, new_clonk);
	new_clonk->DoEnergy(new_clonk.Energy ?? 100000);
	
	if (relaunch_time)
	{
		var container = new_clonk->CreateObject(RelaunchContainer, nil, nil, plr);
		container->StartRelaunch(new_clonk);
	}
	return true;
}

protected func FindRelaunchPos(int plr)
{
	var loc = FindLocation(Loc_Or(Loc_Sky(), Loc_Tunnel()), Loc_Space(20, CNAT_Top), Loc_Wall(CNAT_Bottom));
	if (loc == nil)
		return nil;
	return [loc.x, loc.y];
}


/*-- Scenario saving --*/

public func SaveScenarioObject(props, ...)
{
	if (!inherited(props, ...)) 
		return false;
	// Custom properties
	props->Remove("Name"); // updated by initialization
	props->Remove("Description"); // updated by initialization
	if (inventory_transfer) 
		props->AddCall("InventoryTransfer", this, "SetInventoryTransfer", inventory_transfer);
	if (free_crew) 
		props->AddCall("FreeCrew", this, "SetFreeCrew", free_crew);
	if(respawn_at_base)
		props->AddCall("BaseRespawn", this, "SetBaseRespawn", respawn_at_base);
	return true;
}


/*-- Globals --*/

global func SetRelaunchCount(int plr, int value)
{
	if(UnlimitedRelaunches()) return;
	GetRelaunchRule().relaunches[plr] = value;
	Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule().relaunches[plr]);
	return value;
}

global func GetRelaunchCount(int plr)
{
	return GetRelaunchRule().relaunches[plr];
}

global func DoRelaunchCount(int plr, int value)
{
	if(UnlimitedRelaunches()) return;
	GetRelaunchRule().relaunches[plr] += value;
	Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule().relaunches[plr]);
	return;
}

global func UnlimitedRelaunches()
{
	return GetRelaunchRule().default_relaunch_count == nil;
}

global func GetRelaunchRule()
{
	return FindObject(Find_ID(Rule_Relaunch)) || CreateObject(Rule_Relaunch);
}


/*-- Editor --*/

public func Definition(def)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.inventory_transfer = { Name="$InventoryTransfer$", EditorHelp="$InventoryTransferHelp$", Type="bool", Set="SetInventoryTransfer" };
	def.EditorProps.free_crew = { Name="$FreeCrew$", EditorHelp="$FreeCrewHelp$", Type="bool", Set="SetFreeCrew" };
	def.EditorProps.respawn_at_base = {
		Name = "$RespawnAtBase$",
		EditorHelp = "$RespawnAtBaseHelp$",
		Type = "bool",
		Set = "SetBaseRespawn"
	};
	
	def.EditorProps.hold = {
		Name = "$Holding$",
		EditorHelp = "$HoldingHelp$",
		Type = "bool",
		Set = "Setholding"
	};
	
	def.EditorProps.respawn_delay = {
		Name = "$RespawnDelay$",
		EditorHelp = "$RespawnDelayHelp$",
		Type = "int",
		Set = "SetRespawnDelay"
	};
	
	def.EditorProps.relaunch_count = {
		Name = "$RelaunchCount$",
		EditorHelp = "$RelaunchCountHelp$",
		Type = "int",
		Set = "SetDefaultRelaunches"
	};
}


/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
