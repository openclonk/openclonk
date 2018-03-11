/**
	Relaunch Rule
	This rule enables and handles relaunches with its various aspects. These are:
	 * SetInventoryTransfer(bool transfer): inventory of crew is transferred on respawn [default false].
	 * SetFreeCrew(bool free): whether the crew is free or needs to be bought [default false].
	 * SetBaseRespawn(bool set): whether to respawn at a nearby base [default false].
	 * SetLastClonkRespawn(bool b): whether to respawn the last clonk only [default false].
	 * SetRespawnDelay(int delay): respawn delay in seconds [default 10].
	 * SetAllowPlayerRestart(bool on): whether a player can select restart in the rule menu.
	 * SetPerformRestart(bool on): whether this rule actually handles the respawn [default true].
	 * SetDefaultRelaunchCount(int r): the number of relaunches a player has [default nil == infinte].
	 * SetInitialRelaunch(bool on): whether a relaunch on round start is done [default true].
	The active relaunch rule can be obtained by the global function GetRelaunchRule(). The rule also
	keeps track of player's actual number of relaunches which can be modified and accessed by:
	 * SetPlayerRelaunchCount(int plr, int value): set player relaunch count.
	 * GetPlayerRelaunchCount(int plr): get player relaunch count.
	 * DoPlayerRelaunchCount(int plr, int value): add to player relaunch count.
	 * HasUnlimitedRelaunches(): whether the players have infinite relaunches.
	 
	@author Maikel, Sven2, Fulgen
*/


/*-- Settings --*/

// Determines whether the inventory of the crew member is transfered upon respawn.
local inventory_transfer = false;

public func SetInventoryTransfer(bool transfer)
{
	inventory_transfer = transfer;
	return this;
}

public func GetInventoryTransfer() { return inventory_transfer; }


// Determines whether a crew member needs to be bought.
local free_crew = true;

public func SetFreeCrew(bool free)
{
	free_crew = free;
	return this;
}

public func GetFreeCrew() { return free_crew; }


// Determines whether the clonk will be respawned at the base.
local respawn_at_base = false;

public func SetBaseRespawn(bool set)
{
	respawn_at_base = set;
	return this;
}

public func GetBaseRespawn() { return respawn_at_base; }


// Determines whether only the last clonk gets respawned.
local respawn_last_clonk = false;

public func SetLastClonkRespawn(bool b)
{
	respawn_last_clonk = b;
	return this;
}

public func GetLastClonkRespawn() { return respawn_last_clonk; }


// Determines the amount of time in the relaunch container.
local relaunch_time = 10; // seconds

public func SetRespawnDelay(int delay)
{
	relaunch_time = delay;
	return this;
}

public func GetRespawnDelay() { return relaunch_time / 36; }


// Determines whether a player can select to restart in a round via the rule menu.
local allow_restart_player = false;
public func SetAllowPlayerRestart(bool on)
{
	allow_restart_player = on;
	return this;
}

public func GetAllowPlayerRestart() { return allow_restart_player; }


// Determines whether a relaunch is performed by the rule.
local perform_restart = true;

public func SetPerformRestart(bool on)
{
	perform_restart = on;
	return this;
}

public func GetPerformRestart() { return perform_restart; }


// Determines the default relaunch count.
local default_relaunch_count = nil;
local relaunches = [];

public func SetDefaultRelaunchCount(int r)
{
	default_relaunch_count = r;
	return this;
}

public func GetDefaultRelaunchCount() { return default_relaunch_count; }

// Determines whether a relaunch is needed on round start.
local initial_relaunch = true;

public func SetInitialRelaunch(bool on)
{
	initial_relaunch = on;
	return this;
}

public func GetInitialRelaunch() { return initial_relaunch; }


// Determines whether the crew is released after weapon selection.
local hold_crew = false;

public func SetHolding(bool hold)
{
	hold_crew = hold;
	return this;
}

public func GetHolding() { return hold_crew; }


// Determines whether the player can select the last weapon again.
local disable_last_weapon = false;
local last_used_player_weapons = [];

public func SetLastWeaponUse(bool use)
{
	disable_last_weapon = !use;
	return this;
}

public func GetLastWeaponUse() { return disable_last_weapon; }


// Not modifiable at the moment.
local respawn_script_players = false;
local clonk_type = Clonk;


/*-- Rule Code --*/

public func Activate(int plr)
{
	// Only restart player if enabled unless this is a definition call.
	if (this != Rule_Relaunch && !allow_restart_player)
		return MessageWindow(this.Description, plr);
	// Notify scenario and stop execution if handled by scenario.
	if (GameCall("OnPlayerActivatedRestart", plr))
		return;
	// Remove the player's clonk, including contents.
	var clonk = GetCrew(plr);
	if (clonk && clonk->GetCrewEnabled())
	{
		clonk->Kill(clonk, true);
		clonk->RemoveObject();
	}
	return;
}

