/**
	Relaunch Rule
	This rule enables and handles relaunches.
	@author Maikel, Sven2, Fulgen
*/

// Determines whether the inventory of the crew member is transfered upon respawn.
local inventory_transfer = false;

// Determines whether a crew member needs to be bought.
local free_crew = true;

//Determines whether the clonk will be respawned at the base
local respawn_at_base = false;
//Determines whether only the last clonk gets respawned
local RespawnLastClonk = false;

local DefaultRelaunchCount = nil;
local aRelaunches = [];

local ClonkType = Clonk;

local DisableLastWeapon = false;
local LastUsedPlayerWeapons = [];
local RelaunchTime = 36 * 10;
local Hold = false;
local RestartPlayer = false;

public func Activate(int plr)
{
	if(!RestartPlayer) return MessageWindow(this.Description, plr);
	// Notify scenario.
	if (!GameCall("OnPlayerRestart", plr))
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
	if(GetScenarioVal("Mode", "Game") == "Melee") DefaultRelaunchCount = 5;
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
	return true;
}

public func GetInventoryTransfer()
{
	return inventory_transfer;
}

public func SetFreeCrew(bool free)
{
	free_crew = free;
	return true;
}

public func GetFreeCrew()
{
	return free_crew;
}

public func SetRespawnDelay(int iDelay)
{
	RelaunchTime = iDelay * 36;
	return this;
}

public func GetRespawnDelay()
{
	return RelaunchTime / 36;
}

public func SetHolding(bool fHold)
{
	Hold = fHold;
	return this;
}

public func GetHolding()
{
	return Hold;
}

public func SetLastWeaponUse(bool fUse)
{
	this.DisableLastWeapon = !fUse;
	return this;
}

public func GetLastWeaponUse()
{
	return DisableLastWeapon;
}

public func SetBaseRespawn(bool fSet)
{
	respawn_at_base = fSet;
	return this;
}

public func GetBaseRespawn()
{
	return respawn_at_base;
}

public func SetDefaultRelaunches(int iRelaunches)
{
	DefaultRelaunchCount = iRelaunches;
}

public func SetLastClonkRespawn(bool b)
{
	RespawnLastClonk = b;
	return this;
}

public func EnablePlayerRestart()
{
	RestartPlayer = true;
	return this;
}

public func DisablePlayerRestart()
{
	RestartPlayer = false;
	return this;
}

public func GetLastClonkRespawn()
{
	return RespawnLastClonk;
}

public func InitializePlayer(int iPlr)
{
	_inherited(iPlr, ...);
	// Scenario script callback.
	aRelaunches[iPlr] = DefaultRelaunchCount;
	GameCallEx("OnPlayerRelaunch", iPlr, false);
	return DoRelaunch(iPlr, nil, nil, true);
}

/*public func OnClonkDeath(int plr, object pClonk, int iKiller)
{
	return RelaunchPlayer(plr, iKiller, pClonk);
}*/

public func RelaunchPlayer(int plr, int killer, object pClonk)
{
	if(plr == nil || plr == NO_OWNER) return Log("NO PlAYER: %d", plr);
	
	if(DefaultRelaunchCount != nil)
	{
		aRelaunches[plr]--;
		if(aRelaunches[plr] < 0)
		{
			EliminatePlayer(plr);
			return;
		}
	}
	
	GameCall("OnPlayerRelaunch", plr, true);
	
	return DoRelaunch(plr, pClonk, nil);
}

private func RespawnAtBase(int iPlr, object pClonk)
{
	for(var base in GetBases(pClonk))
	{
		if(base) return [base->GetX(), base->GetY() + base->GetDefHeight() / 2];
	}
}

private func TransferInventory(object from, object to)
{
	if(!from || !to) return;
	// Drop some items that cannot be transferred (such as connected pipes and dynamite igniters)
	var i = from->ContentsCount(), contents;
	while (i--)
		if (contents = from->Contents(i))
			if (contents->~IsDroppedOnDeath(from))
			{
				contents->Exit();
			}
			else
			{
				// The new clonk doesn't burn. To be consistent, also extinguish contents
				contents->Extinguish();
			}
	return to->GrabContents(from);
}

