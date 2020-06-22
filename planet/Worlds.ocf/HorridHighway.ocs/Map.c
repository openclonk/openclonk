/**
	Horrid Highway
	Dynamic map with two separated sky islands.
	
	@author Maikel
*/

#include Library_Map


// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Set the map size.
	map->Resize(240 + 40 * SCENPAR_MapSize, 120);
	
	// At which height the highway will be constructed.
	var highway_height = map.Hgt / 2;
	
	// Draw left island.
	map->DrawLeftIsland(map, highway_height);
	
	// Draw right island.
	map->DrawRightIsland(map, highway_height);
	
	// Draw middle island.
	map->DrawMiddleIsland(map, highway_height);
	
	// Draw small islands.
	map->DrawSmallIslands(map);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

public func DrawLeftIsland(proplist map, int highway_height)
{
	var width = 54;
	var height = 64;
	
	var island1 = {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt / 2 - height / 2, Wdt = width - 10, Hgt = height};
	var island2 = {Algo = MAPALGO_Rect, X = 0, Y = highway_height, Wdt = width - 2, Hgt = height / 2 + map.Hgt / 2 - highway_height};
	var island = {Algo = MAPALGO_Or, Op = [island1, island2]};
	island = {Algo = MAPALGO_Or, Op = [island, {Algo = MAPALGO_Turbulence, Amplitude = 16, Scale = 12, Iterations = 4, Seed = Random(65536), Op = island}]};
	island = {Algo = MAPALGO_Or, Op = [island, {Algo = MAPALGO_Turbulence, Amplitude = 16, Scale = 12, Iterations = 4, Seed = Random(65536), Op = island}]};
	Draw("Earth-earth", island);
	DrawMaterial("Earth-earth_root", island, 2, 16);
	DrawMaterial("Earth-earth_spongy", island, 2, 16);
	
	DrawMaterial("Rock", island, 4, 10);
	DrawMaterial("Ore", island, 4, 10);
	DrawMaterial("Coal", island, 4, 10);
	DrawMaterial("Firestone", island, 4, 10);
	
	var border = {Algo = MAPALGO_Border, Right = 4, Bottom = 4, Op = island};
	Draw("Granite-granite", border);
	DrawMaterial("Rock-rock", border, 3, 20);
	DrawMaterial("Rock-rock_smooth", border, 3, 20);
	
	var top = {Algo = MAPALGO_Border, Top = 4, Op = island};
	Draw("Earth-earth", top);
	DrawMaterial("Earth-earth_root", top, 2, 16);
	DrawMaterial("Earth-earth_spongy", top, 2, 16);
	
	var passage = {Algo = MAPALGO_Rect, X = 0, Y = highway_height - 7, Wdt = 2 * width, Hgt = 7};
	passage = {Algo = MAPALGO_And, Op = [passage, island]};
	Draw("Tunnel-tunnel", passage);
	DrawMaterial("Tunnel-brickback", passage, 3, 20);
	
	var road = {Algo = MAPALGO_Rect, X = 0, Y = highway_height, Wdt = 2 * width, Hgt = 2};
	road = {Algo = MAPALGO_And, Op = [road, island]};
	Draw("Brick", road);
	
	var cave_up = {Algo = MAPALGO_Rect, X = 0, Y = highway_height - 20, Wdt = 30, Hgt = 10};
	cave_up = {Algo = MAPALGO_Or, Op = [cave_up, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = cave_up}]};
	cave_up = {Algo = MAPALGO_And, Op = [cave_up, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = map.Wdt, Hgt = highway_height - 10}]};
	Draw("Tunnel-tunnel", cave_up);
	DrawMaterial("Tunnel-brickback", cave_up, 3, 20);
	var cave_up_road = {Algo = MAPALGO_And, Op = [cave_up, {Algo = MAPALGO_Rect, X = 0, Y = highway_height - 12, Wdt = map.Wdt, Hgt = 2}]};
	Draw("Brick", cave_up_road);
	var cave_up_mat = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = 2, Hgt = map.Hgt};
	cave_up_mat = {Algo = MAPALGO_Or, Op = [cave_up_mat, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = cave_up_mat}]};
	cave_up_mat = {Algo = MAPALGO_And, Op = [cave_up, cave_up_mat]};
	Draw("Coal:Coal", cave_up_mat);
	
	var cave_down = {Algo = MAPALGO_Rect, X = 0, Y = highway_height + 5, Wdt = 30, Hgt = 10};
	cave_down = {Algo = MAPALGO_Or, Op = [cave_down, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = cave_down}]};
	cave_down = {Algo = MAPALGO_And, Op = [cave_down, {Algo = MAPALGO_Rect, X = 0, Y = highway_height + 2, Wdt = map.Wdt, Hgt = 13}]};
	Draw("Tunnel-tunnel", cave_down);
	DrawMaterial("Tunnel-brickback", cave_down, 3, 20);
	var cave_down_road = {Algo = MAPALGO_And, Op = [cave_down, {Algo = MAPALGO_Rect, X = 0, Y = highway_height + 13, Wdt = map.Wdt, Hgt = 2}]};
	Draw("Brick", cave_down_road);
	var cave_down_mat = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = 6, Hgt = map.Hgt};
	cave_down_mat = {Algo = MAPALGO_Or, Op = [cave_down_mat, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = cave_down_mat}]};
	cave_down_mat = {Algo = MAPALGO_And, Op = [cave_down, cave_down_mat, {Algo = MAPALGO_Rect, X = 0, Y = highway_height + 10, Wdt = map.Wdt, Hgt = 20}]};
	Draw("Water:Water", cave_down_mat);
	var cave_down_mat_border = {Algo = MAPALGO_Border, Right = 1, Bottom = 1, Op = cave_down_mat};
	Draw("Everrock", cave_down_mat_border);
	return;
}

