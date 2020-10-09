/**
	Krakatoa's Krach
	A Volcano with one main chasm and a few side chasms.
	
	@author Maikel
*/

#include Library_Map


// Variable to store the chasm exit points.
static chasm_exits;

// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Set chasm exits to be an array.
	chasm_exits = [];
	
	// Retrieve the settings according to the MapSize setting.
	var map_size;
	if (SCENPAR_MapSize == 1)
		map_size = [200, 150]; 
	if (SCENPAR_MapSize == 2)
		map_size = [240, 180];
	if (SCENPAR_MapSize == 3)
		map_size = [280, 210];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	// Get the volcano main shape.
	var volcano = CreateVolcano(map);
		
	// Get the main chasm shape.
	var main_chasm = CreateChasm(map, volcano);
	
	// Draw the various parts of the map.
	DrawVolcano(map, volcano, SCENPAR_Difficulty);
	DrawChasm(map, volcano, main_chasm);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Creates the main volcano shape.
public func CreateVolcano(proplist map)
{
	var wdt = map.Wdt;
	var hgt = map.Hgt;
    
	// Use the polygon algorithm to construct the main size of the volcano, and rescale according to map size.
	var x_points = [  0,   0, 32, 51, 67, 83, 88, 90, 92, 94, 106, 108, 110, 112, 117, 133, 149, 168, 200, 200];
	var y_points = [150, 100, 86, 75, 60, 38, 46, 48, 50, 51,  51,  50,  48,  46,  38,  60,  75,  86, 100, 150];
	for (var i = 0; i < GetLength(x_points); i++)
	{
		x_points[i] = x_points[i] * wdt / 200;
		y_points[i] = y_points[i] * hgt / 150;
	}	
	// Create the volcano layer.
	var volcano = {Algo = MAPALGO_Polygon, X = x_points, Y = y_points};
	volcano = {Algo = MAPALGO_Or, Op = [volcano, {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 6, Iterations = 2, Seed = Random(65536), Op = volcano}]};
    return volcano;
}

// Creates the chasm structure.
public func CreateChasm(proplist map, proplist volcano)
{
	var wdt = map.Wdt;
	var hgt = map.Hgt;

	// The main chasm ends in the crater and is a larger reservoir of lava at the bottom.
	var main_chasm = {Algo = MAPALGO_Rect, X = wdt / 2 - wdt / 40, Y = hgt / 3, Wdt = wdt / 20, Hgt = 2 * hgt / 3};
	
	// Complete the chasm layer, rescale for larger map sizes.
	var x_points = [ 73,  82,  94, 106, 118, 127];
	var y_points = [150, 125, 113, 113, 125, 150];
	for (var i = 0; i < GetLength(x_points); i++)
	{
		x_points[i] = x_points[i] * wdt / 200;
		y_points[i] = y_points[i] * hgt / 150;
	}
	var chasm_bottom = {Algo = MAPALGO_Polygon, X = x_points, Y = y_points}; 
	main_chasm = {Algo = MAPALGO_Or, Op = [main_chasm, chasm_bottom]};
	main_chasm = {Algo = MAPALGO_Or, Op = [main_chasm, {Algo = MAPALGO_Turbulence, Amplitude = 8, Scale = 8, Iterations = 2, Seed = Random(65536), Op = main_chasm}]};
	
	// Draw the four side chasms, rescale for larger map sizes.
	var chasm_points = [
		[[100, 53], [122, 70]], 
		[[100, 71], [92, 52]], 
		[[100, 129], [92, 52]], 
		[[100, 147], [122, 70]]
	];
	for (var i = 0; i < GetLength(chasm_points); i++)
	{
		chasm_points[i][0][0] = chasm_points[i][0][0] * wdt / 200;
		chasm_points[i][0][1] = chasm_points[i][0][1] * wdt / 200;
		chasm_points[i][1][0] = chasm_points[i][1][0] * hgt / 150;
		chasm_points[i][1][1] = chasm_points[i][1][1] * hgt / 150;
	}
	for (var i = 0; i < 4; i++)
	{
		var points = chasm_points[i];
		var side_chasm = {Algo = MAPALGO_Polygon, X = points[0], Y = points[1], Wdt = 3, Open = 1, Empty = 1};
		side_chasm = {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 6, Iterations = 2, Seed = Random(65536), Op = side_chasm};
		side_chasm = {Algo = MAPALGO_And, Op = [side_chasm, volcano]};
		main_chasm = {Algo = MAPALGO_Or, Op = [main_chasm, side_chasm]};
	}
	
	// Store chasm exits.
	chasm_exits[0] = [wdt / 2, hgt / 3];
	for (var i = 1; i <= 4; i++)
		chasm_exits[i] = [chasm_points[i - 1][0][1], chasm_points[i - 1][1][1]];
			
	return main_chasm;
}

