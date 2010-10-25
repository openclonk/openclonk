/* Everyone wins! */

#appendto Goal_Parkour

public func PlrReachedFinishCP(int i, object cp)
{
	if (finished)
		return;

	for(var plr=0; plr<GetPlayerCount(); plr++)
	{
		UpdateScoreboard(plr);
		DoBestTime(plr);
		SetEvalData(plr);
	}
	finished = true;
	return;
}