public func DrawRightIsland(proplist map, int highway_height)
{
	var width = 54;
	var height = 64;
	
	var island1 = {Algo = MAPALGO_Rect, X = map.Wdt - width + 10, Y = map.Hgt / 2 - height / 2, Wdt = width - 10, Hgt = height};
	var island2 = {Algo = MAPALGO_Rect, X = map.Wdt - width, Y = highway_height, Wdt = width - 2, Hgt = height / 2 + map.Hgt / 2 - highway_height};
	var island = {Algo = MAPALGO_Or, Op = [island1, island2]};
	island = {Algo = MAPALGO_Or, Op = [island, {Algo = MAPALGO_Turbulence, Amplitude = 16, Scale = 12, Iterations = 4, Seed = Random(65536), Op = island}]};
	island = {Algo = MAPALGO_Or, Op = [island, {Algo = MAPALGO_Turbulence, Amplitude = 16, Scale = 12, Iterations = 4, Seed = Random(65536), Op = island}]};
	Draw("Earth-earth", island);
	DrawMaterial("Earth-earth_root", island, 2, 16);
	DrawMaterial("Earth-earth_spongy", island, 2, 16);
	
	DrawMaterial("Rock", island, 4, 10);
	DrawMaterial("Ore", island, 4, 10);
	DrawMaterial("Coal", island, 4, 10);
	DrawMaterial("Firestone", island, 4, 10);
	
	var border = {Algo = MAPALGO_Border, Left = 4, Bottom = 4, Op = island};
	Draw("Granite-granite", border);
	DrawMaterial("Rock-rock", border, 3, 20);
	DrawMaterial("Rock-rock_smooth", border, 3, 20);
	
	var top = {Algo = MAPALGO_Border, Top = 4, Op = island};
	Draw("Earth-earth", top);
	DrawMaterial("Earth-earth_root", top, 2, 16);
	DrawMaterial("Earth-earth_spongy", top, 2, 16);
	
	var passage = {Algo = MAPALGO_Rect, X = map.Wdt - 2 * width, Y = highway_height - 7, Wdt = 2 * width, Hgt = 7};
	passage = {Algo = MAPALGO_And, Op = [passage, island]};
	Draw("Tunnel-tunnel", passage);
	DrawMaterial("Tunnel-brickback", passage, 3, 20);
	
	var road = {Algo = MAPALGO_Rect, X = map.Wdt - 2 * width, Y = highway_height, Wdt = 2 * width, Hgt = 2};
	road = {Algo = MAPALGO_And, Op = [road, island]};
	Draw("Brick", road);
	
	var cave_up = {Algo = MAPALGO_Rect, X = map.Wdt - 30, Y = highway_height - 20, Wdt = 30, Hgt = 10};
	cave_up = {Algo = MAPALGO_Or, Op = [cave_up, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = cave_up}]};
	cave_up = {Algo = MAPALGO_And, Op = [cave_up, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = map.Wdt, Hgt = highway_height - 10}]};
	Draw("Tunnel-tunnel", cave_up);
	DrawMaterial("Tunnel-brickback", cave_up, 3, 20);
	var cave_up_road = {Algo = MAPALGO_And, Op = [cave_up, {Algo = MAPALGO_Rect, X = 0, Y = highway_height - 12, Wdt = map.Wdt, Hgt = 2}]};
	Draw("Brick", cave_up_road);
	var cave_up_mat = {Algo = MAPALGO_Rect, X = map.Wdt - 2, Y = 0, Wdt = 2, Hgt = map.Hgt};
	cave_up_mat = {Algo = MAPALGO_Or, Op = [cave_up_mat, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = cave_up_mat}]};
	cave_up_mat = {Algo = MAPALGO_And, Op = [cave_up, cave_up_mat]};
	Draw("Ore:Ore", cave_up_mat);
	
	var cave_down = {Algo = MAPALGO_Rect, X = map.Wdt - 30, Y = highway_height + 5, Wdt = 30, Hgt = 10};
	cave_down = {Algo = MAPALGO_Or, Op = [cave_down, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = cave_down}]};
	cave_down = {Algo = MAPALGO_And, Op = [cave_down, {Algo = MAPALGO_Rect, X = 0, Y = highway_height + 2, Wdt = map.Wdt, Hgt = 13}]};
	Draw("Tunnel-tunnel", cave_down);
	DrawMaterial("Tunnel-brickback", cave_down, 3, 20);
	var cave_down_road = {Algo = MAPALGO_And, Op = [cave_down, {Algo = MAPALGO_Rect, X = 0, Y = highway_height + 13, Wdt = map.Wdt, Hgt = 2}]};
	Draw("Brick", cave_down_road);
	var cave_down_mat = {Algo = MAPALGO_Rect, X = map.Wdt - 2, Y = 0, Wdt = 2, Hgt = map.Hgt};
	cave_down_mat = {Algo = MAPALGO_Or, Op = [cave_down_mat, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = cave_down_mat}]};
	cave_down_mat = {Algo = MAPALGO_And, Op = [cave_down, cave_down_mat]};
	Draw("Rock:Rock", cave_down_mat);
	return;
}

