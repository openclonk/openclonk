/**
	Team account
	Allied players share a common account.
	
	@author Maikel
*/


protected func Initialize()
{
	// Under no circumstance there may by multiple copies of this rule.
	if (ObjectCount(Find_ID(Rule_TeamAccount)) > 1)
		return RemoveObject();
	return;
}

protected func Definition(def type)
{
	Player.SetWealth_inherited = Player.SetWealth;
	Player.SetWealth = type.SetWealth;
}

// Only SetWealth needs to be overloaded, DoWealth just uses that.
func SetWealth(int wealth)
{
	// Only if team account rule is activated.
	if (!FindObject(Find_ID(Rule_TeamAccount)))
		return this->SetWealth_inherited(wealth, ...);
		
	// Also set wealth of all allies.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var to_plr = GetPlayerByIndex(i);
		if (to_plr != this && !to_plr->Hostile(this))
			to_plr->SetWealth_inherited(wealth, ...);
	}

	return this->SetWealth_inherited(wealth, ...);
}

protected func InitializePlayer(proplist plr)
{
	// Find an ally and add this wealth.
	for (var to_plr in GetPlayers())
	{
		if (to_plr != plr && !to_plr->Hostile(plr))
		{
			to_plr->DoWealth(plr->GetWealth());
			break;
		}
	}	
	return;
}

protected func RemovePlayer(proplist plr)
{
	// Nothing should happen here.
	return;
}

protected func OnTeamSwitch(proplist player, int new_team, int old_team)
{
	// Remove player from old team, i.e. substract his fair share.
	var count = 0;
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		if (!player->Hostile(GetPlayerByIndex(i)))
			count++;		
	}
	//var share = player->GetWealth() / count;

	// Add player to new team, i.e. add his wealth.
	// TODO Implement
	return;
}

protected func OnHostilityChange(proplist plr, proplist plr2, bool hostility, bool old_hostility)
{
	// TODO: Implement
	return;
}

public func Activate(proplist by_plr)
{
	MessageWindow(this.Description, by_plr);
	return true;
}

/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
