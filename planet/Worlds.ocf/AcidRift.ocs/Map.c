/**
	Acid Rift
	Mine the rubies before it's too late
	
	@author Sven2
*/


#include Library_Map

//static const SCENPAR_MapSize = 2;
//static const SCENPAR_Difficulty = 2;

local top_off = 20;      // top open area of basin
local bottom_off = 20;   // bottom granite area of basin
local border_width = 10; // left and right border width
local bottom_indent = 5; // diagonal indent at bottom left and bottom right corners
local top_off_earth = 50;// top offset from which earth and materials can start
local bottom_off_earth = 50;// bottom offset from which earth and materials can start

static g_start_map_x, g_start_map_y;

// Called be the engine: draw the complete map here.
func InitializeMap(proplist map)
{
	var s = SCENPAR_MapSize - 1;
	var d = SCENPAR_Difficulty - 1;
	// Map size
	var map_size = [[100, 200],[120, 250],[130, 400]][s];
	Resize(map_size[0], map_size[1]);
	
	// Background sky to ensure "^*" can be used to place materials
	Draw("Sky");
	
	// Parameters by map size and difficulty
	var acid_level = [10, 30, 40][d];	 // filling (in %) of basin with acid
	var num_earth = [15, 20, 30][s];
	var num_coal = [12, 15, 20][s];
	var num_ore = [4, 5, 8][s];
	var num_firestone = [15, 20, 30][s];
	var num_water = [4, 5, 8][s];
	var num_granite = [80, 100, 120][s];
	var num_granite_top = [15, 20, 25][s];
	var num_rock = [35, 40, 60][s];
	var num_gold = [15, 18, 25][s];
	var num_gems = 100;
	var num_acidsurround = [20, 25, 40][s];
	var size_earth = [[16, 12, 8][d], 8];
	var size_coal = [6, 3];
	var size_ore = [4, 4];
	var size_water = [4, 4];
	var size_firestone = [3, 3];
	var size_granite = [2, 1];
	var size_granite_top = [[8, 4, 1][d], 2];
	var size_rock = [3, 3];
	var size_gold = [5, 4, 3][d];
	var size_gems = [5, 3, 2][d];
	var size_gem_cave = 10;
	var size_acidsurround = [[5, 4], [6, 5], [7, 5]][s];
	
	// Draw starting acid filling
	var acid_area = [0, top_off + (100-acid_level) * (this.Hgt-top_off-bottom_off) / 100, this.Wdt, this.Hgt];
	Draw("^Acid", nil, acid_area);
	
	// Basin surrounded by granite
	var surround_poly = [
		[-50, top_off],
		[border_width, top_off],
		[border_width + bottom_indent, this.Hgt*3/4],
		[this.Wdt/2, this.Hgt-bottom_off],
		[this.Wdt-border_width-bottom_indent, this.Hgt*3/4],
		[this.Wdt-border_width, top_off],
		[this.Wdt + 50, top_off],
		[this.Wdt + 50, this.Hgt + 50],
		[-50, this.Hgt + 50]
	];
	surround_poly = TransposeArray(surround_poly);
	var algo = {Algo = MAPALGO_Polygon, 
		X = surround_poly[0],
		Y = surround_poly[1]};
	algo = { Algo = MAPALGO_Turbulence, Op = algo, Amplitude = 10, Scale = 30 };
	algo = { Algo = MAPALGO_Turbulence, Op = algo, Amplitude = 20, Scale = 5 };
	var basin = CreateLayer();
	basin->Draw("Granite", algo);
	basin->DrawMaterial("Rock", basin, [10, 4], 30);
	
	// Bottom acid cave containing gems
	algo = { Algo = MAPALGO_Ellipse, X = this.Wdt/2, Y = this.Hgt-bottom_off, Wdt = size_gem_cave, Hgt = size_gem_cave };
	algo = { Algo = MAPALGO_Turbulence, Op = algo, Amplitude = 20, Scale = 20 };
	basin->Draw("Transparent", algo);
	
	// Materials areas are in surrounding basin
	var basin_materials = CreateLayer();
	basin_materials->Draw("Snow", {Algo = MAPALGO_Border, Wdt=-5, Op = basin});
	
	// Acid caves in lower area
	acid_area[1] += 20;
	basin->DrawPatches(num_acidsurround, "Acid", "Granite", size_acidsurround, false, 20, acid_area);
	
	// Gold in upper area
	var gold_area = [0, top_off_earth, this.Wdt, this.Hgt-bottom_off-top_off_earth-size_gem_cave];
	basin->DrawPatches(num_gold, "Gold", basin_materials, size_gold, false, 10, gold_area);
	
	// Gems in lower area	
	var gem_area = [0, this.Hgt-bottom_off-size_gem_cave, this.Wdt, this.Hgt];
	var i = 0;
	while (basin->GetPixelCount("Ruby")+basin->GetPixelCount("Amethyst") < num_gems)
	{
		basin->DrawPatch(["Ruby", "Amethyst"][++i%2], basin_materials, size_gems, false, 6, gem_area);
	}
	
	// Initial basin blit to ensure earth is not drawn there
	Blit(basin);
	
	// Small patches of rock and granite
	DrawPatches(num_granite, "Granite", "^*", size_granite, false, 3);
	DrawPatches(num_rock, "Rock", "^*", size_rock, false, 3);
	
	// Alternate rock textures
	var rock_area = Duplicate("Rock");
	DrawMaterial("Rock", rock_area, [3, 3], 50);
	
	// Patches of earth
	DrawPatches(num_earth, "Earth", "^*", size_earth, true, 20, [0, top_off_earth, this.Wdt, this.Hgt - top_off_earth - bottom_off_earth]);
	
	// Large patches of granite in top map area
	DrawPatches(num_granite_top, "Granite", "^*", size_granite_top, false, 3, [0, top_off/2, this.Wdt, this.Hgt/4]);
	
	// Empty starting area
	var start_x = this.Wdt/4 + Random(this.Wdt/2);
	var start_y = this.Hgt/3;
	var start_algo = {Algo = MAPALGO_Ellipse, X = start_x, Y = start_y, Wdt = 10, Hgt = 10};
	Draw("Sky", start_algo);
	g_start_map_x = start_x; g_start_map_y = start_y;
	
	// Materials
	DrawPatches(num_water, "Water", "Earth", size_water, false, 10);
	Draw("Earth-earth", {Algo = MAPALGO_Border, Op = Duplicate("Water")});
	DrawPatches(num_firestone, "Firestone", "Earth", size_firestone, false, 10);
	DrawPatches(num_coal, "Coal", "Earth", size_coal, false, 7);
	DrawPatches(num_ore, "Ore", "Earth", size_ore, true, 6);	
	
	// Re-draw basin to ensure it wasn't overdrawn by earth turbulence
	Blit(basin);
	
	// Draw starting area
	var algo = {Algo = MAPALGO_Ellipse, X = start_x, Y = start_y + 5, Wdt = 6, Hgt = 5};
	algo = {Algo = MAPALGO_Turbulence, Iterations = 1, Amplitude = 5, Scale = 5, Op = algo};
	Draw("Earth", algo);
	Draw("Sky", nil, [start_x-4, start_y-6, 8, 8]);
	algo = {Algo = MAPALGO_Polygon, X=[start_x-4, start_x + 4], Y=[start_y + 2, start_y + 2]};
	
	// Alternations in earth texture
	var earth_area = Duplicate("Earth");
	DrawMaterial("Earth-earth_root", earth_area, [10, 4], 30);
	DrawMaterial("Earth-earth", earth_area, [10, 4], 30);
	
	// Start area is always dark
	Draw("Earth-earth", algo);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}


/*-- Helper Functions --*/

// Find places matching mask and draw spots of material on it
private func DrawPatches(int num, ...)
{
	for (var i = 0; i<num; ++i) DrawPatch(...);
}

private func DrawPatch(string mat, mask, size, bool allow_rotation, int turbulence, array area)
{
	var pos = {};
	if (!area) area = [0, top_off, this.Wdt, this.Hgt];
	if (GetType(size) != C4V_Array) size = [size, size];
	var mask_layer = this;
	if (GetType(mask) != C4V_String)
	{
		mask_layer = mask;
		mask = "Snow";
	}
	if (!mask_layer->FindPosition(pos, mask, area)) return false;
	var algo = {Algo = MAPALGO_Ellipse, X = pos.X, Y = pos.Y, Wdt = size[0], Hgt = size[1]};
	algo = {Algo = MAPALGO_Turbulence, Iterations = 3, Amplitude = turbulence, Scale = 5, Op = algo};
	if (allow_rotation) algo = {Algo= MAPALGO_Rotate, R = Random(360), OffX = pos.X, OffY = pos.Y, Op = algo};
	return Draw(mat, algo);
}
