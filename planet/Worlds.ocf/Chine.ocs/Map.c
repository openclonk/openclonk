/**
	Chine
	A chine hosting a waterfall and lots of vegetation.
	
	@author Maikel
*/

#include Library_Map


// Scenario properties which can be set later by the lobby options.
static const SCENOPT_MapSize = 1;

// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Retrieve the settings according to the MapSize setting.
	var map_size;
	if (SCENOPT_MapSize == 1)
		map_size = [48, 240]; 
	if (SCENOPT_MapSize == 2)
		map_size = [48, 300];
	if (SCENOPT_MapSize == 3)
		map_size = [48, 360];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	// Draw the chine.
	var chine = DrawChine(map);
	
	// Draw a small starting cave.
	DrawStartCave(map, chine);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Draws the chine.
public func DrawChine(proplist map)
{
	var wdt = map.Wdt;
	var hgt = map.Hgt;
	var granite_wdt = 4;
	var side_wdt = 14;
	    
	// Construct the chine sides.
	var left = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = side_wdt, Hgt = hgt};
	var right = {Algo = MAPALGO_Rect, X = wdt - side_wdt, Y = 0, Wdt = side_wdt, Hgt = hgt};
	var chine = {Algo = MAPALGO_Or, Op = [left, right]};
	var chine_rnd1 = {Algo = MAPALGO_Turbulence, Amplitude = 20, Scale = 4, Iterations = 4, Seed = Random(65536), Op = chine};
	var chine_rnd2 = {Algo = MAPALGO_Turbulence, Amplitude = 20, Scale = 2, Iterations = 4, Seed = Random(65536), Op = chine};
	chine = {Algo = MAPALGO_Or, Op = [chine, chine_rnd1, chine_rnd2]};
	// Draw the material for the sides.
	map->Draw("Earth", chine);
	map->DrawMaterial("Earth-earth_rough", chine, 2, 20);
	map->DrawMaterial("Earth-earth_dry", chine, 2, 20);
	map->DrawMaterial("Earth-earth_midsoil", chine, 4, 18);
	map->DrawMaterial("Granite", chine, 3, 16);
	map->DrawMaterial("Tunnel", chine, 2, 24);
	map->DrawMaterial("Rock-rock_cracked", chine, 3, 14);
	map->DrawMaterial("Rock", chine, 3, 14);
	map->DrawMaterial("Ore", chine, 3, 14);
	map->DrawMaterial("Firestone", chine, 3, 10);
	map->DrawMaterial("Coal", chine, 3, 14);
	// Draw the gold more at the top than bottom.
	var sides1 = {Algo = MAPALGO_And, Op = [chine, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = wdt, Hgt = hgt / 4}]};
	var sides2 = {Algo = MAPALGO_And, Op = [chine, {Algo = MAPALGO_Rect, X = 0, Y = hgt / 4, Wdt = wdt, Hgt = hgt / 4}]};
	var sides3 = {Algo = MAPALGO_And, Op = [chine, {Algo = MAPALGO_Rect, X = 0, Y = hgt / 2, Wdt = wdt, Hgt = hgt / 4}]};
	var sides4 = {Algo = MAPALGO_And, Op = [chine, {Algo = MAPALGO_Rect, X = 0, Y = 3 * hgt / 4, Wdt = wdt, Hgt = hgt / 4}]};
	map->DrawMaterial("Gold", sides1, 3, 3);
	map->DrawMaterial("Gold", sides2, 3, 2);
	map->DrawMaterial("Gold", sides3, 2, 2);
	map->DrawMaterial("Gold", sides4, 2, 1);
	
	// Construct an inside border.
	var border = {Algo = MAPALGO_Border, Left = 1, Right = 1, Op = chine};
	border = {Algo = MAPALGO_And, Op = [border, {Algo = MAPALGO_Rect, X = side_wdt / 2, Y = 0, Wdt = wdt - side_wdt, Hgt = hgt}]};
	// Draw the border.
	map->Draw("Granite", border);
	map->DrawMaterial("Tunnel", border, 2, 30);
	map->DrawMaterial("Rock-rock_cracked", border, 3, 20);
	map->DrawMaterial("Rock", border, 3, 20);
	
	// The outsides of the map are covered with granite.
	var granite = {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = granite_wdt, Y = 0, Wdt = wdt - 2 * granite_wdt, Hgt = hgt}};
	var granite_rnd1 = {Algo = MAPALGO_Turbulence, Amplitude = 4, Scale = 4, Iterations = 4, Seed = Random(65536), Op = granite};
	var granite_rnd2 = {Algo = MAPALGO_Turbulence, Amplitude = 4, Scale = 2, Iterations = 4, Seed = Random(65536), Op = granite};
	granite = {Algo = MAPALGO_Or, Op = [granite, granite_rnd1, granite_rnd2]};
	map->Draw("Granite", granite);
	map->DrawMaterial("Rock-rock_cracked", granite, 3, 10);
	map->DrawMaterial("Rock", granite, 3, 10);
	
	// Clear the top of the chine from material.
	var top = {Algo = MAPALGO_Rect, X = side_wdt, Y = 0, Wdt = wdt - 2 * side_wdt, Hgt = 2};
	map->Draw("Sky", top);
	return {Algo = MAPALGO_Or, Op = [chine, border]};
}

// Draws a small cave where the players start.
public func DrawStartCave(proplist map, proplist chine)
{
	var wdt = map.Wdt;
	var hgt = map.Hgt;
	
	var cave = {Algo = MAPALGO_Rect, X = 6, Y = hgt - 15, Wdt = 15, Hgt = 5};
	cave = {Algo = MAPALGO_And, Op = [cave, chine]};
	map->Draw("Tunnel", cave);
	map->DrawMaterial("Tunnel-brickback", cave, 3, 10);
	var cave_bottom = {Algo = MAPALGO_Border, Bottom = -1, Op = cave};
	cave_bottom = {Algo = MAPALGO_And, Op = [cave_bottom, {Algo = MAPALGO_Rect, X = 6, Y = hgt - 10, Wdt = 10, Hgt = 2}]};
	map->Draw("Granite", cave_bottom);
	return;
}

// Draws some material inside an island.
public func DrawMaterial(string mat, proplist onto_mask, int speck_size, int ratio)
{
	if (!speck_size)
		speck_size = 4;
	if (!ratio)
		ratio = 15;
	// Use random checker algorithm to draw patches of the material. 
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = ratio, Wdt = speck_size, Hgt = speck_size};
	rnd_checker = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = rnd_checker};
	var algo = {Algo = MAPALGO_And, Op = [onto_mask, rnd_checker]};
	Draw(mat, algo);
	
	return;
}
