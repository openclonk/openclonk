static num_to_collect;

func Initialize()
{
	Log("Difficulty = %d", SCENPAR_Difficulty);
	num_to_collect = SCENPAR_Difficulty;
	for (var i=0; i<num_to_collect; ++i)
	{
		var ng = CreateObject(Nugget, Random(LandscapeWidth()-100) + 50, 10);
		ng.Entrance = Scenario.GotNugget;
	}
	return true;
}

func GotNugget()
{
	--num_to_collect;
	if (num_to_collect == 0) GameCall("Finished");
}

func Finished()
{
	Log("All nuggets collected!");
	GainScenarioAchievement("Done", BoundBy(SCENPAR_Difficulty, 1, 3));
	GameOver();
}
