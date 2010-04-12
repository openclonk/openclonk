/*--
		Last Man Standing
		Author: Maikel
	
		Premade goal for simple melees with relaunches.
		Callbacks made to scenario script:
			* OnPlrRelaunch(int plr) made when the player is relaunched and at game start plr init.
			* RelaunchCount() should return the number of relaunches.
			* KillsToRelaunch() should return how many kills will earn the player an extra relaunch.
--*/

// Based on the regular melee goal.
#include Goal_Melee

// Include modular scoreboard columns, notice the reverse order.
#include Scoreboard_KillStreak
#include Scoreboard_Kill
#include Scoreboard_Death
#include Scoreboard_Relaunch
#include Scoreboard_Player

// Some static constants.
static const MIME_RelaunchCount = 5; // Number of relaunches.
static const MIME_KillsToRelaunch = 4; // Number of kills one needs to make before gaining a relaunch.
static const MIME_ShowBoardTime = 5; // Duration in seconds the scoreboard will be shown to a player on an event.

protected func Initialize()
{
	// Create melee goal if there isn't any.
	//if (!ObjectCount(Find_ID(Goal_Melee)))
	//	CreateObject(Goal_Melee, 0, 0, NO_OWNER);
	return _inherited(...);
}

/*-- Scenario parts --*/

private func RelaunchCount()
{
	var relaunch_cnt = GameCall("RelaunchCount");
	if (relaunch_cnt)
		return relaunch_cnt; 
	return MIME_RelaunchCount;
}

private func KillsToRelaunch()
{
	var kills_to_relaunch = GameCall("KillsToRelaunch");
	if (kills_to_relaunch)
		return kills_to_relaunch; 
	return MIME_KillsToRelaunch;
}

/*-- Player section --*/

protected func InitializePlayer(int plr)
{
	// Join plr.
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("OnPlrRelaunch", plr);
	return _inherited(plr, ...);
}

protected func RelaunchPlayer(int plr, int killer)
{
	_inherited(plr, killer, ...);
	if (GetRelaunchCount(plr) < 0)
		return EliminatePlayer(plr);
	
	// Log messages.
	LogKill(killer, plr);
	
	// Kill bonus: 1 extra relaunch per MIME_KillsToRelaunch kills.
	// Only if killer exists and has not committed suicide.
	if (plr != killer && GetPlayerName(killer))
		// Only if killer and victim are on different teams.
		if (!(GetPlayerTeam(killer) && GetPlayerTeam(killer) == GetPlayerTeam(plr)))
			if (KillsToRelaunch() && !(GetKillCount(killer) % KillsToRelaunch()) && GetKillCount(killer))
				DoRelaunchCount(killer, 1);
				
	var clonk = CreateObject(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	SelectCrew(plr, clonk, true);
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("OnPlrRelaunch", plr);
	// Show scoreboard for a while.
	DoScoreboardShow(1, plr + 1);  
	Schedule(Format("DoScoreboardShow(-1, %d)", plr + 1), 35 * MIME_ShowBoardTime);
	return; // _inherited(plr, killer, ...);
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
	return _inherited(plr, ...);
}

/*-- Statistics --*/

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
	if (GetRelaunchCount(victim) < 0) // Player eliminated.
		msg = Format("%s %s", msg, "$MsgFail$");
	else if (GetRelaunchCount(victim) == 0) // Last relaunch.
		msg = Format("%s %s", msg, "$MsgRelaunch0$");
	else if (GetRelaunchCount(victim) == 1) // One relaunch remaining.
		msg = Format("%s %s", msg, "$MsgRelaunch1$");
	else // Multiple relaunches remaining.
		msg = Format("%s %s", msg, Format("$MsgRelaunchX$", GetRelaunchCount(victim)));
	// Log message.
	Log(msg);
	return;
}

protected func Definition(def)
{
	SetProperty("Name", "$Name$", def);
}
