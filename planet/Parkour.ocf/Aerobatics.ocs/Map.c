/**
	Aerobatics
	A map with lots of small sky islands forming a parkour sequence.
	
	@author Maikel
*/

#include Library_Map


static checkpoint_locations;
static inventorslab_location;

// Called be the engine: draw the complete map here.
public func InitializeMap(proplist map)
{
	// Initialize a list for checkpoint locations.
	checkpoint_locations = [];
	
	// Determine the number of checkpoints.
	var nr_checkpoints = BoundBy(SCENPAR_NrCheckPoints, 6, 20);

	// Set the map size: depends on game mode and number of checkpoints.
	var map_wdt = 140 + nr_checkpoints * 6;
	var map_hgt = map_wdt / 2;
	if (SCENPAR_GameMode == 2)
		map_wdt *= 2;
	map->Resize(map_wdt, map_hgt);
	
	// Draw the start platform
	DrawStartFinishIsland(map, map.Wdt / 5, map.Hgt / 2 - 2);
	PushBack(checkpoint_locations, [map.Wdt / 5, map.Hgt / 2 - 2]);
	
	// Find the checkpoint locations: substract the finish from the number of checkpoints.
	var checkpoint_islands = FindCheckPointLocations(map, nr_checkpoints - 1);
	// Store the locations in the list and draw the islands.
	for (var index = 0; index < GetLength(checkpoint_islands); index++)
	{
		var cp_island = checkpoint_islands[index];
		PushBack(checkpoint_locations, [cp_island.X, cp_island.Y]);
		DrawCheckpointIsland(map, cp_island.X, cp_island.Y, index >= GetLength(checkpoint_islands) - 1 - Random(2));
	}
	
	// Draw the finish platform.
	DrawStartFinishIsland(map, 4 * map.Wdt / 5, map.Hgt / 2 - 2);
	PushBack(checkpoint_locations, [4 * map.Wdt / 5, map.Hgt / 2 - 2]);
	
	// Draw a platform for the inventor's lab.
	DrawStartFinishIsland(map, map.Wdt / 2, map.Hgt / 2 - 2);
	inventorslab_location = [map.Wdt / 2, map.Hgt / 2 - 2];
	
	// Draw other smaller islands.
	DrawSmallIslands(map, checkpoint_locations);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

public func DrawStartFinishIsland(proplist map, int x, int y)
{
	var island = {Algo = MAPALGO_Rect, X = x - 7, Y = y, Wdt = 15, Hgt = 8};
	island = {Algo = MAPALGO_Or, Op = [island, {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 6, Op = island}]};
	island = {Algo = MAPALGO_And, Op = [island, {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = x - 6, Y = y - 5, Wdt = 13, Hgt = 5}}]};
	island = {Algo = MAPALGO_And, Op = [island, {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = x - 9, Y = y - 5, Wdt = 17, Hgt = 4}}]};
	Draw("Earth", island);
	DrawMaterial("Firestone", island, 2, 10);
	DrawMaterial("Rock", island, 2, 25);
	var brick = {Algo = MAPALGO_Rect, X = x - 5, Y = y, Wdt = 11, Hgt = 4};
	Draw("Brick", brick);
	return;
}

