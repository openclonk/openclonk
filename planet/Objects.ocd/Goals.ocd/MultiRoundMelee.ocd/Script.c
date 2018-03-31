/*-- Multi-Round Melee --*/
/*	Originally part of HotIce, but now also used in other scenarios.

	Usage:
		In the scenario script, implement the following functions:

		InitializeRound():
			InitializeRound should create scenario objects for each round.

		InitPlayerRound(int plr, object crew):
			InitPlayerRound is called every round for each player and should equip
			and position their Clonks. Note that the players won't be able to
			control their Clonks until the round start countdown finishes.
			Check Goal_MultiRoundMelee->IsHandicapped(plr) to improve balance
			with unequal team sizes (see documentation below).

		StartRound():
			Called after the round start countdown finishes and the players can
			control their Clonks.

	@author Luchs
*/

#include Goal_Melee

/* Public Interface */

// SetRounds changes the number of remaining rounds. The number of rounds
// defaults to the `Rounds` scenario parameter if available.
public func SetRounds(int rounds)
{
	if (this == Goal_MultiRoundMelee) return FindObject(Find_ID(Goal_MultiRoundMelee))->SetRounds(rounds);
	remaining_rounds = rounds;
}

// IsHandicapped indicates whether the given player should receive a handicap.
// When playing with inbalanced teams, the goal randomly selects players to be
// handicapped so that the number of non-handicapped players is tha same for
// all teams.
public func IsHandicapped(int plr)
{
	if (this == Goal_MultiRoundMelee) return FindObject(Find_ID(Goal_MultiRoundMelee))->IsHandicapped(plr);
	return !!handicapped_players[plr];
}

/* Implementation */

local remaining_rounds, winners, check_victory_effect, gameover;
local handicapped_players;

protected func Initialize()
{
	// Don't allow creating the goal at runtime. This is important as the
	// engine will recreate goals during section changes, but we need to retain
	// all data.
	if (FrameCounter() > 1) return RemoveObject();

	remaining_rounds = SCENPAR.Rounds ?? 1;
	winners = [];
	InitializeRound();

	Scoreboard->Init([
		// Invisible team column for sorting players under their teams.
		{key = "team", title = "", sorted = true, desc = false, default = "", priority = 90},
		{key = "wins", title = "Wins", sorted = true, desc = true, default = 0, priority = 100},
		{key = "death", title = "", sorted = false, default = "", priority = 0},
	]);

	return inherited(...);
}

protected func InitializePlayer(int plr, int x, int y, object base, int team)
{
	// Add the player and their team to the scoreboard.
	Scoreboard->NewPlayerEntry(plr);
	Scoreboard->SetPlayerData(plr, "wins", "");
	Scoreboard->NewEntry(ScoreboardTeam(team), GetTeamName(team));
	Scoreboard->SetData(ScoreboardTeam(team), "team", "", ScoreboardTeam(team));
	Scoreboard->SetPlayerData(plr, "team", "", ScoreboardTeam(team) + 1);

	// Players joining at runtime will participate in the following round.
	// Should only happen if it's not game start, else Clonks would start stuck in a RelaunchContainer.
	if (FrameCounter() > 1) PutInRelaunchContainer(GetCrew(plr));

	return inherited(plr, x, y, base, team, ...);
}

protected func InitializePlayers()
{
	AssignHandicaps();
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		InitPlayerRound(plr);
	}
}

// InitPlayerRound initializes the round for the given player.
private func InitPlayerRound(int plr)
{
	// Unmark death on scoreboard.
	Scoreboard->SetPlayerData(plr, "death", "");
	// Players can scroll freely while waiting for the next round. Disable this now.
	SetPlayerViewLock(plr, true);
	// Disable the Clonk during the countdown.
	var crew = GetCrew(plr);
	crew->SetCrewEnabled(false);
	crew->SetComDir(COMD_Stop);

	// Let the scenario do its thing.
	Scenario->~InitPlayerRound(plr, crew);
}

