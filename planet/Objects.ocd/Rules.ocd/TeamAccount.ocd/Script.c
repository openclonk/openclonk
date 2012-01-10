/**
	Team account
	Allied players share a common account.
	
	@author Maikel
*/


protected func Initialize()
{
	// Give all existing players an account.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		AddEffect("IntTeamAccount", nil, 100, 2, this, nil, plr);		
	}
	return;
}

protected func InitializePlayer(int plr)
{
	// Give new player an account.
	var account = AddEffect("IntTeamAccount", nil, 100, 2, this, nil, plr);
	// Store wealth.
	var wealth = GetWealth(plr);
	// Add wealth of allies to player's account.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var allied_plr = GetPlayerByIndex(i);
		EffectCall(nil, account, "Change", allied_plr, GetWealth(allied_plr));
	}
	// Add original wealth to all allies.
	var i = 0, effect;
	while (effect = GetEffect("IntTeamAccount", nil, i, 0))
	{
		EffectCall(nil, effect, "Change", plr, wealth);
		i++;
	}
	return;
}

protected func RemovePlayer(int plr)
{
	// Nothing should happen here.
	return;
}

protected func OnTeamSwitch(int player, int new_team, int old_team)
{
	// Remove player from old team, i.e. substract his fair share.
	var count = 0;
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		if (!Hostile(player, GetPlayerByIndex(i)))
			count++;		
	}
	var share = GetWealth(player) / count;

	// Add player to new team, i.e. add his wealth.

	return;
}

protected func OnHostilityChange(int plr, int plr2, bool hostility, bool old_hostility)
{
	// TODO: Implement
	return;
}

protected func Activate(int byplr)
{
	MessageWindow("$Description$", byplr);
	return;
}


/*-- Team Account --*/
// Every player gets an effect which stores the current wealth.

protected func FxIntTeamAccountStart(object target, proplist effect, int temporary, int plr)
{
	// Effect start: store player number and player's wealth.
	effect.Plr = plr;
	effect.Wealth = GetWealth(plr);
	return 1;
}

protected func FxIntTeamAccountTimer(object target, proplist effect, int time)
{
	// Remove effect if player is non-existant.
	if (!GetPlayerName(effect.Plr))
		return -1;
	// Register wealth change.
	var change = GetWealth(effect.Plr) - effect.Wealth;
	if (change != 0)
	{
		// Reset effect Wealth.
		effect.Wealth = GetWealth(effect.Plr);
		// Apply wealth change to allied accounts.
		var i = 0, account;
		while (account = GetEffect("IntTeamAccount", target, i, 0))
		{
			EffectCall(nil, account, "Change", effect.Plr, change);
			i++;
		}			
	}
	return 1;
}

protected func FxIntTeamAccountStop(object target, proplist effect, int reason, bool temporary)
{

	return 1;
}

protected func FxIntTeamAccountChange(object target, proplist effect, int from_plr, int change)
{
	// Only accept changes from allied players.
	if (effect.Plr == from_plr || Hostile(effect.Plr, from_plr))
		return 1;
	// Change both wealth and effect var.
	DoWealth(effect.Plr, change);
	effect.Wealth += change;	
	return 1;
}

/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
