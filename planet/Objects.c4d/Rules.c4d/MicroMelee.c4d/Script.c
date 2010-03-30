/*
	Micromeleee
	Author: Maikel
	
	Premade goal for simple micro melees.

	Callbacks made to scenario script:
		* OnPlrRelaunch(int plr) made when the player is relaunched and at game start plr init.
		* OnPlrKill(int killer, int victim) made when the player has been killed.
		* RelaunchCount() should return the number of relaunches.
		*

*/

// Predefined constants.
static const MIME_RelaunchCount = 5; // Number of relaunches.
static const MIME_KillsToRelaunch = 4; // Number of kills one needs to make before gaining a relaunch.
static const MIME_ShowBoardTime = 5; // Duration in seconds the scoreboard will be shown to a player on an event.
static const MIME_ScoreString = "MIME_Score";

static const MIME_ScoreDif = 2500; // Ab dieser Differenz halbiert sich der Punktebonus
static const MIME_ScoreBase = 0; // Startpunkte
static const MIME_ScoreGain	= 50; // Punkte pro getöteten Clonk bei gleichem Punktestand
static const MIME_ScoreGainMin = 1; // Minimale Punkte pro Kill
static const MIME_ScoreGainMax = 200; // Maximale Punkte pro Kill

// Lists used to keep track of statistics.
local relaunch_list; // Array for the player's relaunches.
local death_list; // Array for the player's deaths.
local kill_list; // Array for the player's kills.
local streak_list; // Array for the player's kill streak.
local score_list; // Array for the player's score.

protected func Initialize()
{
	// Init statistics lists.
	relaunch_list = [];
	death_list = [];
	kill_list = [];
	streak_list = [];
	score_list = [];
	// Create melee goal if there isn't any.
	if (!ObjectCount(Find_ID(Goal_Melee)))
		CreateObject(Goal_Melee, 0, 0, NO_OWNER);
	return;
}

/*-- Scenario parts --*/

private func GetRelaunchCount()
{
	var relaunch_cnt = GameCall("RelaunchCount");
	if (relaunch_cnt)
		return relaunch_cnt; 
	return MIME_RelaunchCount;
}

/*-- Player section --*/

protected func InitializePlayer(int plr, int x, int y, object base, int team)
{
	// Init statistics lists for plr.
	relaunch_list[plr] = GetRelaunchCount();
	death_list[plr] = 0;
	kill_list[plr] = 0;
	streak_list[plr] = 0;
	score_list[plr] = 0;
	if (GetPlrExtraData(plr, MIME_ScoreString))
		score_list[plr] = GetPlrExtraData(plr, MIME_ScoreString);

	// Set scoreboard for plr.
	InitScoreboard(plr);
	UpdateScoreboard(plr);
	// Join plr.
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("OnPlrRelaunch", plr);
	return;
}

protected func RelaunchPlayer(int plr, int killer)
{
	GiveDeath(plr);
	LogKill(killer, plr);
	GiveKill(killer, plr);
	
	if (relaunch_list[plr] < 0)
		return EliminatePlayer(plr);
	UpdateScoreboard(plr, true);
	var clonk = CreateObject(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	SelectCrew(plr, clonk, true);
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("OnPlrRelaunch", plr);
	return;
}

protected func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var x, y;
	FindRelaunchPos(plr, x, y);
	clonk->SetPosition(x, y);
	return;
}

private func FindRelaunchPos(int plr, int &x, int &y)
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
		x = tx;
		y = ty;
		break;
	}
	return true;
}


protected func RemovePlayer(int plr)
{
	// TODO

	return;
}

/*-- Statistics --*/

private func GiveKill(int killer, int victim)
{
	if (killer == victim || !GetPlayerName(killer)) // No killer.
		return;
	if (GetPlayerTeam(killer) && GetPlayerTeam(killer) == GetPlayerTeam(victim)) // Team kill.
		return GiveScore(killer, victim, true);
	// Regular kill.
	GiveScore(killer, victim);
	kill_list[killer]++;
	streak_list[killer]++;
	if (!(kill_list[killer] % MIME_KillsToRelaunch))
		relaunch_list[killer]++;

	GameCall("OnPlrKill", killer, victim);

	UpdateScoreboard(killer, true);
	return;
}

