/*-- 
	Wealth
	Author: K-Pone, original by Maikel
	
	Each player much reach the specified wealth to complete the goal.
--*/


#include Library_Goal

static stalematecheck_ok;

protected func Initialize()
{
	stalematecheck_ok = false;
	GUI_Clock->CreateCountdown(60 * SCENPAR_GoalTime);
	return inherited(...);
}

public func OnlyRichSurvives()
{
	var mostwealth = 0;
	var teamwithmostwealth = -1;
	
	// Check for the wealthiest player
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var pwealth = GetWealth(GetPlayerByIndex(i));
		if (pwealth > mostwealth)
		{
			mostwealth = pwealth;
			teamwithmostwealth = GetPlayerTeam(GetPlayerByIndex(i));
		}
	}
	
	// Check if there's a stalemate
	// Players in another team than the winning one with most wealth means there's a stalemate
	var onlyoneteam = true;
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var pwealth = GetWealth(GetPlayerByIndex(i));
		if (pwealth == mostwealth)
		{
			if (GetPlayerTeam(GetPlayerByIndex(i)) != teamwithmostwealth)
			{
				// Stalemate detected
				stalematecheck_ok = false;
				GUI_Clock->CreateCountdown(10);
				return;
			}
		}
	}
	
	// No stalemate, eliminate Players that lost
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var pwealth = GetWealth(GetPlayerByIndex(i));
		if (pwealth < mostwealth)
		{
			EliminatePlayer(GetPlayerByIndex(i));
		}
	}
	
	stalematecheck_ok = true;
}


/*-- Goal interface --*/

// The goal is fulfilled if all players have the specfied wealth.
public func IsFulfilled()
{
	if (g_timeover == false) return false;
	if (stalematecheck_ok == false) return false;
	// Goal fulfilled.
	return true;
}

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = Format("$MsgGoalFulfilled$");	
	else
		message = Format("$MsgGoalUnfulfilled$");

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
		message = Format("@$MsgGoalFulfilled$");	
	else
		message = Format("@$MsgGoalUnfulfilled$");

	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}

public func GetShortDescription(int plr)
{
	return "";
}

/*-- Proplist --*/

local Name = "$Name$";
