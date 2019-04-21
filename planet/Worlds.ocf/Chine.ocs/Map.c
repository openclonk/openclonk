/**
	Chine
	A chine hosting a waterfall and lots of vegetation. The chine has
	several obstacles, like small lakes or empty vertical caves. The
	difficulty level affects the amount of granite in the map and the
	amount of background material in the middle of the chine.
	
	@author Maikel
*/

#include Library_Map


// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Retrieve the settings according to the MapSize setting.
	var map_size;
	if (SCENPAR_MapSize == 1)
		map_size = [48, 240]; 
	if (SCENPAR_MapSize == 2)
		map_size = [48, 300];
	if (SCENPAR_MapSize == 3)
		map_size = [48, 360];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	// Draw the chine.
	var chine = DrawChine(map, 2 + SCENPAR_MapSize, SCENPAR_Difficulty);
	
	// Draw a small starting cave.
	DrawStartCave(map, chine);
	
	// Draw the top of the chine with waterfall and exit.
	DrawChineTop(map);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Draws the chine.
public func DrawChine(proplist map, int nr_hurdles, int difficulty)
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
	map->DrawMaterial("Earth-earth_root", chine, 2, 20);
	map->DrawMaterial("Earth-earth_spongy", chine, 2, 20);
	map->DrawMaterial("Earth-earth", chine, 4, 18);
	map->DrawMaterial("Granite", chine, 3, 12 + 4 * difficulty);
	map->DrawMaterial("Tunnel", chine, 2, 24);
	map->DrawMaterial("Rock-rock", chine, 3, 14);
	map->DrawMaterial("Rock", chine, 3, 14);
	map->DrawMaterial("Ore", chine, 3, 14);
	map->DrawMaterial("Firestone", chine, 3, 10);
	map->DrawMaterial("Coal", chine, 3, 14);
	// Draw the gold more at the top than bottom.
	var sides1 = {Algo = MAPALGO_And, Op = [chine, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = wdt, Hgt = hgt / 4}]};
	var sides2 = {Algo = MAPALGO_And, Op = [chine, {Algo = MAPALGO_Rect, X = 0, Y = hgt / 4, Wdt = wdt, Hgt = hgt / 4}]};
	var sides3 = {Algo = MAPALGO_And, Op = [chine, {Algo = MAPALGO_Rect, X = 0, Y = hgt / 2, Wdt = wdt, Hgt = hgt / 4}]};
	var sides4 = {Algo = MAPALGO_And, Op = [chine, {Algo = MAPALGO_Rect, X = 0, Y = 3 * hgt / 4, Wdt = wdt, Hgt = hgt / 4}]};
	map->DrawMaterial("Gold", sides1, 3, 6);
	map->DrawMaterial("Gold", sides2, 3, 4);
	map->DrawMaterial("Gold", sides3, 2, 3);
	map->DrawMaterial("Gold", sides4, 2, 2);
	
	// Construct an inside border.
	var border = {Algo = MAPALGO_Border, Left = 1, Right = 1, Op = chine};
	border = {Algo = MAPALGO_And, Op = [border, {Algo = MAPALGO_Rect, X = side_wdt / 2, Y = 0, Wdt = wdt - side_wdt, Hgt = hgt}]};
	// Draw the border.
	map->Draw("Granite", border);
	map->DrawMaterial("Tunnel", border, 2, 36 - 6 * difficulty);
	map->DrawMaterial("Rock-rock", border, 3, 24 - 4 * difficulty);
	map->DrawMaterial("Rock", border, 3, 24 - 4 * difficulty);
	// Parts of this border, which covers the middle section are overground materials.
	// This achieved by double drawing parts of the border as overgroud material.
	var overground = {Algo = MAPALGO_Rect, X = side_wdt, Y = 0, Wdt = wdt - 2 * side_wdt, Hgt = hgt};
	var rand_checker = {Algo = MAPALGO_RndChecker, Seed = Random(65536), Ratio = 80 - 20 * difficulty, Wdt = 8, Hgt = 4};
	var border_overground = {Algo = MAPALGO_And, Op = [border, overground, rand_checker]};
	map->Draw("^Granite", border_overground);
	map->DrawMaterial("Tunnel", border_overground, 2, 30);
	map->DrawMaterial("^Rock-rock", border_overground, 3, 20);
	map->DrawMaterial("^Rock", border_overground, 3, 20);
	
	// Empty out some parts of the chine to provide more serious climbing hurdles.
	var hurdles = [1, 2, 3], add = [2, 3];
	ShuffleArray(hurdles);
	ShuffleArray(add);
	hurdles = Concatenate(hurdles, add);
	for (var i = 0; i < nr_hurdles; i++)
	{
		var y = (i + 1) * hgt / (1 + nr_hurdles) + RandomX(-6, 6); 
		DrawHurdle(map, hurdles[i], y, side_wdt);
	}
	
	// The outsides of the map are covered with granite.
	var granite = {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = granite_wdt, Y = 0, Wdt = wdt - 2 * granite_wdt, Hgt = hgt}};
	var granite_rnd1 = {Algo = MAPALGO_Turbulence, Amplitude = 4, Scale = 4, Iterations = 4, Seed = Random(65536), Op = granite};
	var granite_rnd2 = {Algo = MAPALGO_Turbulence, Amplitude = 4, Scale = 2, Iterations = 4, Seed = Random(65536), Op = granite};
	granite = {Algo = MAPALGO_Or, Op = [granite, granite_rnd1, granite_rnd2]};
	map->Draw("Granite", granite);
	map->DrawMaterial("Rock-rock", granite, 3, 10);
	map->DrawMaterial("Rock", granite, 3, 10);
	
	return {Algo = MAPALGO_Or, Op = [chine, border]};
}

