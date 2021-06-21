/**
	Player.c
	Player and team related functions.
	
	@author timi, Maikel, Joern, Zapper, Randrian
*/

static const Player = new _Player
{
	// _Player contains all the scripting functions for players
	// Player is the prototype for all players and can be expanded
	// with data or functions.

	// Functions
	GetName = Global.GetName,  // No need to redefine this in the engine

	// Returns the name of a player, including color markup using the player color.
	// documented in /docs/sdk/script/fn
	GetTaggedName = func ()
	{
		var player_name = this->GetName();
		if (!player_name)
		{
			return;
		}
		var player_color = MakeColorReadable(this->GetColor());
		var tagged_player_name = Format("<c %x>%s</c>", player_color, player_name);
		return tagged_player_name;
	},

	SetWealth = func (int value)
	{
		this.Data.Wealth = BoundBy(value, 0, 1000000000);
		GameCallEx("OnWealthChanged", this);
		return true;
	},

	GetWealth = func ()
	{
		return this.Data.Wealth;
	},

	// Adds value to the account of iPlayer.
	// documented in /docs/sdk/script/fn
	DoWealth = func (int value)
	{
		return this->SetWealth(value + this->GetWealth());
	},
};

static const NO_PLAYER = NO_OWNER;

// Returns the player number of player_name, or none if there is no such player. (written by timi for CR/CE/CP)
// documented in /docs/sdk/script/fn
global func GetPlayerByName(string player_name)
{
	// Loop through all players.
	var i = GetPlayerCount();
	while (i--)
	{
		var player = GetPlayerByIndex(i);
		// Does the player's name match the one searched for?
		if (WildcardMatch(player->GetName(), player_name))
		{
			// It does -> return player number.
			return player;
		}
	}
	// There is no player with that name.
	return NO_PLAYER;
}

// Returns the team number of team_name, or none if there is no such team.
global func GetTeamByName(string team_name)
{
	// Loop through all teams.
	var i = GetTeamCount();
	while (i--)
	{
		var team = GetTeamByIndex(i);
		// Does the team's name match the one searched for?
		if (WildcardMatch(GetTeamName(team), team_name))
		{
			// It does -> return team number.
			return team;
		}
	}
	// There is no team with that name.
	return NO_OWNER;
}

// Returns the name of a team, including color markup using the team color.
global func GetTaggedTeamName(int team)
{
	var team_name = GetTeamName(team);
	if (!team_name)
	{
		return;
	}
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
	{
		if (GetPlayerByIndex(index)->GetTeam() == team)
		{
			count++;
		}
	}
	return count;
}

// Returns an array of player numbers of players of the specified type and in the specified team.
// Set either player_type or team to nil to ignore that constraint.
global func GetPlayers(int player_type, int team)
{
	var player_list = [];
	for (var index = 0; index < GetPlayerCount(player_type); index++)
	{
		var player = GetPlayerByIndex(index, player_type);
		if (team == nil || player->GetTeam() == team)
		{
			PushBack(player_list, player);
		}
	}
	return player_list;
}

// Returns the player number corresponding to the specified player ID.
global func GetPlayerByID(int player_id)
{
	for (var index = 0; index < GetPlayerCount(); index++)
	{
		var player = GetPlayerByIndex(index);
		if (player_id == player.ID)
			return player;
	}
	return NO_OWNER;
}

// checks whether two players are allied - that means they are not hostile and neither of them is NO_PLAYER
global func IsAllied(proplist player1, proplist player2, bool check_one_way_only /* whether to check the hostility only in one direction */)
{
	if (player1 == NO_PLAYER || player2 == NO_PLAYER)
	{
		return false;
	}
	return !(player1->Hostile(player2, check_one_way_only));
}

// Shows a message window to player for_player.
global func MessageWindow(string msg, proplist for_player, id icon, string caption)
{
	// Get icon.
	icon = icon ?? GetID();
	// Get caption.
	caption = caption ?? GetName();
	// Create msg window as regular text
	CustomMessage(Format("<c ffff00>%s</c>: %s", caption, msg), nil, for_player, 0, 150, nil, GetDefaultMenuDecoration(), icon, MSG_HCenter);
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
global func FindBase(proplist player, int index)
{
	return FindObjects(Find_Owner(player), Find_Func("IsBase"))[index];
}
