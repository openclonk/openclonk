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
	{
		map_wdt = 60 + nr_checkpoints * 28;
		map_hgt = 72 + nr_checkpoints * 2;	
	}
	map->Resize(map_wdt, map_hgt);
	
	// Determine start and finish locations.
	var start_x = map.Wdt / 5;
	var finish_x = 4 * map.Wdt / 5;
	var middle_x = map.Wdt / 2;
	var island_y = map.Hgt / 2 - 2;
	if (SCENPAR_GameMode == 2)
	{
		start_x = 10;
		finish_x = map.Wdt - 10;
	}
		
	// Draw the start platform.
	DrawStartFinishIsland(map, start_x, island_y);
	PushBack(checkpoint_locations, [start_x, island_y]);
	
	// Find the checkpoint locations: substract the finish from the number of checkpoints.
	var checkpoint_islands = FindCheckPointLocations(map, nr_checkpoints - 1, SCENPAR_GameMode, start_x, finish_x, middle_x, island_y);
	// Store the locations in the list and draw the islands.
	for (var index = 0; index < GetLength(checkpoint_islands); index++)
	{
		var cp_island = checkpoint_islands[index];
		PushBack(checkpoint_locations, [cp_island.X, cp_island.Y]);
		DrawCheckpointIsland(map, cp_island.X, cp_island.Y, index >= GetLength(checkpoint_islands) - 1 - Random(2));
	}
	
	// Draw the finish platform.
	DrawStartFinishIsland(map, finish_x, island_y);
	PushBack(checkpoint_locations, [finish_x, island_y]);
	
	// Draw a platform for the inventor's lab.
	DrawStartFinishIsland(map, middle_x, island_y);
	inventorslab_location = [middle_x, island_y];
	
	// Draw other smaller islands.
	DrawSmallIslands(map, checkpoint_locations, start_x, finish_x, middle_x, island_y);
	
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
	var brick = {Algo = MAPALGO_Rect, X = x - 5, Y = y, Wdt = 11, Hgt = 2};
	Draw("Brick", brick);
	return;
}

public func FindCheckPointLocations(proplist map, int nr_checkpoints, int game_mode, int start_x, int finish_x, int middle_x, int island_y)
{
	// Prepare a mask out of the map.
	var mask = map->CreateLayer();
	mask->Draw("Rock");
	// Remove the start, finish and middle island from the mask.
	mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = start_x, Y = island_y, Wdt = 38, Hgt = 38});
	mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = finish_x, Y = island_y, Wdt = 38, Hgt = 38});
	mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = middle_x, Y = island_y, Wdt = 38, Hgt = 38});
	// Remove the middle area between start and finish from the mask.
	mask->Draw("Tunnel", {Algo = MAPALGO_Rect, X = start_x, Y = island_y - 16, Wdt = finish_x - start_x, Hgt = 16});	
	// Array for the checkpoint islands.	
	var checkpoint_islands = [];	
	// Add checkpoint islands at random locations around the map.
	var border = 10;
	var cp_dist_x = 28;
	var cp_dist_y = 28;
	// For the horizontal game mode the distance is larger.
	if (game_mode == 2)
	{
		cp_dist_x = 42;
		cp_dist_y = 30;	
	}
	for (var i = 0; i < nr_checkpoints; i++)
	{
		var cp_island = {};
		if (!mask->FindPosition(cp_island, "Rock", [border, border, map.Wdt - border * 2, map.Hgt - border * 2]))
		{
			Log("WARNING: Map script could not find a suitable checkpoint location, there will be one checkpoint less in this round.");
			continue;
		}
		mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = cp_island.X, Y = cp_island.Y, Wdt = cp_dist_x, Hgt = cp_dist_y});	
		PushBack(checkpoint_islands, cp_island);
	}
	// Sort the checkpoints horizontally if the game mode is such.
	if (game_mode == 2)
	{
		for (var i = 1; i < GetLength(checkpoint_islands); i++)
		{
			var store = checkpoint_islands[i];
			var j = i - 1;
			while (j >= 0 && checkpoint_islands[j].X > store.X)
			{
				checkpoint_islands[j + 1] = checkpoint_islands[j];
				j--;
			}
			checkpoint_islands[j + 1] = store;
		}
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
		var border = {Algo = MAPALGO_Ellipse, X = x, Y = y - 3, Wdt = 7, Hgt = 7};
		border = {Algo = MAPALGO_Border, Op = border, Wdt = 2};
		border = {Algo = MAPALGO_And, Op = [border, {Algo = MAPALGO_Not, Op = island}]};
		Draw("Granite", border);
		DrawMaterial("Rock", border, 2, 20);
	}
	return;
}

public func DrawSmallIslands(proplist map, array checkpoints, int start_x, int finish_x, int middle_x, int island_y)
{
	// Prepare a mask out of the checkpoint positions.
	var mask = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = 0, Hgt = 0};
	var island_border = 16;
	var map_border = 6;
	// Remove checkpoints and middle area from mask.
	for (var cp in checkpoints)
		mask = {Algo = MAPALGO_Or, Op = [mask, {Algo = MAPALGO_Ellipse, X = cp[0], Y = cp[1], Wdt = island_border, Hgt = island_border}]};
	mask = {Algo = MAPALGO_Or, Op = [mask, {Algo = MAPALGO_Rect, X = start_x, Y = island_y - 16, Wdt = finish_x - start_x, Hgt = 16}]};
	mask = {Algo = MAPALGO_Not, Op = mask};
	mask = {Algo = MAPALGO_And, Op = [mask, {Algo = MAPALGO_Rect, X = map_border, Y = map_border, Wdt = map.Wdt - 2 * map_border, Hgt = map.Hgt - 2 * map_border}]};
	
	// Construct the island masks.
	var islands = {Algo = MAPALGO_RndChecker, Seed = Random(65536), Ratio = 4, Wdt = 4, Hgt = 3};
	islands = {Algo = MAPALGO_And, Op = [islands, mask]};
	islands = {Algo = MAPALGO_Or, Op = [islands, {Algo = MAPALGO_Turbulence, Seed = Random(65536), Amplitude = 6, Scale = 6, Op = islands}]};
	
	// Draw earth and some materials.
	Draw("Earth", islands);
	DrawMaterial("Firestone", islands, 2, 10);
	DrawMaterial("Rock", islands, 2, 25);
	DrawMaterial("Ore", islands, 2, 5);
	DrawMaterial("Granite", islands, 2, 5);
	return;
}