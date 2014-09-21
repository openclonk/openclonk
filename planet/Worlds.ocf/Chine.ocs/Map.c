/**
	Chine
	Dynamic map a few layers of materials below a flat shaped earth surface.
	
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
		map_size = [40, 240]; 
	if (SCENOPT_MapSize == 2)
		map_size = [40, 300];
	if (SCENOPT_MapSize == 3)
		map_size = [40, 360];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	// Draw the chine.
	DrawChine(map);
	
	// Draw a small granite starting platform.
	var wdt = map.Wdt;
    var hgt = map.Hgt;
	var sky = {Algo = MAPALGO_Rect, X = wdt/2 - 4, Y = hgt - 15, Wdt = 8, Hgt = 6};
	var platform = {Algo = MAPALGO_Rect, X = wdt/2 - 3, Y = hgt - 10, Wdt = 6, Hgt = 1};
	map->Draw("Sky", sky);
	map->Draw("Granite", platform);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Draws the chine.
public func DrawChine(proplist map)
{
    var wdt = map.Wdt;
    var hgt = map.Hgt;
    
    // Construct the chine sides.
    var side_wdt = 10;    
    var left = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = side_wdt, Hgt = hgt};
    var right = {Algo = MAPALGO_Rect, X = wdt - side_wdt, Y = 0, Wdt = side_wdt, Hgt = hgt};
	var sides = {Algo = MAPALGO_Or, Op = [left, right]};
	var sides_rnd1 = {Algo = MAPALGO_Turbulence, Amplitude = 20, Scale = 4, Iterations = 4, Seed = Random(65536), Op = sides};
	var sides_rnd2 = {Algo = MAPALGO_Turbulence, Amplitude = 20, Scale = 2, Iterations = 4, Seed = Random(65536), Op = sides};
	sides = {Algo = MAPALGO_Or, Op = [sides, sides_rnd1, sides_rnd2]};
	
	// Draw the sides.
	map->Draw("Earth", sides);
	map->DrawMaterial("Earth-earth_rough", sides, 2, 20);
	map->DrawMaterial("Earth-earth_dry", sides, 2, 20);
	map->DrawMaterial("Earth-earth_midsoil", sides, 4, 18);
	map->DrawMaterial("Granite", sides, 3, 16);
	map->DrawMaterial("Tunnel", sides, 2, 24);
	map->DrawMaterial("Rock-rock_cracked", sides, 3, 14);
	map->DrawMaterial("Rock", sides, 3, 14);
	map->DrawMaterial("Gold", sides, 3, 2);
   	map->DrawMaterial("Ore", sides, 3, 14);
   	map->DrawMaterial("Firestone", sides, 3, 10);
   	map->DrawMaterial("Coal", sides, 3, 14);
	
	// Construct an inside border.
	var border = {Algo = MAPALGO_Border, Left = 1, Right = 1, Op = sides};
	border = {Algo = MAPALGO_And, Op = [border, {Algo = MAPALGO_Rect, X = side_wdt / 2, Y = 0, Wdt = wdt - side_wdt, Hgt = hgt}]};
	// Draw the border.
	map->Draw("Granite", border);
	map->DrawMaterial("Tunnel", border, 2, 30);
	map->DrawMaterial("Rock-rock_cracked", border, 3, 20);
	map->DrawMaterial("Rock", border, 3, 20);
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