// ResetRound resets the scenario, redrawing the map.
private func ResetRound()
{
	// Retrieve all Clonks.
	var clonks = [];
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember)))
	{
		var container = clonk->Contained();
		if (container)
		{
			clonk->Exit();
			container->RemoveObject();
		}
		else
		{
			// Players not waiting for a relaunch get a new Clonk to prevent
			// status effects from carrying over to the next round.
			var new_clonk = CreateObject(clonk->GetID(), 0, 0, clonk->GetOwner());
			new_clonk->GrabObjectInfo(clonk);
			clonk = new_clonk;
		}
		PushBack(clonks, clonk);
		clonk->SetObjectStatus(C4OS_INACTIVE);
	}

	// Clear and redraw the map while retaining the goal.
	SetObjectStatus(C4OS_INACTIVE);
	LoadScenarioSection("main");
	SetObjectStatus(C4OS_NORMAL);

	InitializeRound();
	AssignHandicaps();
	// Re-enable the players.
	for (var clonk in clonks)
	{
		clonk->SetObjectStatus(C4OS_NORMAL);
		SetCursor(clonk->GetOwner(), clonk);
		// Select the first item. This fixes item ordering.
		clonk->SetHandItemPos(0, 0);
		InitPlayerRound(clonk->GetOwner());
	}

	// Fix goal icon in the HUD.
	NotifyHUD();
}

local CheckVictory = new Effect
{
	goal = nil,

	Construction = func(object g)
	{
		goal = g;
	},

	Timer = func()
	{
		var find_living = Find_And(Find_OCF(OCF_CrewMember), Find_NoContainer());
		var clonk = FindObject(find_living);
		var msg;
		if (!clonk)
		{
			// Stalemate!
			msg = "$Stalemate$";
			Log(msg);
			goal->ResetRound();
		}
		else if (!FindObject(find_living, Find_Hostile(clonk->GetOwner())))
		{
			// We have a winner!
			var team = GetPlayerTeam(clonk->GetOwner());
			PushBack(goal.winners, team);
			// Announce the winning team.
			msg = Format("$WinningTeam$", GetTeamPlayerNames(team));
			Log(msg);

			// Update the scoreboard.
			goal->UpdateScoreboardWins(team);

			// The leading team has to win the last round.
			if (--goal.remaining_rounds > 0 || goal->GetLeadingTeam() != team)
			{
				var msg2 = goal->CurrentRoundStr();
				Log(msg2);
				msg = Format("%s|%s", msg, msg2);
				goal->ResetRound();
			}
			else
			{
				goal->EliminateLosers();
			}
		}
		// Switching scenario sections makes the Log() messages hard to see, so announce them using a message as well.
		CustomMessage(msg);
		// Go to sleep again.
		this.Interval = 0;
		return FX_OK;
	},

	// GetTeamPlayerNames returns a list of colored player names, for example
	// "Sven2, Maikel, Luchs"
	GetTeamPlayerNames = func(int team)
	{
		var str = "";
		for (var i = 0; i < GetPlayerCount(); i++)
		{
			var plr = GetPlayerByIndex(i);
			if (GetPlayerTeam(plr) == team)
			{
				var comma = "";
				if (str != "") comma = ", ";
				str = Format("%s%s<c %x>%s</c>", str, comma, GetPlayerColor(plr), GetPlayerName(plr));
			}
		}
		return str;
	},
};

private func CurrentRoundStr()
{
	if (remaining_rounds == 1)
		return "$LastRound$";
	else if (remaining_rounds > 1)
		return Format("$RemainingRounds$", remaining_rounds);
	else if (GetLeadingTeam() == nil)
		return "$Tiebreak$";
	else
		return "$BonusRound$";
}

private func InitializeRound()
{
	// Checking for victory: Only active after a Clonk dies.
	if (!check_victory_effect)
		check_victory_effect = CreateEffect(CheckVictory, 1, 0, this);

	// Now let the scenario do its thing.
	Scenario->~InitializeRound();

	// The game starts after a delay to ensure that everyone is ready.
	GUI_Clock->CreateCountdown(3);

	return true;
}

protected func OnCountdownFinished() // called by the round start countdown
{
	// Re-enable all Clonks.
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember)))
	{
		clonk->SetCrewEnabled(true);
		SetCursor(clonk->GetOwner(), clonk);
	}
	Scenario->~StartRound();
}

