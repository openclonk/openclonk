/*--
	Goal control
	Author: Sven2
	
	Include this to all C4D_Goal objects
	Functions to be overloaded:
		bool IsFullfilled(); - is the goal fulfilled?
--*/


local mission_password; // mission password to be gained when the goal is fulfilled

// Initialization
func Initialize()
{
	// Do not create Library_Goal itself
	if (GetID()==Library_Goal)
	{
		Log("WARNING: Abstract Library_Goal object should not be created; object removed.");
		return RemoveObject();
	}
	// Create timer if it doesn't exist yet
	RecheckGoalTimer();
	// Done
	return _inherited(...);
}

func UpdateTransferZone()
{
	// Create timer if it doesn't exist yet
	RecheckGoalTimer();
	return _inherited(...);
}

func RecheckGoalTimer()
{
	// Create timer if it doesn't exist yet
	if (!GetEffect("IntGoalCheck", 0))
	{
		var timer_interval = 35;
		if (GetLeague())
			timer_interval = 2; // league has more frequent checks
		var num = AddEffect("IntGoalCheck", 0, 1, timer_interval, 0);
		FxIntGoalCheckTimer(nil, num);
	}
}

public func NotifyHUD()
{
	// create hud objects for all players
	for (var i = 0; i < GetPlayerCount(); ++i)
	{
		var plr = GetPlayerByIndex(i);
		var HUD = FindObject(Find_ID(GUI_Controller), Find_Owner(plr));
		if (HUD)
			HUD->OnGoalUpdate(this);
	}
}

protected func InitializePlayer(int plr)
{
	var HUD = FindObject(Find_ID(GUI_Controller), Find_Owner(plr));
	if (HUD)
		HUD->OnGoalUpdate(this);
}

global func FxIntGoalCheckTimer(object trg, effect, int time)
{
	if (!time)
		return true;
	var curr_goal = effect.curr_goal;
	// Check current goal object
	if (curr_goal && (curr_goal->GetCategory() & C4D_Goal))
	{
		curr_goal->NotifyHUD();
		if (!curr_goal->~IsFulfilled())
			return true;
	}
	// Current goal is fulfilled/destroyed - check all others
	var goal_count = 0;
	for (curr_goal in FindObjects(Find_Category(C4D_Goal)))
	{
		++goal_count;
		if (!curr_goal->~IsFulfilled())
		{
			effect.curr_goal = curr_goal;
			curr_goal->NotifyHUD();
			return true;
		}
	}
	// No goal object? Kill timer
	if (!goal_count)
		return FX_Execute_Kill;
	// Game over :(
	AllGoalsFulfilled();
	return FX_Execute_Kill;
}

global func AllGoalsFulfilled()
{
	// Goals fulfilled: Set mission password(s)
	for (var goal in FindObjects(Find_Category(C4D_Goal)))
		if (goal.mission_password)
			GainMissionAccess(goal.mission_password);
	// Custom scenario goal evaluation?
	if (GameCall("OnGoalsFulfilled")) return true;
	// We're done. Play some sound and schedule game over call
	Sound("Fanfare", true);
	AddEffect("IntGoalDone", 0, 1, 30, 0);
}

global func FxIntGoalDoneStop()
{
	GameOver();
}

// Set mission password to be gained when all goals are fulfilled
public func SetMissionAccess(string str_password)
{
	mission_password = str_password;
}

// Base implementations to be overloaded by goal objects

public func IsFulfilled() { return true; }

protected func Activate(plr)
{
	if (IsFulfilled())
		return(MessageWindow("$MsgGoalFulfilled$", plr));
	return MessageWindow(GetProperty("Description"), plr);
}