public func DrawMiddleIsland(proplist map, int highway_height)
{
	var width = 22;
	var height = 72;
	
	var island = {Algo = MAPALGO_Ellipse, X = map.Wdt / 2, Y = map.Hgt / 2, Wdt = width / 2, Hgt = height / 2};
	island = {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 8, Iterations = 2, Seed = Random(65536), Op = island};
	Draw("Everrock", island);
	
	var passage = {Algo = MAPALGO_Rect, X = map.Wdt / 2 - 2 * width, Y = highway_height - 7, Wdt = 4 * width, Hgt = 7};
	passage = {Algo = MAPALGO_And, Op = [passage, island]};
	Draw("Tunnel", passage);
	DrawMaterial("Tunnel-brickback", passage, 3, 20);
	
	var road = {Algo = MAPALGO_Rect, X = map.Wdt / 2 - 2 * width, Y = highway_height, Wdt = 4 * width, Hgt = 2};
	road = {Algo = MAPALGO_And, Op = [road, island]};
	Draw("Brick", road);
	
	var core = {Algo = MAPALGO_And, Op = [island, {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Or, Op = [passage, road]}}]};
	var core_border = {Algo = MAPALGO_Border, Op = core, Left = 5, Right = 5, Top = 5};
	core = {Algo = MAPALGO_And, Op = [core, {Algo = MAPALGO_Not, Op = core_border}]};
	Draw("Gold", core);
	var core_bottom = {Algo = MAPALGO_Rect, X = map.Wdt / 2 - 1, Y = map.Hgt / 2 + height / 2 - 12, Wdt = 2, Hgt = map.Hgt};
	core_bottom = {Algo = MAPALGO_Or, Op = [core_bottom, {Algo = MAPALGO_Turbulence, Amplitude = 8, Scale = 8, Iterations = 2, Seed = Random(65536), Op = core_bottom}]};
	core = {Algo = MAPALGO_And, Op = [island, core_bottom]};
	Draw("Gold", core);
	return;
}

public func DrawSmallIslands(proplist map)
{
	var width = 10;
	var height = 14;

	var x1 = 4 * map.Wdt / 11 + RandomX(-3, 3);
	var x2 = 7 * map.Wdt / 11 + RandomX(-3, 3);
	var y = 10 * map.Hgt / 11;
	var island1 = {Algo = MAPALGO_Ellipse, X = x1, Y = y, Wdt = width / 2, Hgt = height};
	var island2 = {Algo = MAPALGO_Ellipse, X = x2, Y = y, Wdt = width / 2, Hgt = height};
	island1 = {Algo = MAPALGO_And, Op = [island1, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = map.Wdt, Hgt = y}]};
	island2 = {Algo = MAPALGO_And, Op = [island2, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = map.Wdt, Hgt = y}]};
	island1 = {Algo = MAPALGO_Or, Op = [island1, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 10, Iterations = 3, Seed = Random(65536), Op = island1}]};
	island2 = {Algo = MAPALGO_Or, Op = [island2, {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 10, Iterations = 3, Seed = Random(65536), Op = island2}]};
	
	var islands = {Algo = MAPALGO_Or, Op = [island1, island2]};
	var islands_border = {Algo = MAPALGO_Border, Left = 3, Right = 3, Top = 3, Op = islands};
	var no_border = {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Or, Op = [{Algo = MAPALGO_Rect, X = x1 - 1, Y = y - 3, Wdt = 3, Hgt = map.Hgt}, {Algo = MAPALGO_Rect, X = x2 - 1, Y = y - 3, Wdt = 3, Hgt = map.Hgt}]}};
	islands_border = {Algo = MAPALGO_And, Op = [islands_border, no_border]};
	
	Draw("Gold", islands);
	Draw("Everrock", islands_border);
	return;
}
