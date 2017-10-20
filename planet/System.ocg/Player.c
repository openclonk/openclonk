/**
	Player.c
	Player and team related functions.
	
	@author timi, Maikel, Joern, Zapper, Randrian
*/

// Returns the player number of plr_name, or none if there is no such player. (written by timi for CR/CE/CP)
// documented in /docs/sdk/script/fn
global func GetPlayerByName(string plr_name)
{
	// Loop through all players.
	var i = GetPlayerCount();
	while (i--)
		// Does the player's name match the one searched for?
		if (WildcardMatch(GetPlayerName(GetPlayerByIndex(i)), plr_name))
			// It does -> return player number.
			return GetPlayerByIndex(i);
	// There is no player with that name.
	return NO_OWNER;
}

// Returns the team number of team_name, or none if there is no such team.
global func GetTeamByName(string team_name)
{
	// Loop through all teams.
	var i = GetTeamCount();
	while (i--)
		// Does the team's name match the one searched for?
		if (WildcardMatch(GetTeamName(GetTeamByIndex(i)), team_name))
			// It does -> return team number.
			return GetTeamByIndex(i);
	// There is no team with that name.
	return NO_OWNER;
}

// Returns the name of a player, including color markup using the player color.
// documented in /docs/sdk/script/fn
global func GetTaggedPlayerName(int plr)
{
	var plr_name = GetPlayerName(plr);
	if (!plr_name)
		return;
	var plr_color = MakeColorReadable(GetPlayerColor(plr));
	var tagged_plr_name = Format("<c %x>%s</c>", plr_color, plr_name);
	return tagged_plr_name;
}

// Returns the name of a team, including color markup using the team color.
global func GetTaggedTeamName(int team)
{
	var team_name = GetTeamName(team);
	if (!team_name)
		return;
	var team_color = MakeColorReadable(GetTeamColor(team));
	var tagged_team_name = Format("<c %x>%s</c>", team_color, team_name);
	return tagged_team_name;
}

// Brightens dark colors, to be readable on dark backgrounds.
global func MakeColorReadable(int color)
{
	// Determine alpha. Not needed at the moment
	//var a = ((color >> 24 & 255) << 24);
	// Strip alpha.
	color = color & 16777215;
	// Calculate brightness: 50% red, 87% green, 27% blue (Max 164 * 255).
	var r = (color >> 16 & 255), g = (color >> 8 & 255), b = (color & 255);
	var lightness = r * 50 + g * 87 + b * 27;
	// Above 55 / 164 (*255) is okay.
	if (lightness < 14025)
	{
		// Brighten up.
		var inc = (14025 - lightness) / 164;
		color = (Min(r + inc, 255) << 16) | (Min(g + inc, 255) << 8) | (Min(b + inc, 255));
		
	}
	// Add alpha. not needed at the moment.
	// color = color | a;
	return color;
}

// Returns the number of players in the specified team.
global func GetPlayerInTeamCount(int team)
{
	var count = 0;
	for (var index = 0; index < GetPlayerCount(); index++)
		if (GetPlayerTeam(GetPlayerByIndex(index)) == team)
			count++;
	return count;
}

// Returns an array of player numbers of players of the specified type and in the specified team.
// Set either player_type or team to nil to ignore that constraint.
global func GetPlayers(int player_type, int team)
{
	var plr_list = [];
	for (var index = 0; index < GetPlayerCount(player_type); index++)
	{
		var plr = GetPlayerByIndex(index, player_type);
		if (team == nil || GetPlayerTeam(plr) == team)
			PushBack(plr_list, plr);
	}
	return plr_list;
}

// Returns the player number corresponding to the specified player ID.
global func GetPlayerByID(int plr_id)
{
	for (var index = 0; index < GetPlayerCount(); index++)
	{
		var plr = GetPlayerByIndex(index);
		if (plr_id == GetPlayerID(plr))
			return plr;
	}
	return NO_OWNER;
}

// Adds value to the account of iPlayer.
// documented in /docs/sdk/script/fn
global func DoWealth(int plr, int value)
{
	return SetWealth(plr, value + GetWealth(plr));
}

// checks whether two players are allied - that means they are not hostile and neither of them is NO_OWNER
global func IsAllied(int plr1, int plr2, bool check_one_way_only /* whether to check the hostility only in one direction */)
{
	if (plr1 == NO_OWNER)
		return false;
	if (plr2 == NO_OWNER)
		return false;
	return !Hostile(plr1, plr2, check_one_way_only);
}

// Shows a message window to player for_plr.
global func MessageWindow(string msg, int for_plr, id icon, string caption)
{
	// Get icon.
	if (!icon)
		icon = GetID();
	// Get caption.
	if (!caption)
		caption = GetName();
	// Create msg window as regular text
	CustomMessage(Format("<c ffff00>%s</c>: %s", caption, msg), nil, for_plr, 0,150, nil, GetDefaultMenuDecoration(), icon, MSG_HCenter);
	return true;
}

// Returns the default menu decoration used in most places.
// The return value should be a definition, e.g. GUI_MenuDeco.
global func GetDefaultMenuDecoration()
{
	return _inherited(...);
}

// Find a base of the given player. Use index to search through all bases.
// documented in /docs/sdk/script/fn
global func FindBase(int plr, int index)
{
	return FindObjects(Find_Owner(plr), Find_Func("IsBase"))[index];
}
