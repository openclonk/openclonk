/**
	Gem Grabbers
	Dynamic map with sky islands and hard to reach gems. There is a main island, with at the bottom gem stalactites
	and some smaller islands which also have gem stalactites hanging from the bottom. Each of the smaller islands
	have different materials.
	
	@author Maikel
*/

#include Library_Map


// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Retrieve the settings according to the MapSize setting.
	var map_size, main_size, nr_islands;
	if (SCENPAR_MapSize == 1)
	{
		map_size = [240, 200]; main_size = 80; nr_islands = RandomX(6, 7);
	}
	if (SCENPAR_MapSize == 2)
	{
		map_size = [280, 220]; main_size = 90; nr_islands = RandomX(7, 8);
	}
	if (SCENPAR_MapSize == 3)
	{
		map_size = [320, 240]; main_size = 100; nr_islands = RandomX(8, 9);
	}
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	// Draw the main island.
	map->DrawMainIsland(map, main_size);
	
	// Draw a number of sky islands at random positions, each with different resources.
	var resources = [["Coal", "Ore"], ["Coal", "Gold"], ["Firestone", "Coal"], ["Rock", "Ore"], ["Rock", "Coal", "Firestone"], ["Gold", "Ore", "Coal"]];
	while (nr_islands > 0)
	{
		var x = 12 + Random(map.Wdt - 24);
		var y = 26 + Random(map.Hgt - 52);
		var wdt = RandomX(35, 45);
		var hgt = wdt - RandomX(16, 22);
		// DrawIsland itself makes sure there is no other island nearby.
		if (DrawIsland(map, x, y, wdt, hgt, resources[Random(6)]))
			nr_islands--;
	}
	
	// Fix the liquid borders with a library function.
	FixLiquidBorders("Earth");
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Draws the main island with all basic resources
public func DrawMainIsland(proplist map, int size)
{
	// Shape of the main island.
	var island = {Algo = MAPALGO_Polygon};
	var x = map.Wdt / 2;
	var y = map.Hgt / 2;
	island.X = [x-size/3, x-size/2, x-size/3, x-size/6, x + size/6, x + size/3, x + size/2, x + size/3];
	island.Y = [y-size/6, y, y + size/3, y + size/6, y + size/6, y + size/3, y, y-size/6];
	
	// Draw the earth patch of the island.
	island = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = [10, 6], Seed = Random(65536), Op = island};
	Draw("Earth", island);
		
	// Overlay a set of materials inside the island.
	DrawIslandMat("Earth-earth_root", island, 4, 30);
	DrawIslandMat("Earth-earth", island, 3, 30);
	DrawIslandMat("Tunnel", island, 3, 10);
	DrawIslandMat("Water", island, 4, 8);
	DrawIslandMat("Gold", island, 3, 6);
	DrawIslandMat("Ore", island, 6, 12);
	DrawIslandMat("Firestone", island, 6, 12);
	DrawIslandMat("Coal", island, 6, 12);
	
	// Draw a top border out of sand and top soil.
	var sand_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Op = island, Top = [-1, 2]}, {Algo = MAPALGO_RndChecker, Ratio = 50, Wdt = 4, Hgt = 3}]};
 	var topsoil_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Op = island, Top = [-1, 3]}, {Algo = MAPALGO_RndChecker, Ratio = 40, Wdt = 4, Hgt = 2}]};
	Draw("Sand", sand_border);
	Draw("Earth-earth_root", topsoil_border);	
	
	// Draw a bottom border out of granite and rock (or everrock on insane).
	var granite_border = {Algo = MAPALGO_Border, Op = island, Bottom = [-4, 3]};
	Draw(["Rock", "Granite", "Everrock"][SCENPAR_Difficulty - 1], granite_border);
	var rock_border = {Algo = MAPALGO_RndChecker, Ratio = 25, Wdt = 2, Hgt = 2};
	Draw(["Granite", "Rock", "Rock"][SCENPAR_Difficulty - 1], {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
	Draw(["Granite", "Rock", "Granite"][SCENPAR_Difficulty - 1], {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
	
	// Draw some gems attached at the bottom of the island.
	var full_island = {Algo = MAPALGO_Or, Op = [island, granite_border]};
	var rect = [x - size / 2, y - size / 3, size / 2, 3 * size / 4];
	DrawGems("Ruby", rect, 3, map, full_island);
	rect = [x, y - size / 3, size / 2, 3 * size / 4];
	DrawGems("Amethyst", rect, 3, map, full_island);
	return;
}

// Draws a smaller island at the specified location if the location rectangle is free, returns whether succeeded.
public func DrawIsland(proplist map, int x, int y, int wdt, int hgt, array mats)
{
	// Don't draw an island if there is already something nearby.
	var rect = [x - wdt, y - 3 * hgt / 2, 2 * wdt, 3 * hgt];
	if (GetPixelCount("Solid", rect) > 0)
		return false;
	
	// An island is just an ellipse with turbulence.
	var island = {Algo = MAPALGO_Ellipse, X = x, Y = y, Wdt = wdt / 2, Hgt = hgt / 2};
	island = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = [6, 12], Seed = Random(65536), Op = island};
	Draw("Earth", island);
	
	// Overlay a set of materials inside the island.
	DrawIslandMat("Earth-earth_root", island, 4, 30);
	DrawIslandMat("Earth-earth", island, 3, 30);
	DrawIslandMat("Tunnel", island, 3, 10);
	for (var mat in mats)
	{
		DrawIslandMat(mat, island, 4, 18);
		DrawIslandMat(mat, island, 6, 12);
	}
		
	// Draw a top border out of sand and top soil.
	var sand_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Op = island, Top = [-1, 2]}, {Algo = MAPALGO_RndChecker, Ratio = 50, Wdt = 4, Hgt = 3}]};
 	var topsoil_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Op = island, Top = [-1, 3]}, {Algo = MAPALGO_RndChecker, Ratio = 40, Wdt = 4, Hgt = 2}]};
	Draw("Sand", sand_border);
	Draw("Earth-earth", topsoil_border);	
	
	// Draw a bottom border out of granite and rock (or everrock on insane).
	var granite_border = {Algo = MAPALGO_Border, Op = island, Bottom = [-2, 3]};
	Draw(["Rock", "Granite", "Everrock"][SCENPAR_Difficulty - 1], granite_border);
	var rock_border = {Algo = MAPALGO_RndChecker, Ratio = 20, Wdt = 2, Hgt = 2};
	Draw(["Granite", "Rock", "Granite"][SCENPAR_Difficulty - 1], {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
	Draw(["Granite", "Rock", "Granite"][SCENPAR_Difficulty - 1], {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
		
	// Draw some gems attached at the bottom of the island.
	var gem = ["Ruby", "Amethyst"][Random(2)];
	var full_island = {Algo = MAPALGO_Or, Op = [island, granite_border]};
	DrawGems(gem, rect, 3, map, full_island);
	return true;
}

// Looks for a location at the bottom of a sky islands and places some gems.
public func DrawGems(string gem_mat, array rect, int size, proplist map, proplist island)
{
	// Adapt rect to find at least a spot 8 * MapZoom pixel distance from the map borders.
	rect[2] -= Max(0, 8 - rect[0]); rect[0] = Max(8, rect[0]); // Left boundary.
	rect[2] = Min(rect[2], map.Wdt - rect[0] - 8); // Right boundary.
	
	var low_spot = {X = rect[0], Y = rect[1]};
	for (var i = 0; i < 250; i++)
	{
		var spot = {};
		this->FindPosition(spot, "Solid", rect);
		if (spot.Y > low_spot.Y)
		{
			low_spot = spot;
		}
	}
	
	// Sometimes draw a large patch of tunnel behind the gems.
	if (!Random(3))
	{
		var gem_tunnel = {Algo = MAPALGO_Ellipse, X = low_spot.X, Y = low_spot.Y + 2, Wdt = size * 3, Hgt = 2 * size};
		gem_tunnel = {Algo = MAPALGO_And, Op = [gem_tunnel, {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Lines, X = 1, Y = 0, OffX = Random(6), Distance = RandomX(4, 6)}}]};
		gem_tunnel = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = 14, Scale = 8, Seed = Random(65536), Op = gem_tunnel};
		gem_tunnel = {Algo = MAPALGO_And, Op = [gem_tunnel, {Algo = MAPALGO_Not, Op = island}]};
		Draw("Tunnel", gem_tunnel);
	}
	
	// Draw the gems.
	var gems = {Algo = MAPALGO_Ellipse, X = low_spot.X, Y = low_spot.Y + 1, Wdt = size - 1, Hgt = size};
	gems = {Algo = MAPALGO_Turbulence, Amplitude = 5, Scale = 5, Iterations = 2, Op = gems};
	Draw(gem_mat, gems);
	
	// Some rock/granite/everrock border above the gems to make it harder to reach.
	var gem_border = {Algo = MAPALGO_Border, Top = -4, Op = gems};
	Draw(["Rock", "Granite", "Everrock"][SCENPAR_Difficulty - 1], gem_border);
	return;
}

// Draws some material inside an island.
public func DrawIslandMat(string mat, proplist onto_mask, int speck_size, int ratio)
{
	if (!speck_size)
		speck_size = 3;
	if (!ratio)
		ratio = 20;
	// Use random checker algorithm to draw patches of the material. 
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = ratio, Wdt = speck_size, Hgt = speck_size};
	rnd_checker = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = rnd_checker};
	var mask_border = {Algo = MAPALGO_Border, Op = onto_mask, Wdt = 3};
	var algo = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Xor, Op = [onto_mask, mask_border]}, rnd_checker]};
	Draw(mat, algo);
	return;
}
