/*-- 
	Ruby hunt
	Author: Sven2
	
	Mine one ruby and return to start
--*/


#include Library_Goal

local goal_rect, has_winner;
local is_cooperative;

protected func Initialize()
{
	// default to race mode
	is_cooperative = false;
	return inherited(...);
}

func SetGoalRect(r)
{
	goal_rect = r;
	return true;
}

func SetCooperative(bool to_val)
{
	is_cooperative = to_val;
	return true;
}


/* Scenario saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (goal_rect) props->AddCall("Goal", this, "SetGoalRect", goal_rect);
	if (is_cooperative) props->AddCall("Goal", this, "SetCooperative", is_cooperative);
	return true;
}


/*-- Goal interface --*/

// The goal is fulfilled if a ruby is in the goal rectangle
public func IsFulfilled()
{
	var winner = NO_OWNER, winners, winner_teams;
	if (has_winner) return true;
	for (var ruby in FindObjects(Find_InRect(goal_rect.x, goal_rect.y, goal_rect.wdt, goal_rect.hgt), Find_ID(Ruby)))
	{
		if (ruby->Contained()) winner = ruby->Contained()->GetOwner();
		if (winner == NO_OWNER) winner = ruby->GetController();
		if (winner == NO_OWNER) continue;
		if (!winners) winners = [winner]; else winners[GetLength(winners)] = winner;
		var team = GetPlayerTeam(winner);
		if (team) if (!winner_teams) winner_teams = [team]; else winner_teams[GetLength(winner_teams)] = team;
	}
	if (!winners) return false;
	has_winner = true;
	var iplr = GetPlayerCount();
	while (iplr--)
	{
		var plr = GetPlayerByIndex(iplr);
		// Free view when game is over
		for (var flag in [PLRZOOM_LimitMax, PLRZOOM_Direct])
			SetPlayerZoomByViewRange(plr, LandscapeWidth(),LandscapeWidth(),flag);
		SetPlayerViewLock(plr, false);
		SetFoW(false, plr);
		// Eliminate non-winning players
		if (is_cooperative) continue; // in coop mode, everyone wins
		if (GetIndexOf(winners, plr) >= 0) continue;
		if (winner_teams) if (GetIndexOf(winner_teams, GetPlayerTeam(plr)) >= 0) continue;
		EliminatePlayer(plr);
	}
	return true;
}

// Shows or hides a message window with information.
public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = "$MsgGoalFulfilled$";	
	else
		message = "$MsgGoalUnfulfilled$";

	return message;
}


// Shows or hides a message window with information.
public func Activate(int plr)
{
	// If goal message open -> hide it.
	if (GetEffect("GoalMessage", this))
	{
		CustomMessage("", nil, plr, nil, nil, nil, nil, nil, MSG_HCenter);
		RemoveEffect("GoalMessage", this);
		return;
	}
	// Otherwise open a new message.
	AddEffect("GoalMessage", this, 100, 0, this);
	var message;
	if (IsFulfilled())
		message = "@$MsgGoalFulfilled$";	
	else
		message = "@$MsgGoalUnfulfilled$";

	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}

public func GetShortDescription(int plr)
{
	return nil;
}

/*-- Proplist --*/

local Name = "$Name$";