private func PutInRelaunchContainer(object clonk)
{
	var plr = clonk->GetOwner();
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, plr);
	// We just use the relaunch object as a dumb container.
	clonk->Enter(relaunch);
	// Allow scrolling around the landscape.
	SetPlayerViewLock(plr, false);
}

protected func OnClonkDeath(object clonk)
{
	var plr = clonk->GetOwner();
	// Mark death on scoreboard.
	Scoreboard->SetPlayerData(plr, "death", "{{Scoreboard_Death}}");
	// Skip eliminated players, NO_OWNER, etc.
	if (GetPlayerName(plr)) 
	{
		var crew = CreateObject(Clonk, 0, 0, plr);
		crew->MakeCrewMember(plr);
		PutInRelaunchContainer(crew);
	}

	// Check for victory after three seconds to allow stalemates.
	if (!gameover)
		check_victory_effect.Interval = check_victory_effect.Time + 36 * 3;
}

// GetTeamPlayers returns an array of team -> number of players in team.
private func GetTeamPlayers()
{
	var result = CreateArray(GetTeamCount() + 1);
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i), team = GetPlayerTeam(plr);
		SetLength(result, Max(team + 1, GetLength(result)));
		result[team] = result[team] ?? [];
		PushBack(result[team], plr);
	}
	return result;
}

func _MinSize(int a, array b) { if (b == nil) return a; else return Min(a, GetLength(b)); }

// Assigns handicaps so that the number of not-handicapped players is the same for all teams.
func AssignHandicaps()
{
	handicapped_players = CreateArray(GetPlayerCount());
	var teams = GetTeamPlayers();
	var smallest_size = Reduce(teams, this._MinSize, ~(1<<31));
	for (var team in teams) if (team != nil)
	{
		var to_handicap = GetLength(team) - smallest_size;
		while (GetLength(team) > to_handicap)
			RemoveArrayIndexUnstable(team, Random(GetLength(team)));
		for (var plr in team)
		{
			SetLength(handicapped_players, Max(plr + 1, GetLength(handicapped_players)));
			handicapped_players[plr] = true;
		}
	}
}

// GetLeadingTeam returns the team which won the most rounds, or nil if there is a tie.
private func GetLeadingTeam()
{
	var teams = [], winning_team = winners[0];
	for (var w in winners)
	{
		teams[w] += 1;
		if (teams[w] > teams[winning_team])
			winning_team = w;
	}
	// Detect a tie.
	for (var i = 0; i < GetLength(teams); i++)
	{
		if (i != winning_team && teams[i] == teams[winning_team])
			return nil;
	}
	return winning_team;
}

private func EliminateLosers()
{
	gameover = true;
	// Determine the winning team.
	var winning_team = GetLeadingTeam();
	// Eliminate everybody who isn't on the winning team.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (GetPlayerTeam(plr) != winning_team)
			EliminatePlayer(plr);
	}
	// The included melee goal will end the scenario.
}

/* Scoreboard */

private func ScoreboardTeam(int team) { return team * 100; }

// GetTeamWins returns how many rounds that team has won.
private func GetTeamWins(int team)
{
	var wins = 0;
	for (var w in winners)
		if (w == team)
			wins++;
	return wins;
}

private func UpdateScoreboardWins(int team)
{
	var wins = GetTeamWins(team);
	Scoreboard->SetData(ScoreboardTeam(team), "wins", wins, wins);
	// We have to update each player as well to make the sorting work.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (GetPlayerTeam(plr) == team)
		{
			Scoreboard->SetPlayerData(plr, "wins", "", wins);
		}
	}
}

/* Goal interface */

public func GetDescription(int plr)
{
	// Count active enemy clonks.
	var hostile_count = ObjectCount(Find_OCF(OCF_CrewMember), Find_NoContainer(), Find_Hostile(plr));
	var message;
	if (!hostile_count)
		message = "$MsgGoalFulfilled$";
	else
		message = Format("$MsgGoalUnfulfilled$", hostile_count);

	// Also report the remaining rounds.
	message = Format("%s|%s", message, CurrentRoundStr());

	return message;
}

public func GetShortDescription(int plr)
{
	return CurrentRoundStr();
}

local Name = "$Name$";