// Draws the main volcano shape.
public func DrawVolcano(proplist map, proplist volcano, int difficulty)
{
	// The main material for the volcano is earth.
	map->Draw("Earth", volcano);
	
	// Also add other materials to the volcano.
	map->DrawMaterial("Earth-earth_root", volcano, 2, 18);
	map->DrawMaterial("Earth-earth_spongy", volcano, 2, 18);
	map->DrawMaterial("Earth-earth", volcano, 4, 16);
	map->DrawMaterial("Ashes", volcano, 3, 8);
	map->DrawMaterial("Tunnel", volcano, 7, 16);
	map->DrawMaterial("Tunnel", volcano, 8, 10);
	map->DrawMaterial("Rock", volcano, 3, 6);
	map->DrawMaterial("Rock-rock_smooth", volcano, 3, 6);
	map->DrawMaterial("Ore", volcano, 5, 12);
	map->DrawMaterial("Firestone", volcano, 5, 12);
	map->DrawMaterial("Coal", volcano, 5, 12);
	map->DrawMaterial(["Water", "Water", "DuroLava"][difficulty - 1], volcano, 6, 2);
	map->DrawMaterial(["Water", "DuroLava", "DuroLava"][difficulty - 1], volcano, 6, 2);
	
	// Draw ashes borders around the lava parts.
	var lava = this->Duplicate("DuroLava");
	var lava_borders = {Algo = MAPALGO_Border, Op = lava, Wdt = -1, Top = 0};
	map->Draw("Ashes", lava_borders);
	
	// Ashes are scattered around the surface.
	var surface = {Algo = MAPALGO_Border, Top = 1, Op = volcano};
	map->DrawMaterial("Ashes", surface, 4, 80);
	var surface = {Algo = MAPALGO_Border, Top = 3, Op = volcano};
	map->DrawMaterial("Ashes", surface, 4, 40);
	
	FixLiquidBorders("Earth", "Ashes");
	return;
}

// Draws the main chasm.
public func DrawChasm(proplist map, proplist volcano, proplist chasm)
{
	var wdt = map.Wdt;
	var hgt = map.Hgt;
	
	// Add part of the crater with to the chasm.
	var sky = map->Duplicate("*");
	sky = {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Layer, Layer = sky}};
	var crater = {Algo = MAPALGO_And, Op = [sky, {Algo = MAPALGO_Rect, X = wdt / 2 - 3 * wdt / 40, Y = 43 * hgt / 150, Wdt = 6 * wdt / 40, Hgt = 20}]};
	var full_chasm = {Algo = MAPALGO_Or, Op = [chasm, crater]};
	
	// First draw the chasm with tunnel and then make sure the side chasms have a flat surface.
	map->Draw("Tunnel", full_chasm);
	for (var i = 1; i <= 4; i++)
	{
		var exit = chasm_exits[i];
		var flat = {Algo = MAPALGO_Rect, X = exit[0] - 5, Y = exit[1] - 3, Wdt = 10, Hgt = 5};
		full_chasm = {Algo = MAPALGO_And, Op = [full_chasm, {Algo = MAPALGO_Not, Op = flat}]};
	}
	// Then fill the chasm with durolava.
	map->Draw("DuroLava", full_chasm);
	
	// The border of the chasm is out of rock, granite and ashes.
	var chasm_crust = {Algo = MAPALGO_Border, Left = -5, Right = -5, Op = full_chasm};
	chasm_crust = {Algo = MAPALGO_And, Op = [chasm_crust, volcano]};
	map->Draw("Everrock", chasm_crust);
	chasm_crust = {Algo = MAPALGO_Border, Left = -1, Right = -1, Op = full_chasm};
	map->Draw("Rock", chasm_crust);
	map->DrawMaterial("Rock-rock_smooth", chasm_crust, 3, 30);
	map->DrawMaterial("Granite", chasm_crust, 3, 30);
	
	// Draw the gold area in the volcano's core.
	var core = {Algo = MAPALGO_Ellipse, X = wdt / 2, Y = hgt, Wdt = wdt / 10, Hgt = hgt / 7};
	core = {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 6, Iterations = 2, Seed = Random(65536), Op = core};
	var lines = {Algo = MAPALGO_Lines, X = 4, Y = 0, Distance = 8};
	lines = {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 12, Iterations = 4, Seed = Random(65536), Op = lines}; 
	core = {Algo = MAPALGO_And, Op = [core, lines]};
	map->Draw("Gold", core);
	return;
}
