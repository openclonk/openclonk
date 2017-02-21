
/*--
		CollectStatistics.c
		Authors: Luchs

		Global entry point for statistics collection for the masterserver.
--*/


// This function is called after the round ends. The return value is passed to
// the masterserver.
global func CollectStatistics()
{
	var result = {};
	var i = 0, def, stats;
	while (def = GetDefinition(i++))
	{
		stats = def->~CollectStats();
		if (stats != nil)
			result[def->GetName(true)] = stats;
	}
	stats = Scenario->~CollectStats();
	if (stats != nil)
		result.Scenario = stats;
	if (GetLength(GetProperties(SCENPAR)))
		result.SCENPAR = SCENPAR;
	if (GetLength(GetProperties(result)))
		return result;
	return nil;
}
