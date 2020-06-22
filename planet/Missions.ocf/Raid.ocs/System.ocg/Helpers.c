// SetPlrKnowledge(-1, ...) sets knowledge for all players
global func SetPlrKnowledge(int plr, ...)
{
	if (!GetType(plr) || (plr == NO_OWNER))
	{
		for (var i = 0; i<GetPlayerCount(); ++i)
			SetPlrKnowledge(GetPlayerByIndex(i), ...);
		return true;
	}
	return inherited(plr, ...);
}