/**
	Iron Peak
	A mountain peak filled with iron ore, firestone and coal, the crust consists 
	of granite, rock, ice and some entrances.
	
	@authors Maikel
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
		map_size = [150, 150]; 
	if (SCENOPT_MapSize == 2)
		map_size = [150, 175];
	if (SCENOPT_MapSize == 3)
		map_size = [150, 200];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	
	// Draw the main surface: a mountain with the polygon algorithm.
	var wdt = map.Wdt;
	var hgt = map.Hgt;
	var x_points = [14 * wdt / 100, 48 * wdt / 100, 52 * wdt / 100, 86 * wdt / 100];
	var y_points = [hgt + 10, 12 * hgt / 100, 12 * hgt / 100, hgt + 10];
	var surface = {Algo = MAPALGO_Polygon, X = x_points, Y = y_points};
	surface = {Algo = MAPALGO_Turbulence, Seed = Random(65536), Op = surface, Amplitude = [25, 15], Scale = 10, Iterations = 3};	
	Draw("Earth", surface);	
	
	// Draw the inner materials of the mountain.
	DrawMountainMaterials(surface);
	
	// Draw a border on the mountain.
	DrawMountainBorder(surface);
		
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Draws the inner materials of the mountain.
public func DrawMountainMaterials(proplist surface)
{
	// Draw all the materials.
	DrawMaterial("Earth-earth_dry", surface);
	DrawMaterial("Earth-earth_rough", surface);
	DrawMaterial("Granite", surface);
	DrawMaterial("Rock-rock_cracked", surface);
	DrawMaterial("Snow-snow1", surface);
	DrawMaterial("Ice-ice3", surface);
	DrawMaterial("Rock-rock", surface);
	DrawMaterial("Tunnel", surface);
	DrawMaterial("Ore", surface);
	DrawMaterial("Firestone", surface, 3, 15);
	DrawMaterial("Coal", surface, 3, 15);
	
	// Draw some diagonal tunnels to allow the players to move.
	var lines = {Algo = MAPALGO_Lines, X = 3, Y = 0, Distance = 20};
	var right = {Algo = MAPALGO_Rotate, R = 45, Op = lines};
	var left = {Algo = MAPALGO_Rotate, R = -45, Op = lines};
	lines = {Algo = MAPALGO_Or, Op = [right, left]};
	lines = {Algo = MAPALGO_And, Op = [surface, lines]};
	lines = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = lines};
	Draw("Tunnel", lines);	
	return;
}

// Draws a border from granite, rock, etc. on the mountain.
public func DrawMountainBorder(proplist surface)
{
	var border = {Algo = MAPALGO_Border, Op = surface, Top = 5, Left = 5, Right = 5};
	DrawMaterial("Earth-earth_dry", border);
	DrawMaterial("Ice-ice3", border, 3, 20);
	DrawMaterial("Tunnel", border, 3, 20);
	DrawMaterial("Granite", border, 3, 25);
	DrawMaterial("Rock-rock", border, 2, 25);
	DrawMaterial("Rock-rock_cracked", border, 3, 30);
	return;
}

// Draws some material inside an island.
public func DrawMaterial(string mat, proplist onto_mask, int speck_size, int ratio)
{
	if (!speck_size)
		speck_size = 3;
	if (!ratio)
		ratio = 15;
	// Use random checker algorithm to draw patches of the material. 
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = ratio, Wdt = speck_size, Hgt = speck_size};
	rnd_checker = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = rnd_checker};
	var algo = {Algo = MAPALGO_And, Op = [onto_mask, rnd_checker]};
	Draw(mat, algo);
	
	return;
}
