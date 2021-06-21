/*-- Melee --*/

#include Library_Goal

func MakeHostileToAll(proplist newplr, int team)
{
	// If the player is in a team, don't change hostility.
	if (team) return;

	// Otherwise, make all other players enemies.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (plr == newplr) continue;
		
		newplr->SetHostility(plr, true, true);
		plr->SetHostility(newplr, true, true);
	}
}

protected func InitializePlayer(proplist newplr, int x, int y, object base, int team)
{
	MakeHostileToAll(newplr, team);
	return inherited(newplr, x, y, base, team, ...);
}

private func CheckTeamHostile(proplist plr1, proplist plr2)
{
	var team1 = plr1->GetTeam();
	if (team1 != plr2->GetTeam())
		return true;
	if (team1)
		return false;
	return plr1->Hostile(plr2);
}

public func IsFulfilled()
{
	// If Teams.txt-Teams still need to be chosen, the goal cannot be fulfilled.
	if (GetPlayerByIndex()->GetTeam() == -1) return;

	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		// Compare with other players.
		for (var j = i + 1; j < GetPlayerCount(); j++)
		{
			var plr2cmp = GetPlayerByIndex(j);
			// Still enemy players out there?
			if (CheckTeamHostile(plr, plr2cmp) ) return false;
		}
	}
	
	// No enemy players, goal fulfilled.
	return true;
}

public func GetDescription(proplist plr)
{
	// Count enemy players.
	var hostile_count;
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var byplr = GetPlayerByIndex(i);
		if (byplr == plr)
			continue;
		if (byplr->Hostile(plr) )
			hostile_count++;
	}
	
	var message;
	
	// Output
	if (!hostile_count)
		message = "$MsgGoalFulfilled$";
	else
		message = Format("$MsgGoalUnfulfilled$", hostile_count);
	return message;
}

public func Activate(proplist byplr)
{
	// Count enemy players.
	var hostile_count;
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (plr == byplr)
			continue;
		if (plr->Hostile(byplr) )
			hostile_count++;
	}
	
	// Output
	if (!hostile_count)
		MessageWindow("$MsgGoalFulfilled$", byplr);
	else
		MessageWindow(Format("$MsgGoalUnfulfilled$", hostile_count), byplr);
	return;
}

public func GetShortDescription(proplist plr)
{
	return ""; // TODO
}

local Name = "$Name$";
