/*--
	Global functions used in correlation to Objects.ocd\Libraries.ocd\Goal.ocd
	
	Authors: Sven2
--*/

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