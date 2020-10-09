/**
	Sandbox
	Map drawing for sandbox scenario.

	@author K_Pone, Maikel
*/

#include Library_Map

// Called be the engine: draw the complete map here.
public func InitializeMap(proplist map)
{
	// Initialize the map settings from scenario parameters. If a map is created in god mode
	// these settings are already up to date.
	InitMapSettings();
	
	// Resize the map.
	Resize(Settings_MapWdt, Settings_MapHgt);
	
	// Draw empty map according to type setting.
	if (Settings_MapType == CSETTING_MapType_Empty)
		return true;
	
	// Create the main surface: a rectangle with some turbulence on top.
	var height = [9 * map.Hgt / 20, 9 * map.Hgt / 20, 11 * map.Hgt / 20][Settings_MapType - 1];
	var amplitude = [[0, 10], [0, 18], [0, 40]][Settings_MapType - 1];
	var scale = [[0, 10], [0, 16], [0, 24]][Settings_MapType - 1];
	var rect = {X = 0, Y = height, Wdt = map.Wdt,  Hgt = map.Hgt - height};
	var surface = {Algo = MAPALGO_Rect, X = rect.X, Y = rect.Y, Wdt = rect.Wdt, Hgt = 8 * rect.Hgt / 6};
	surface = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = amplitude, Scale = scale, Seed = Random(65536), Op = surface};
	
	// Draw materials inside the main surface.
	DrawMaterials(rect, surface);
	
	// Draw some sky islands.
	for (var x = RandomX(30, 60); x < map.Wdt - 30; x += RandomX(45, 85))
		DrawSkyIsland(map, x, RandomX(32, 52), RandomX(20, 30), RandomX(18, 24)); 
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}


// Draws materials on the given surface.
public func DrawMaterials(proplist rect, proplist surface)
{
	var mask;
	var x = rect.X;
	var y = rect.Y;
	var wdt = rect.Wdt;
	var hgt = rect.Hgt;
	
	// Earth forms the basis.
	Draw("Earth", surface);	
	
	// A bit of different types of earth all around the surface.
	DrawMaterialSlab("Earth-earth", surface, y, hgt, 4, 12);
	DrawMaterialSlab("Earth-earth_root", surface, y, hgt, 2, 16);
	DrawMaterialSlab("Earth-earth_spongy", surface, y, hgt, 2, 16);
	DrawMaterialSlab("Earth-earth", surface, y, hgt, 4, 12);
	
	// Basic materials.
	DrawMaterialSlab("Rock", surface, y, hgt, 2, 8);
	DrawMaterialSlab("Tunnel", surface, y, hgt, 6, 3);
	DrawMaterialSlab("Ore", surface, y + hgt / 6, hgt / 2, 8, 10);
	DrawMaterialSlab("Coal", surface, y + hgt / 7, hgt / 2, 8, 10);
	DrawMaterialSlab("Firestone", surface, y + hgt / 6, hgt / 2, 6, 6);
	DrawMaterialSlab("Tunnel", surface, y + hgt / 7, hgt / 4, 12, 8);
	DrawMaterialSlab("Tunnel", surface, y + 2 * hgt / 3, hgt / 4, 8, 12);
	DrawMaterialSlab("Water", surface, y + hgt / 3, hgt / 3, 8, 10);
	
	// Valuable materials in the bottom layer.
	DrawMaterialSlab("Firestone", surface, y + 3 * hgt / 5, 2 * hgt / 5, 6, 2);
	DrawMaterialSlab("Rock", surface, y + 3 * hgt / 5, 2 * hgt / 5, 6, 14);
	DrawMaterialSlab("Granite", surface, y + 3 * hgt / 5, 2 * hgt / 5, 6, 12);	
	DrawMaterialSlab("Tunnel", surface, y + 3 * hgt / 5, 2 * hgt / 5, 10, 8);
	DrawMaterialSlab("Gold", surface, y + 2 * hgt / 3, hgt / 4, 3, 6);
	DrawMaterialSlab("Gold", surface, y + 3 * hgt / 4, hgt / 4, 6, 6);
	DrawMaterialSlab("Ruby", surface, y + 4 * hgt / 5, hgt / 5, 6, 4);
	DrawMaterialSlab("Amethyst", surface, y + 4 * hgt / 5, hgt / 5, 6, 4);

	// Draw the surface layer according to map type.
	var border_height = [4, 6, 12][Settings_MapType - 1];
	var border = {Algo = MAPALGO_Border, Top = border_height, Op = surface};
	Draw("Earth", border);
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = 30, Wdt = 2, Hgt = 2};
	var rnd_border = {Algo = MAPALGO_And, Op = [border, rnd_checker]};
	Draw(["Sand", "Rock", "Granite"][Settings_MapType - 1], rnd_border);
	Draw(["Earth-earth_root", "Rock-rock_smooth", "Rock"][Settings_MapType - 1], rnd_border);
	if (Settings_MapType == CSETTING_MapType_MapTypeMountains)
		Draw("Everrock", rnd_border);
	return;
}


public func DrawSkyIsland(proplist map, int x, int y, int wdt, int hgt)
{
	// An island is just an ellipse with turbulence.
	var island = {Algo = MAPALGO_Ellipse, X = x, Y = y, Wdt = wdt / 2, Hgt = hgt / 2};
	island = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = [8, 18], Seed = Random(65536), Op = island};
	Draw("Earth", island);
	
	// Overlay a set of materials inside the island.
	DrawMaterial("Earth-earth_root", island, 4, 30);
	DrawMaterial("Earth-earth", island, 3, 30);
	DrawMaterial("Tunnel", island, 3, 10);
	for (var mat in ["Coal", "Ore", "Firestone"])
	{
		DrawMaterial(mat, island, 4, 12);
		DrawMaterial(mat, island, 6, 6);
	}
		
	// Draw a top border out of sand and top soil.
	var sand_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Op = island, Top = [-1, 2]}, {Algo = MAPALGO_RndChecker, Ratio = 50, Wdt = 4, Hgt = 3}]};
 	var topsoil_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Op = island, Top = [-1, 3]}, {Algo = MAPALGO_RndChecker, Ratio = 40, Wdt = 4, Hgt = 2}]};
	Draw(["Sand", "Rock", "Granite"][Settings_MapType - 1], sand_border);
	Draw(["Earth-earth_root", "Rock-rock_smooth", "Rock"][Settings_MapType - 1], topsoil_border);	
	
	// Draw a bottom border out of granite and rock (or everrock on insane).
	var granite_border = {Algo = MAPALGO_Border, Op = island, Bottom = [-2, 3]};
	Draw(["Rock", "Granite", "Everrock"][Random(3)], granite_border);
	var rock_border = {Algo = MAPALGO_RndChecker, Ratio = 20, Wdt = 2, Hgt = 2};
	Draw(["Granite", "Rock", "Granite"][Random(3)], {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
	Draw(["Granite", "Rock", "Granite"][Random(3)], {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
	return;
}


/*-- Helper Functions --*/

public func DrawMaterialSlab(string mat, proplist mask, int y, int hgt, int spec_size, int ratio)
{
	var slab = {Algo = MAPALGO_Rect, X = this.X,  Y = y, Wdt = this.Wdt, Hgt = hgt};
	slab = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = slab};
	slab = {Algo = MAPALGO_And, Op = [slab, mask]}; 
	DrawMaterial(mat, slab, spec_size, ratio);
	return;
}
