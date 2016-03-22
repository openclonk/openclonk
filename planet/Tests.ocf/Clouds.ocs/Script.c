

func Initialize()
{
	// Note: Lava won't work because the temperature is too low.
	var mats = ["Water", "DuroLava", "Acid", "Ice", "Snow"];
	var n = GetLength(mats);
	Cloud->Place(n);
	var clouds = FindObjects(Find_ID(Cloud));
	for (var i = 0; i < n; i++)
	{
		clouds[i]->SetPrecipitation(mats[i], 200);
	}
}

func InitializePlayer(int plr)
{
	var clonk = GetCrew(plr);
	var plane = CreateObjectAbove(Airplane, clonk->GetX(), clonk->GetY() - 50);
	clonk->Enter(plane);
}
