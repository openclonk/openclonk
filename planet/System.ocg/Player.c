/*--
		Player.c
		Authors: timi, Maikel

		Player and team related functions.
--*/


// Returns the player number of plr_name, or none if there is no such player. (written by timi for CR/CE/CP)
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
	// Determine alpha.
	var a = ((color >> 24 & 255) << 24);
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
