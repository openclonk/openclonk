/**
	Goal control
	Include this to all C4D_Goal objects. Functions to be overloaded:
	 * bool IsFullfilled(); - is the goal fulfilled?
	 * string GetDescription(int plr); - description of the goal
		
	@author Sven2
*/

local EditorPlacementLimit = 1; // Goals are to be placed only once (unless overloaded by the goal)

local mission_password; // mission password to be gained when the goal is fulfilled

// Initialization
func Initialize()
{
	// Do not create Library_Goal itself
	if (GetID() == Library_Goal)
	{
		Log("WARNING: Abstract Library_Goal object should not be created; object removed.");
		return RemoveObject();
	}
	// Create timer if it doesn't exist yet
	RecheckGoalTimer();
	// Done
	return _inherited(...);
}

func OnSynchronized()
{
	// Create timer if it doesn't exist yet
	RecheckGoalTimer();
	return _inherited(...);
}

func RecheckGoalTimer()
{
	// Create timer if it doesn't exist yet
	if (!GetEffect("IntGoalCheck", nil))
	{
		var timer_interval = 35;
		if (GetLeague())
			timer_interval = 2; // league has more frequent checks
		var num = AddEffect("IntGoalCheck", nil, 1, timer_interval, nil, Library_Goal);
		FxIntGoalCheckTimer(nil, num);
	}
}

protected func FxIntGoalCheckTimer(object trg, effect, int time)
{
	if (!time)
		return true;
	var curr_goal = effect.curr_goal;
	// Check current goal object
	if (curr_goal && (curr_goal->GetCategory() & C4D_Goal))
	{
		if (!curr_goal->~IsFulfilled())
		{
			curr_goal->NotifyHUD(); // The HUD has to be updated only if the goal is not fulfilled: in the other case a new goal will be chosen and that goal then updates the HUD
			return true;
		}
	}
	// Current goal is fulfilled/destroyed - check all others
	var goal_count = 0;
	for (curr_goal in FindObjects(Find_Category(C4D_Goal)))
	{
		++goal_count;
		// The first unfulfilled goal is chosen
		if (!curr_goal->~IsFulfilled())
		{
			effect.curr_goal = curr_goal;
			curr_goal->NotifyHUD();
			return true;
		}
	}
	// There were goal objects, and all goals are fulfilled
	if (goal_count)
	{
		AllGoalsFulfilled(); // Game over :(
	}
	// Kill Timer
	return FX_Execute_Kill;
}

protected func AllGoalsFulfilled()
{
	// Goals fulfilled: Set mission password(s)
	for (var goal in FindObjects(Find_Category(C4D_Goal)))
		if (goal.mission_password)
			GainScenarioAccess(goal.mission_password);
	// Custom scenario goal evaluation?
	if (GameCall("OnGoalsFulfilled")) return true;
	// We're done. Play some sound and schedule game over call
	Sound("UI::Fanfare", true);
	AddEffect("IntGoalDone", nil, 1, 30, nil, Library_Goal);
}

protected func FxIntGoalDoneStop()
{
	GameOver();
}

public func NotifyHUD()
{
	// create hud objects for all players
	for (var i = 0; i < GetPlayerCount(); ++i)
	{
		NotifyPlayerHUD(GetPlayerByIndex(i));
	}
}

protected func InitializePlayer(int plr, ...)
{
	NotifyPlayerHUD(plr);
	_inherited(plr, ...);
}

private func NotifyPlayerHUD(int plr)
{
	var HUD = FindObject(Find_ID(Library_HUDController->GetGUIControllerID()), Find_Owner(plr));
	if (HUD)
		HUD->OnGoalUpdate(this);
}

// Set mission password to be gained when all goals are fulfilled
public func SetMissionAccess(string str_password)
{
	mission_password = str_password;
}

// Base implementations to be overloaded by goal objects

// Overload: return whether the goal has been fulfilled.
public func IsFulfilled() { return true; }

// Overload: return the current description for this goal.
public func GetDescription(int plr)
{
	return this.Description ?? "WARNING: GetDescription(int plr) not overloaded by goal";
}

protected func Activate(plr)
{
	if (IsFulfilled())
		return MessageWindow("$MsgGoalFulfilled$", plr);
	return MessageWindow(this->GetDescription(plr), plr);
}

// Scenario saving.
func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (mission_password) props->AddCall("MissionAccess", this, "SetMissionAccess", Format("%v", mission_password));
	return true;
}


/* Graphics storage */
// workaround so goals with different graphics are correctly displayed in the HUD

local goal_custom_graphics;

func SetGraphics(string new_gfx, ...)
{
	goal_custom_graphics = new_gfx;
	return inherited(new_gfx, ...);
}

func GetGraphics(int plr) { return goal_custom_graphics; }

public func IsGoal() { return true; }

local Visibility = VIS_Editor;