// Draws one of the four types of hurdles.
public func DrawHurdle(proplist map, int type, int y, int side_wdt)
{
	var wdt = map.Wdt;
	
	// Empty out part of the chine.
	if (type == 1)
	{
		var ellipsis = {Algo = MAPALGO_Ellipse, X = wdt / 2 + RandomX(-2, 2), Y = y, Wdt = wdt / 2 - side_wdt + RandomX(3, 5), Hgt = RandomX(8, 9)};
		ellipsis = {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 6, Iterations = 4, Seed = Random(65536), Op = ellipsis};
		map->Draw("Sky", ellipsis);
		var ellipsis_top = {Algo = MAPALGO_Border, Top = -2, Op = ellipsis};
		ellipsis_top = {Algo = MAPALGO_And, Op = [ellipsis_top, {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = wdt / 2 - 3, Y = y - 12, Wdt = 6, Hgt = 24}}]};
		map->Draw("Granite", ellipsis_top);
	}
	// Granite stripes as an obstacle.
	if (type == 2)
	{
		var rect = {Algo = MAPALGO_Rect, X = side_wdt, Y = y - 3, Wdt = wdt - 2 * side_wdt, Hgt = 6};
		rect = {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 6, Iterations = 4, Seed = Random(65536), Op = rect};
		var stripes = {Algo = MAPALGO_And, Op = [rect, {Algo = MAPALGO_Lines, X = 1, Y = 0, Distance = 3}]};
		var sky = {Algo = MAPALGO_And, Op = [rect, {Algo = MAPALGO_Not, Op = stripes}]};
		map->Draw("Sky", sky);
		map->Draw("Granite", stripes);
		map->DrawMaterial("Rock-rock", stripes, 3, 10);
		map->DrawMaterial("Rock", stripes, 3, 10);
	}
	// A basin with water, shifted a bit to the left or right.
	if (type == 3)
	{
		var shift = (2 * Random(2) - 1) * 8;
		var ellipsis = {Algo = MAPALGO_Ellipse, X = wdt / 2 + shift, Y = y, Wdt = 8, Hgt = RandomX(5, 6)};
		ellipsis = {Algo = MAPALGO_Turbulence, Amplitude = 4, Scale = 4, Iterations = 2, Seed = Random(65536), Op = ellipsis};
		var top = {Algo = MAPALGO_And, Op = [ellipsis, {Algo = MAPALGO_Rect, X = 0, Y = y - 18, Wdt = wdt, Hgt = 18}]};
		var bottom = {Algo = MAPALGO_And, Op = [ellipsis, {Algo = MAPALGO_Rect, X = 0, Y = y, Wdt = wdt, Hgt = 18}]};
		map->Draw("Sky", top);
		map->Draw("Water", bottom);
		var border = {Algo = MAPALGO_Border, Wdt = [-1, 1], Op = ellipsis};
		border = {Algo = MAPALGO_And, Op = [border, {Algo = MAPALGO_Rect, X = 0, Y = y, Wdt = wdt, Hgt = 18}]};
		map->Draw("Granite", border);
		map->DrawMaterial("Rock-rock", border, 1, 10);
		map->DrawMaterial("Rock", border, 1, 10);
	}
	return;
}

// Draws a small cave where the players start.
public func DrawStartCave(proplist map, proplist chine)
{
	var hgt = map.Hgt;
	
	// Draw the start cave, with granite floor.
	var cave = {Algo = MAPALGO_Rect, X = 6, Y = hgt - 15, Wdt = 15, Hgt = 5};
	cave = {Algo = MAPALGO_And, Op = [cave, chine]};
	map->Draw("Tunnel", cave);
	map->DrawMaterial("Tunnel-brickback", cave, 3, 10);
	var cave_bottom = {Algo = MAPALGO_Border, Bottom = -1, Op = cave};
	cave_bottom = {Algo = MAPALGO_And, Op = [cave_bottom, {Algo = MAPALGO_Rect, X = 6, Y = hgt - 10, Wdt = 10, Hgt = 2}]};
	map->Draw("Granite", cave_bottom);
	return;
}

// Draws the top of the chine with waterfall and exit.
public func DrawChineTop(proplist map, proplist chine)
{
	var wdt = map.Wdt;
	var granite_wdt = 4;
	var side_wdt = 14;

	// Clear the top and the right of the chine from material.
	var top = {Algo = MAPALGO_Rect, X = side_wdt, Y = 0, Wdt = wdt - 2 * side_wdt, Hgt = 5};
	map->Draw("Sky", top);
	var right = {Algo = MAPALGO_Polygon, X = [wdt - side_wdt, wdt - granite_wdt, wdt - side_wdt], Y = [0, 0, 5]};
	map->Draw("Sky", right);
	return;
}