private func GiveScore(int killer, int victim, bool teamkill)
{
	var score_dif = score_list[killer] - score_list[victim];
	var score_factor;
	if (score_dif > 0)
		score_factor = 1000 * MIME_ScoreDif / (score_dif + MIME_ScoreDif);
	else
		score_factor = 1000 * (MIME_ScoreDif - score_dif) / MIME_ScoreDif;

	var score_change = BoundBy(score_factor * MIME_ScoreGain / 1000, MIME_ScoreGainMin, MIME_ScoreGainMax);
	score_list[victim] = BoundBy(score_list[victim] - score_change, 0, 100 * MIME_ScoreDif);
	if (teamkill)
		score_change = -score_change;
	score_list[killer] = BoundBy(score_list[killer] + score_change, 0, 100 * MIME_ScoreDif);	
	
	// Update score to player data.
	SetPlrExtraData(killer, MIME_ScoreString, score_list[killer]);
	SetPlrExtraData(victim, MIME_ScoreString, score_list[victim]);
	return;
}

private func GiveDeath(int plr)
{
	relaunch_list[plr]--;
	death_list[plr]++;
	streak_list[plr] = 0;
	return;
}

private func LogKill(int killer, int victim)
{
	var msg;
	// Determine kill-part.
	if (killer == victim || !GetPlayerName(killer)) // Suicide or unknown killer.
		msg = Format("$MsgSelfKill$", GetPlayerName(victim));
	else if (GetPlayerTeam(killer) && GetPlayerTeam(killer) == GetPlayerTeam(victim)) // Teamkill.
		msg = Format("$MsgTeamKill$", GetPlayerName(victim), GetPlayerName(killer));
	else // Normal kill.
		msg = Format("$MsgKill$", GetPlayerName(victim), GetPlayerName(killer));
	// Determine relaunch part.
	if (relaunch_list[victim] < 0) // Player eliminated.
		msg = Format("%s %s", msg, "$MsgFail$");
	else if (relaunch_list[victim] == 0) // Last relaunch.
		msg = Format("%s %s", msg, "$MsgRelaunch0$");
	else if (relaunch_list[victim] == 1) // One relaunch remaining.
		msg = Format("%s %s", msg, "$MsgRelaunch1$");
	else // Multiple relaunches remaining.
		msg = Format("%s %s", msg, Format("$MsgRelaunchX$", relaunch_list[victim]));
	// Log message.
	Log(msg);
	return;
}

/*-- Scoreboard --*/

static const SBRD_Relaunch = 0;
static const SBRD_Death = 1;
static const SBRD_Kill = 2;
static const SBRD_Streak = 3;
static const SBRD_Score = 4;

private func InitScoreboard(int plr)
{
	var plrid = GetPlayerID(plr);
	// The above row.
	SetScoreboardData(SBRD_Caption, SBRD_Caption, "Scoreboard", SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_Relaunch, "{{MicroMelee_Relaunch}}", SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_Death, "{{MicroMelee_Death}}", SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_Kill, "{{MicroMelee_Kill}}", SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_Streak, "{{MicroMelee_Streak}}", SBRD_Caption);
	SetScoreboardData(SBRD_Caption, SBRD_Score, "{{MicroMelee_Relaunch}}", SBRD_Caption);
	// Player name.
	SetScoreboardData(plrid, SBRD_Caption, GetTaggedPlayerName(plr), plrid);
	return;
}

private func UpdateScoreboard(int plr, bool show)
{
	var plrid = GetPlayerID(plr);
	// The scores.
	SetScoreboardData(plrid, SBRD_Relaunch, Format("%d", relaunch_list[plr]), relaunch_list[plr]);
	SetScoreboardData(plrid, SBRD_Death, Format("%d", death_list[plr]), death_list[plr]);
	SetScoreboardData(plrid, SBRD_Kill, Format("%d", kill_list[plr]), kill_list[plr]);
	SetScoreboardData(plrid, SBRD_Streak, Format("%d", streak_list[plr]), streak_list[plr]);
	SetScoreboardData(plrid, SBRD_Score, Format("%d", score_list[plr]), score_list[plr]);

	// Show scoreboard.
	if(show)
	{
		DoScoreboardShow(1, plr+1);
		Schedule(Format("DoScoreboardShow(-1, %d)", plr+1), 35 * MIME_ShowBoardTime);
	}
	return;
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