private func GetBases(object clonk)
{
	var plr = clonk->GetOwner();
	// Neutral flagpoles are preferred respawn points, because they are used as the only respawn points in missions.
	var bases = clonk->FindObjects(Find_ID(Flagpole), Find_Func("IsNeutral"), clonk->Sort_Distance());
	// If there are no neutral flagpoles, find closest base owned by the player (or team) and try to buy a clonk.
	if (GetLength(bases) <= 0) 
		bases = clonk->FindObjects(Find_Func("IsBaseBuilding"), Find_Allied(plr), clonk->Sort_Distance());
	return bases;
}

public func DoRelaunch(int iPlr, object pClonk, array position, bool fNoCreation)
{
	if(!GetPlayerName(iPlr)) return;
	if(RespawnLastClonk && GetCrewCount(iPlr) >= 1) return;
	
	if(respawn_at_base) position = RespawnAtBase(iPlr, pClonk);
	position = (position ?? GameCallEx("RelaunchPosition", iPlr, GetPlayerTeam(iPlr))) ?? FindRelaunchPos(iPlr);
	
	var spawn;
	
	
	// position array either has the form [x, y] or [[x, y], [x, y], ...]
	if(GetType(position) == C4V_Array)
	{
		if(GetType(position[0]) == C4V_Array)
		{
			spawn = position[Random(GetLength(position))];
		}
		else spawn = position;
	}
	
	var clonk;
	if(!fNoCreation)
	{
		if(free_crew)
		{
			clonk = CreateObjectAbove(ClonkType, spawn[0], spawn[1],iPlr);
			if(!clonk) return;
			clonk->MakeCrewMember(iPlr);
		}
		else
		{
			// Try to buy a crew member at the base.
			var pay_plr = base->GetOwner();
			// Payment in neutral bases by clonk owner.
			if (pay_plr == NO_OWNER) 
			pay_plr = plr;
			clonk = base->~DoBuy(ClonkType, plr, pay_plr, pClonk);
			if (clonk)
			{
				clonk->Exit();
			}
		}
	}
	else
	{
		clonk = GetCrew(iPlr);
		if(!clonk) return;
	}
	
	if (inventory_transfer) TransferInventory(pClonk, clonk);
	
	clonk->SetPosition(spawn[0], spawn[1], iPlr)
	
	if(!GetCursor(iPlr) || GetCursor(iPlr) == pClonk) SetCursor(iPlr, clonk);
	clonk->DoEnergy(clonk.Energy || 100000);
	
	if(RelaunchTime)
	{
		clonk->CreateObject(RelaunchContainer,nil,nil,iPlr)->StartRelaunch(clonk);
	}
	return true;
}

protected func FindRelaunchPos(int plr)
{
	var tx, ty; // Test position.
	for (var i = 0; i < 500; i++)
	{
		tx = Random(LandscapeWidth());
		ty = Random(LandscapeHeight());
		if (GBackSemiSolid(AbsX(tx), AbsY(ty)))
			continue;
		if (GBackSemiSolid(AbsX(tx+5), AbsY(ty+10)))
			continue;
		if (GBackSemiSolid(AbsX(tx+5), AbsY(ty-10)))
			continue;
		if (GBackSemiSolid(AbsX(tx-5), AbsY(ty+10)))
			continue;
		if (GBackSemiSolid(AbsX(tx-5), AbsY(ty-10)))
			continue;
		// Succes.
		return [tx, ty];
	}
	return nil;
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
	GetRelaunchRule().aRelaunches[plr] = value;
	Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule().aRelaunches[plr]);
	return value;
}

global func GetRelaunchCount(int plr)
{
	return GetRelaunchRule().aRelaunches[plr];
}

global func DoRelaunchCount(int plr, int value)
{
	if(UnlimitedRelaunches()) return;
	GetRelaunchRule().aRelaunches[plr] += value;
	Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule().aRelaunches[plr]);
	return;
}

global func UnlimitedRelaunches()
{
	return GetRelaunchRule().DefaultRelaunchCount == nil;
}

global func GetRelaunchRule()
{
	return FindObject(Find_ID(Rule_Relaunch)) || CreateObject(Rule_Relaunch);
}

/* Editor */

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
		Set = "SetHolding"
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
