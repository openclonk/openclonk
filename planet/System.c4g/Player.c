/*-- Player related functions --*/

#strict 2

// Returns the player number of szPlrName, or none if there is no such player. (written by timi for CR/CE/CP)
global func GetPlayerByName(string szPlrName)
{
	// Loop through all players.
	var i = GetPlayerCount();
	while (i--)
		// Does the player's name match the one searched for?
		if (WildcardMatch(GetPlayerName(GetPlayerByIndex(i)), szPlrName))
			// It does -> return player number.
			return GetPlayerByIndex(i); 
	// There is no player with that name.
	return NO_OWNER;
}

// Returns the team number of szTeamName, or none if there is no such team.
global func GetTeamByName(string szTeamName)
{
	// Loop through all teams.
	var i = GetTeamCount();
	while (i--)
		// Does the team's name match the one searched for?
		if (WildcardMatch(GetTeamName(GetTeamByIndex(i)), szTeamName))
			// It does -> return team number.
			return GetTeamByIndex(i); 
	// There is no team with that name.
	return NO_OWNER;
}

// Returns the name of a player, including color markup using the player color.
global func GetTaggedPlayerName(int iPlr)
{
	var szPlrName = GetPlayerName(iPlr);
	if (!szPlrName) 
		return;
	var iPlrColor = MakeColorReadable(GetPlrColor(iPlr));
	var szTaggedPlrName = Format("<c %x>%s</c>", iPlrColor, szPlrName);
	return szTaggedPlrName;
}

// Returns the name of a team, including color markup using the team color.
global func GetTaggedTeamName(int iTeam)
{
	var szTeamName = GetTeamName(iTeam);
	if (!szTeamName) 
		return;
	var iTeamColor = MakeColorReadable(GetTeamColor(iTeam));
	var szTaggedTeamName = Format("<c %x>%s</c>", iTeamColor, szTeamName);
	return szTaggedTeamName;
}

// Brightens dark colors, to be readable on dark backgrounds.
global func MakeColorReadable(int dwColor)
{
	// Determine alpha.
	var a = ((dwColor >> 24 & 255) << 24);
	// Strip alpha.
	dwColor = dwColor & 16777215;
	// Calculate brightness: 50% red, 87% green, 27% blue (Max 164 * 255).
	var r = (dwColor >> 16 & 255), g = (dwColor >> 8 & 255), b = (dwColor & 255);
	var iLightness = r*50 + g*87 + b*27;
	// Above 55/164 (*255) is okay.
	if (iLightness < 14025) 
	{
		// Brighten up.
		var inc = (14025 - iLightness) / 164;
		dwColor = (Min(r+inc, 255)<<16) | (Min(g+inc, 255)<<8) | (Min(b+inc, 255));
		
	}
	// Add alpha. not needed at the moment.
	// dwColor = dwColor | a;
	return dwColor;
}
