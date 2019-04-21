/**
	Clonkomotive
	A map with a large number of cliffs.
	
	@author Pyrit, Maikel
*/

#include Library_Map


// Bat cave [x, y] coordinates.
static bat_cave;

// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Retrieve the settings according to the MapSize setting.
	var map_size;
	if (SCENPAR_MapSize == 1)
		map_size = [400, 120]; 
	if (SCENPAR_MapSize == 2)
		map_size = [480, 120];
	if (SCENPAR_MapSize == 3)
		map_size = [560, 120];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
  
	// First tunnel background on the bottom and set the water level.
	var tunnel = {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - 30, Wdt = map.Wdt,  Hgt = 30};
	tunnel = {Algo = MAPALGO_Turbulence, Iterations = 2, Amplitude = [10, 16], Scale = 20, Op = tunnel};
	Draw("Tunnel", tunnel);	
	Draw("Water", {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - 22, Wdt = map.Wdt, Hgt = 22});
	
	// Construct a series of cliffs based on a large surfaced interfaced with lines.
	var basic_shape = {Algo = MAPALGO_Polygon, X = [0, 0, map.Wdt, map.Wdt, map.Wdt - 40, map.Wdt / 2], Y = [3 * map.Hgt / 7, map.Hgt, map.Hgt, map.Hgt / 3, map.Hgt / 3, 3 * map.Hgt / 7]};
	// Substract lines in the middle of the map, 50 pixels at both borders are left out.
	var lines = {Algo = MAPALGO_Lines, X = 40, OffX = 40 + 20, Distance = 80};
	var cliff_rect = {Algo = MAPALGO_Rect, X = 40, Y = 0, Wdt = map.Wdt - 80, Hgt = map.Hgt};
	lines = {Algo = MAPALGO_And, Op = [lines, cliff_rect]};
	lines = {Algo = MAPALGO_Not, Op = lines};
	// Construct the basic cliff shape.	
	var cliffs = {Algo = MAPALGO_And, Op = [basic_shape, lines]};
	cliffs = {Algo = MAPALGO_Or, Op = [{Algo = MAPALGO_And, Op = [cliffs, {Algo = MAPALGO_Not, Op = cliff_rect}]}, {Algo = MAPALGO_Turbulence, Iterations = 3, Amplitude = [10, 4], Scale = [8, 4], Seed = Random(65536), Op = cliffs}]};
	
	// Fill the shape with earth (different types).
	Draw("Earth", cliffs);
	DrawMaterial("Earth-earth_root", cliffs, [6, 2], 20);
	DrawMaterial("Earth-earth_spongy", cliffs, [6, 2], 20);
	
	// Construct the cliffs substracting a border.
	var cliffs_border = {Algo = MAPALGO_Border, Wdt = 3, Op = cliffs};
	var cliffs_noborder = {Algo = MAPALGO_And, Op = [cliffs, {Algo = MAPALGO_Not, Op = cliffs_border}]};
	
	// Fill the bottom of all the cliffs with materials.
	var cliffs_bottom = {Algo = MAPALGO_And, Op = [cliffs_noborder, {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - 28, Wdt = map.Wdt, Hgt = 28}]};
	DrawMaterial("Granite", cliffs_bottom, [6, 3], 6);
	DrawMaterial("Gold", cliffs_bottom, [6, 3], 6);
	DrawMaterial("Tunnel", cliffs_bottom, [6, 6], 6);
	DrawMaterial("Firestone", cliffs_bottom, [6, 3], 12);	
	DrawMaterial("Rock-rock", cliffs_bottom, [6, 3], 20);
	DrawMaterial("Rock-rock_smooth", cliffs_bottom, [6, 3], 20);
	
	// Fill the middle of all the cliffs with materials.
	var cliffs_middle = {Algo = MAPALGO_And, Op = [cliffs_noborder, {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - 54, Wdt = map.Wdt, Hgt = 26}]};
	DrawMaterial("Ore", cliffs_middle, [6, 3], 12);
	DrawMaterial("Coal", cliffs_middle, [6, 3], 14);
	DrawMaterial("Tunnel", cliffs_middle, [6, 6], 6);
	DrawMaterial("Firestone", cliffs_middle, [6, 3], 8);	
	DrawMaterial("Rock-rock", cliffs_middle, [6, 3], 6);
	DrawMaterial("Rock-rock_smooth", cliffs_middle, [6, 3], 6);
	
	// Fill the top of all the cliffs with materials.
	var cliffs_top = {Algo = MAPALGO_And, Op = [cliffs_noborder, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = map.Wdt, Hgt = map.Hgt - 54}]};
	DrawMaterial("Tunnel", cliffs_top, [6, 6], 6);
	DrawMaterial("Tunnel", cliffs_top, [3, 3], 6);
	DrawMaterial("Rock-rock", cliffs_top, [6, 3], 6);
	DrawMaterial("Rock-rock_smooth", cliffs_middle, [6, 3], 6);
	
	// The base of the cliff is made out of granite.
	var cliff_base = {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - 8, Wdt = map.Wdt, Hgt = 8};
	cliff_base = {Algo = MAPALGO_Or, Op = [cliff_base, {Algo = MAPALGO_Turbulence, Iterations = 3, Amplitude = [6, 16], Scale = [6, 12], Seed = Random(65536), Op = cliff_base}]};
 	Draw("Granite", cliff_base);	
 	DrawMaterial("Rock-rock", cliff_base, [6, 2], 20);
	DrawMaterial("Rock-rock_smooth", cliff_base, [6, 2], 20);
	
	// Construct brick platforms on either side for a small village.
	var x = 10;
	var height = 0;
	while(!GetPixel(x, height) && height < map.Hgt)
    	height++;
    var brick = {Algo = MAPALGO_Rect, X = 0, Y = height, Wdt = 32, Hgt = 2};
    brick = {Algo = MAPALGO_Or, Op = [brick, {Algo = MAPALGO_Turbulence, Iterations = 2, Amplitude = [0, 8], Scale = [0, 8], Seed = Random(65536), Op = brick}]};
    Draw("Brick", brick); 
	Draw("Sky", {Algo = MAPALGO_Rect, X = 0, Y = height - 8, Wdt = 36, Hgt = 8}); 
	var x = map.Wdt - 10;
	var height = 0;
	while(!GetPixel(x, height) && height < map.Hgt)
    	height++;
    var brick = {Algo = MAPALGO_Rect, X = map.Wdt - 32, Y = height, Wdt = 32, Hgt = 2};
    brick = {Algo = MAPALGO_Or, Op = [brick, {Algo = MAPALGO_Turbulence, Iterations = 2, Amplitude = [0, 8], Scale = [0, 8], Seed = Random(65536), Op = brick}]};
    Draw("Brick", brick); 
	Draw("Sky", {Algo = MAPALGO_Rect, X = map.Wdt - 36, Y = height - 8, Wdt = 36, Hgt = 8});
	
	// Construct some very small sky islands.
	var island_layer = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Not, Op = cliffs}, {Algo = MAPALGO_Rect, X = 0, Y = 3 * map.Hgt / 7 + 9, Wdt = map.Wdt, Hgt = 26}]};
	var island_layer_border = {Algo = MAPALGO_Border, Left = 4, Right = 4, Op = island_layer};
	island_layer = {Algo = MAPALGO_And, Op = [island_layer, {Algo = MAPALGO_Not, Op = island_layer_border}]};
	var islands = {Algo = MAPALGO_And, Op = [island_layer, {Algo = MAPALGO_RndChecker, Seed = Random(65536), Wdt = 3, Hgt = 3, Ratio = 5}]};
	islands = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = 18, Scale = 18, Seed = Random(65536), Op = islands};
	Draw("Earth", islands);
	DrawMaterial("Earth-earth_root", islands, [6, 2], 20);
	DrawMaterial("Earth-earth_spongy", islands, [6, 2], 20);
	
	// Make a bat cave in one of the middle cliffs.
	for (var tries = 0; tries < 500; tries++)
	{
		var pos = {};
		if (!map->FindPosition(pos, "Solid", [140, map.Hgt - 46, map.Wdt - 240, 22]))
			continue;
		var correct_pos = true;
		var try_positions = [[0, 8], [0, -8], [10, 0], [-10, 0]];
		for (var try_pos in try_positions)
		{
			var pix = GetPixel(pos.X + try_pos[0], pos.Y + try_pos[1]);
			if (pix == 0 || pix == GetMaterialTextureIndex("Tunnel"))
			{
				correct_pos = false;
				continue;
			}
		}
		if (!correct_pos)
			continue;
		bat_cave = [pos.X, pos.Y];
		break;
	}
	var cave = {Algo = MAPALGO_Ellipse, X = bat_cave[0], Y = bat_cave[1], Wdt = 4, Hgt = 5};
	cave = {Algo = MAPALGO_Or, Op = [cave, {Algo = MAPALGO_Turbulence, Iterations = 2, Amplitude = [0, 8], Scale = [0, 8], Seed = Random(65536), Op = {Algo = MAPALGO_Rect, X = bat_cave[0] - 8, Y = bat_cave[1] - 4, Wdt = 16, Hgt = 8}}]};
	var cave_border = {Algo = MAPALGO_Border, Bottom = -2, Op = cave};
	Draw("Tunnel", cave);
	Draw("Rock-rock", cave_border);
	
	// Return true to tell the engine a map has been successfully created.
	return true;	
}
