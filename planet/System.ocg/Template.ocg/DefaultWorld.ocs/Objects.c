/* Default scenario objects */
// Use gold rush objects with minor adjustments

/*-- Scenario Initialization --*/

public func InitializeObjects()
{
	// Place player start centered above ground
	var start_x = LandscapeWidth()/2;
	var start_y = 0;
	while (!GBackSolid(start_x, start_y) && start_y < LandscapeHeight()) ++start_y;
	CreateObjectAbove(PlayerStart, start_x, start_y);
	// Place regular objects
	InitEnvironment();
	InitVegetation();
	InitAnimals();
	return true;
}

private func InitEnvironment()
{
	CreateEnvironmentObjects("Temperate");
	
	// Set time of day to evening and create some clouds and celestials.
	Cloud->Place(10);
	Cloud->SetPrecipitation("Water", 8);
	var time = CreateObject(Time);
	time->SetTime(60 * 12);
	time->SetCycleSpeed(20);
}

private func InitVegetation()
{
	// Place some trees in a forest shape.
	PlaceForest([Tree_Deciduous, Tree_Coniferous2], 0, LandscapeHeight() / 2 + 50, nil, true);

	SproutBerryBush->Place();
	PlaceGrass(100);
	
	// Some objects in the earth.
	var map_size = Max(1, LandscapeWidth() * LandscapeHeight() / 250000);	
	PlaceObjects(Rock, 25 + 10 * map_size + Random(10),"Earth");
	PlaceObjects(Firestone, 20 + 10 * map_size + Random(5), "Earth");
	PlaceObjects(Loam, 20 + 10 * map_size + Random(5), "Earth");
}

private func InitAnimals(int map_size)
{
	// Some butterflies
	for (var i = 0; i < 10 + 5 * Max(1, LandscapeWidth() / 500); i++)
		PlaceAnimal(Butterfly);
}