public func FindCheckPointLocations(proplist map, int nr_checkpoints)
{
	// Prepare a mask out of the map.
	var mask = map->CreateLayer();
	mask->Draw("Rock");
	// Remove the start, finish and middle island from the mask.
	mask->Draw("Tunnel", {Algo = MAPALGO_Ellipsis, X = map.Wdt / 5, Y = map.Hgt / 2, Wdt = 38, Hgt = 38});
	mask->Draw("Tunnel", {Algo = MAPALGO_Ellipsis, X = 4 * map.Wdt / 5, Y = map.Hgt / 2, Wdt = 38, Hgt = 38});
	mask->Draw("Tunnel", {Algo = MAPALGO_Ellipsis, X = map.Wdt / 2, Y = map.Hgt / 2, Wdt = 38, Hgt = 38});
	// Remove the middle area between start and finish from the mask.
	mask->Draw("Tunnel", {Algo = MAPALGO_Rect, X = map.Wdt / 5, Y = map.Hgt / 2 - 16, Wdt = 3 * map.Wdt / 5, Hgt = 16});	
	// Array for the checkpoint islands.	
	var checkpoint_islands = [];	
	// Add checkpoint islands at random locations around the map.
	var border = 10;
	var cp_dist = 28;
	for (var i = 0; i < nr_checkpoints; i++)
	{
		var cp_island = {};
		if (!mask->FindPosition(cp_island, "Rock", [border, border, map.Wdt - border * 2, map.Hgt - border * 2]))
		{
			Log("WARNING: Map script could not find a suitable checkpoint location, there will be one checkpoint less in this round.");
			continue;
		}
		mask->Draw("Tunnel", {Algo = MAPALGO_Ellipsis, X = cp_island.X, Y = cp_island.Y, Wdt = cp_dist, Hgt = cp_dist});	
		PushBack(checkpoint_islands, cp_island);
	}
	return checkpoint_islands;
}

public func DrawCheckpointIsland(proplist map, int x, int y, bool enclosed)
{
	var island = {Algo = MAPALGO_Rect, X = x - 4, Y = y, Wdt = 9, Hgt = 6};
	island = {Algo = MAPALGO_Or, Op = [island, {Algo = MAPALGO_Turbulence, Seed = Random(65536), Amplitude = 8, Scale = 8, Op = island}]};
	island = {Algo = MAPALGO_And, Op = [island, {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = x - 4, Y = y - 5, Wdt = 9, Hgt = 5}}]};
	Draw("Earth", island);
	DrawMaterial("Firestone", island, 2, 10);
	DrawMaterial("Rock", island, 2, 25);
	var brick = {Algo = MAPALGO_Rect, X = x - 2, Y = y, Wdt = 5, Hgt = 2};
	Draw("Brick", brick);
	// Trap the checkpoint in rock / granite if it must be enclosed.
	if (enclosed)
	{
		var border = {Algo = MAPALGO_Ellipsis, X = x, Y = y - 3, Wdt = 7, Hgt = 7};
		border = {Algo = MAPALGO_Border, Op = border, Wdt = 2};
		border = {Algo = MAPALGO_And, Op = [border, {Algo = MAPALGO_Not, Op = island}]};
		Draw("Granite", border);
		DrawMaterial("Rock", border, 2, 20);
	}
	return;
}

public func DrawSmallIslands(proplist map, array checkpoints)
{
	// Prepare a mask out of the checkpoint positions.
	var mask = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = 0, Hgt = 0};
	var island_border = 16;
	var map_border = 6;
	// Remove checkpoints and middle area from mask.
	for (var cp in checkpoints)
		mask = {Algo = MAPALGO_Or, Op = [mask, {Algo = MAPALGO_Ellipsis, X = cp[0], Y = cp[1], Wdt = island_border, Hgt = island_border}]};
	mask = {Algo = MAPALGO_Or, Op = [mask, {Algo = MAPALGO_Rect, X = map.Wdt / 5, Y = map.Hgt / 2 - 16, Wdt = 3 * map.Wdt / 5, Hgt = 16}]};
	mask = {Algo = MAPALGO_Not, Op = mask};
	mask = {Algo = MAPALGO_And, Op = [mask, {Algo = MAPALGO_Rect, X = map_border, Y = map_border, Wdt = map.Wdt - 2 * map_border, Hgt = map.Hgt - 2 * map_border}]};
	
	var islands = {Algo = MAPALGO_RndChecker, Seed = Random(65536), Ratio = 4, Wdt = 4, Hgt = 3};
	islands = {Algo = MAPALGO_And, Op = [islands, mask]};
	islands = {Algo = MAPALGO_Or, Op = [islands, {Algo = MAPALGO_Turbulence, Seed = Random(65536), Amplitude = 6, Scale = 6, Op = islands}]};
	
	Draw("Earth", islands);
	DrawMaterial("Firestone", islands, 2, 10);
	DrawMaterial("Rock", islands, 2, 25);
	DrawMaterial("Ore", islands, 2, 5);
	DrawMaterial("Granite", islands, 2, 5);
	return;
}