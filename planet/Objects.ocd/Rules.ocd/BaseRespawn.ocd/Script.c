/**
	Base Respawn 
	If neutral flagpoles are available, respawn there (for free)
	Otherwise, respawn at the nearest base if sufficient clonks and clunkers are available.
	TODO: make more general for all kinds of available crew members.
	
	@author Maikel, Sven2
*/


// Determines whether the inventory of the crew member is transfered upon respawn.
local inventory_transfer;

// Determines whether a crew member needs to be bought.
local free_crew;

protected func Initialize()
{
	inventory_transfer = false;
	free_crew = false;
	ScheduleCall(this, this.CheckDescription, 1,1);
	return true;
}

private func CheckDescription()
{
	// If neutral flagpoles exist, update name and description
	if (ObjectCount(Find_ID(Flagpole), Find_Func("IsNeutral")))
	{
		SetName("$Name2$");
		this.Description = "$Description2$";
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

protected func OnClonkDeath(object clonk)
{
	var plr = clonk->GetOwner();
	if (!GetPlayerName(plr)) return true; // skip eliminated players, NO_OWNER, etc.
	
	// Only respawn a clonk if it is the last crew member.
	if (GetCrewCount(plr) >= 1) return true;

	// Neutral flagpoles are preferred respawn points, because they are used as the only respawn points in missions
	var bases = clonk->FindObjects(Find_ID(Flagpole), Find_Func("IsNeutral"), clonk->Sort_Distance());
	// If there are no neutral flagpoles, find closest base owned by the player (or team) and try to buy a clonk.
	if (!GetLength(bases)) bases = clonk->FindObjects(Find_Func("IsBaseBuilding"), Find_Allied(plr), clonk->Sort_Distance());
	for (var base in bases)
	{
		if (!base)
			continue;
			
		// If free crew just create a clonk at the base.
		if (free_crew)
		{
			var crew = CreateObject(Clonk, base->GetX() - GetX(), base->GetY() + base->GetDefHeight() / 2 - GetX(), plr);
			crew->MakeCrewMember(plr);
			SetCursor(plr, crew);
			// Transfer inventory if turned on.
			if (inventory_transfer)
				crew->GrabContents(clonk);
			break;
		}
		// Try to buy a crewmember at the base.
		var crew = base->~DoBuy(Clonk, plr, base->GetOwner(), clonk);
		if (crew && GetType(crew) == C4V_C4Object)
		{
			crew->Exit(0, base->GetDefHeight() / 2);
			SetCursor(plr, crew);
			// Transfer inventory if turned on.
			if (inventory_transfer)
				crew->GrabContents(clonk);
			break;
		}
	}	
	return true;
}

protected func Activate(int byplr)
{
	MessageWindow(this.Description, byplr);
	return true;
}

/* Scenario saving */

// Scenario saving
func SaveScenarioObject(props, ...)
{
	if (!inherited(props, ...)) return false;
	// Custom properties
	props->Remove("Name"); // updated by initialization
	props->Remove("Description"); // updated by initialization
	if (inventory_transfer) props->AddCall("InvenctoryTransfer", this, "SetInventoryTransfer", inventory_transfer);
	if (free_crew) props->AddCall("FreeCrew", this, "SetFreeCrew", free_crew);
	return true;
}


/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
