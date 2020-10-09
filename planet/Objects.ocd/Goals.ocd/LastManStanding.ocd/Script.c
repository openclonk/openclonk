/*--
		Last Man Standing
		Author: Maikel
	
		Premade goal for simple melees with relaunches.
		Callbacks made to scenario script:
			* KillsToRelaunch() should return how many kills will earn the player an extra relaunch.
--*/

// Based on the regular melee goal.
#include Goal_Melee

// Include modular scoreboard columns, notice the reverse order.
#include Scoreboard_KillStreak
#include Scoreboard_Kill
#include Scoreboard_Relaunch

// Some rule default values
local DefaultKillsToRelaunch = 4; // Number of kills one needs to make before gaining a relaunch.
local ShowBoardTime = 5; // Duration in seconds the scoreboard will be shown to a player on an event.

protected func Initialize()
{
	// Create melee goal if there isn't any.
	//if (!ObjectCount(Find_ID(Goal_Melee)))
	//	CreateObject(Goal_Melee, 0, 0, NO_OWNER);
	return _inherited(...);
}

/*-- Scenario callbacks --*/

private func KillsToRelaunch()
{
	var kills_to_relaunch = GameCall("KillsToRelaunch");
	if (kills_to_relaunch != nil)
		return kills_to_relaunch;
	return DefaultKillsToRelaunch;
}

/*-- Player section --*/

protected func InitializePlayer(int plr)
{
	_inherited(plr, ...);
}

protected func OnClonkDeath(object clonk, int killer)
{
	var plr = clonk->GetOwner();
	_inherited(clonk, killer, ...);
	// the kill logs rule cares about logging the respawn
	// ..
	
	// Kill bonus: 1 extra relaunch per KillsToRelaunch kills.
	// Only if killer exists and has not committed suicide.
	if (plr != killer && GetPlayerName(killer))
		// Only if killer and victim are on different teams.
		if (!(GetPlayerTeam(killer) && GetPlayerTeam(killer) == GetPlayerTeam(plr)))
			if (KillsToRelaunch() && !(GetKillCount(killer) % KillsToRelaunch()) && GetKillCount(killer))
			{
				GetRelaunchRule()->DoPlayerRelaunchCount(killer, 1);
				Log("$MsgRelaunchGained$", GetPlayerName(killer));
			}
				
	// Show scoreboard for a while.
	DoScoreboardShow(1, plr + 1);
	Schedule(this, Format("DoScoreboardShow(-1, %d)", plr + 1), 35 * ShowBoardTime);
	return;
}
protected func RemovePlayer(int plr)
{
	return _inherited(plr, ...);
}


local Name = "$Name$";
