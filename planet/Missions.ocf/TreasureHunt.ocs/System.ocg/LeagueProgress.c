/* Store league progress and score for all players */

global func SetLeagueProgressScore(int new_progress, int new_score)
{
	// Progress must be between 0 and 25
	new_progress = BoundBy(new_progress, 0, 25);
	for (var i = 0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i);
		var plrid = GetPlayerID(plr);
		if (!plrid) return;
		// Get old progress from previous round
		var progress_string = GetLeagueProgressData(plrid);
		if (progress_string && GetLength(progress_string)>=2)
		{
			var old_progress = GetChar(progress_string, 1)-GetChar("A");
			// If old progress was better than new progress, keep old progress
			new_progress = Max(old_progress, new_progress);
		}
		// Set new progress
		SetLeagueProgressData(Format("A%c", GetChar("A") + new_progress), plrid);
		SetLeaguePerformance(new_score, plrid);
	}
	return true;
}