public func Initialize()
{
	ScheduleCall(this, this.CheckDescription, 1, 1);
	if (GetScenarioVal("Mode", "Game") == "Melee")
		default_relaunch_count = 5;
	return;
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

public func InitializePlayer(int plr)
{
	_inherited(plr, ...);
	relaunches[plr] = default_relaunch_count;
	// Check if relaunch is needed.
	if (!initial_relaunch || !perform_restart)
		return;	
	// Scenario script and goals callbacks.
	GameCallEx("OnPlayerRelaunchCountChanged", plr);
	if (GameCall("OnPlayerRelaunch", plr, false))
		return;
	return DoRelaunch(plr, nil, nil, true);
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
		if (relaunches[plr] < 0)
		{
			EliminatePlayer(plr);
			return;
		}
	}
	// Scenario script and goals callbacks.
	GameCallEx("OnPlayerRelaunchCountChanged", plr);
	if (GameCall("OnPlayerRelaunch", plr, true))
		return;
	return DoRelaunch(plr, clonk, nil);
}

private func RespawnAtBase(int plr, object clonk)
{
	var base = GetRelaunchBase(plr, clonk);
	if (base)
		return [base->GetX(), base->GetY() + base->GetBottom()];
	return;
}

private func TransferInventory(object from, object to)
{
	if (!from || !to) return;
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

private func GetRelaunchBase(int plr, object clonk)
{
	plr = plr ?? clonk->GetOwner();
	// Neutral flagpoles are preferred respawn points, because they are used as the only respawn points in missions.
	var base = FindObject(Find_ID(Flagpole), Find_Func("IsNeutral"), Sort_Random());
	if (clonk)
		clonk->FindObject(Find_ID(Flagpole), Find_Func("IsNeutral"), clonk->Sort_Distance());
	// If there are no neutral flagpoles, find closest base owned by the player (or team) and try to buy a clonk.
	if (!base)
	{
		base = FindObject(Find_Func("IsBaseBuilding"), Find_Allied(plr), Sort_Random());
		if (clonk)
			base = clonk->FindObject(Find_Func("IsBaseBuilding"), Find_Allied(plr), clonk->Sort_Distance());
	}
	return base;
}

public func DoRelaunch(int plr, object clonk, array position, bool no_creation)
{
	if (!GetPlayerName(plr))
		return;
	if (respawn_last_clonk && GetCrewCount(plr) >= 1)
		return;
	
	if (respawn_at_base)
		position = RespawnAtBase(plr, clonk);
	position = position ?? GameCall("RelaunchPosition", plr, GetPlayerTeam(plr));
	position = position ?? this->FindRelaunchPos(plr);
	
	var spawn;
	// Position array either has the form [x, y] or [[x, y], [x, y], ...].
	if (GetType(position) == C4V_Array)
	{
		if (GetType(position[0]) == C4V_Array)
			spawn = position[Random(GetLength(position))];
		else
			spawn = position;
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
			var base = GetRelaunchBase(plr, clonk);
			if (!base)
				return;
			// Try to buy a crew member at the base.
			var pay_plr = base->GetOwner();
			// Payment in neutral bases by clonk owner.
			if (pay_plr == NO_OWNER) 
				pay_plr = plr;
			new_clonk = base->~DoBuy(clonk_type, plr, pay_plr, clonk);
			if (!new_clonk)
				return;
			new_clonk->Exit();
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
	
	new_clonk->SetPosition(spawn[0], spawn[1] - new_clonk->GetBottom(), plr);
	
	if (!GetCursor(plr) || GetCursor(plr) == clonk)
		SetCursor(plr, new_clonk);
	new_clonk->DoEnergy(new_clonk.MaxEnergy ?? 100000);
	
	if (relaunch_time)
	{
		var container = new_clonk->CreateObject(RelaunchContainer, 0, new_clonk->GetBottom(), plr);
		container->SetRelaunchTime(relaunch_time, hold_crew);
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


/*-- Scenario Saving --*/

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
	if (respawn_at_base)
		props->AddCall("BaseRespawn", this, "SetBaseRespawn", respawn_at_base);
	props->RemoveCreation();
	props->Add(SAVEOBJ_Creation, "GetRelaunchRule()");
	return true;
}


/*-- Globals --*/

// Returns the active relaunch rule, creates one if no exists.
global func GetRelaunchRule()
{
	return FindObject(Find_ID(Rule_Relaunch)) || CreateObject(Rule_Relaunch);
}

global func IsActiveRelaunchRule()
{
	return !!FindObject(Find_ID(Rule_Relaunch));
}


/*-- Player Relaunches --*/

public func SetPlayerRelaunchCount(int plr, int value)
{
	if (HasUnlimitedRelaunches())
		return;
	relaunches[plr] = value;
	Scoreboard->SetPlayerData(plr, "relaunches", relaunches[plr]);
	return;
}

public func GetPlayerRelaunchCount(int plr)
{
	return relaunches[plr];
}

public func DoPlayerRelaunchCount(int plr, int value)
{
	if(HasUnlimitedRelaunches())
		return;
	relaunches[plr] += value;
	Scoreboard->SetPlayerData(plr, "relaunches", relaunches[plr]);
	return;
}

public func HasUnlimitedRelaunches()
{
	return default_relaunch_count == nil;
}


/*-- Editor --*/

public func Definition(proplist def)
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
		Set = "SetDefaultRelaunchCount"
	};
}


/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
