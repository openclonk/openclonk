/**
	Rapid Refining
	Use the oil from an underground well to power your settlement.

	@author Maikel
*/

#include Library_Map


// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Map settings.
	var overground_wdt = 40;
	var overground_hgt = 20;
	
	// Retrieve the settings according to the MapSize setting.
	var map_size;
	if (SCENPAR_MapSize == 1)
		map_size = [250, 150]; 
	if (SCENPAR_MapSize == 2)
		map_size = [275, 150];
	if (SCENPAR_MapSize == 3)
		map_size = [300, 150];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	// The overground area is in the top left corner, the underground are thereby the remainder.
	var underground = {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = overground_wdt, Hgt = overground_hgt}};
	Draw("Earth", underground);
	
	// Draw materials in the underground area and then overlay the rest of the map.
	DrawMaterial("Earth-earth_root", underground, 2, 12);
	DrawMaterial("Earth-earth_spongy", underground, 2, 12);
	DrawMaterial("Granite", underground, 3, 6);
	DrawMaterial("Tunnel", underground, 5, 8);
	DrawMaterial("Rock-rock", underground, 3, 4);
	DrawMaterial("Rock", underground, 3, 4);
	DrawMaterial("Ore", underground, 6, 6);
   	DrawMaterial("Firestone", underground, 5, 4);
   	DrawMaterial("Coal", underground, 6, 4);
   	
   	// Draw some underground water lakes.
	var underground_water = {Algo = MAPALGO_And, Op = [underground, {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt / 2, Wdt = map.Wdt - 20, Hgt = map.Hgt / 2}]};
	DrawMaterial("Water", underground_water, [4, 10], 6);

	// The entrance border is out of granite.
	var entrance_border = {Algo = MAPALGO_Border, Left = 3, Op = underground};
	entrance_border = {Algo = MAPALGO_Or, Op = [entrance_border, {Algo = MAPALGO_Turbulence, Iterations = 2, Amplitude = [6, 8], Scale = [6, 8], Seed = Random(65536), Op = entrance_border}]};
	DrawRock(entrance_border);
	
	// The entrance floor is out of earth and brick.
	var entrance_floor = {Algo = MAPALGO_Border, Top = 3, Op = underground};
	Draw("Brick", entrance_floor);
	var entrance_floor_earth = {Algo = MAPALGO_Border, Top = 2, Op = underground};
	var entrance_floor_earth = {Algo = MAPALGO_And, Op = [entrance_floor_earth, {Algo = MAPALGO_Rect, X = 4, Y = 0, Wdt = 17, Hgt = map.Hgt}]};
	Draw("Earth", entrance_floor_earth);
	
	// There are borders on the full underground area of the map.
	var wdt = 3;
	var underground_border = {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = wdt, Y = wdt, Wdt = map.Wdt - 2 * wdt, Hgt = map.Hgt - 2 * wdt}};
	underground_border = {Algo = MAPALGO_And, Op = [underground_border, underground]};
	underground_border = {Algo = MAPALGO_Or, Op = [underground_border, {Algo = MAPALGO_Turbulence, Iterations = 6, Amplitude = 12, Scale = 16, Seed = Random(65536), Op = underground_border}]};
	underground_border = {Algo = MAPALGO_And, Op = [underground_border, underground, {Algo = MAPALGO_Not, Op = entrance_floor}]};
	DrawRock(underground_border);
		
	// Draw a tunnel to the oil well.
	DrawMainTunnel(map, overground_wdt, overground_hgt);
    
	// Some slabs of granite/rock which block the path to the oil.
	DrawRockSlabs(map);
	
	// There is a smaller oil field below the entrance.
	DrawOilLakes(map);

	// The main oil well is in the bottom right of the map.
	DrawOilWell(map, underground_border);
    
    // A large water lake at the bottom of the map.
    DrawWaterLake(map, underground_border);
    
    // Some liquid veins above the oil well.
    DrawLiquidVeins(map, underground_border);
    
    // Fix liquid borders.
	FixLiquidBorders();
    
	// Return true to tell the engine a map has been successfully created.
	return true;
}

public func DrawMainTunnel(proplist map, int overground_wdt, int overground_hgt)
{
	var sx = overground_wdt;
	var sy = overground_hgt - 3;
	var ex = map.Wdt - 20;
	var ey = map.Hgt - 30;
	var nr_steps = 10;
	var tunnel_x = [], tunnel_y = [];
	for (var index = 0; index <= nr_steps; index++)
	{
		var dev = 4 * Min(index, nr_steps - index); 
		tunnel_x[index] = sx + (ex - sx) * index / nr_steps + RandomX(-dev, dev);
		tunnel_y[index] = sy + (ey - sy) * index / nr_steps + RandomX(-dev, dev);
	}
	
	// Draw main tunnel.
	var tunnel = {Algo = MAPALGO_Polygon, X = tunnel_x, Y = tunnel_y, Wdt = 3, Empty = true, Open = true};
	tunnel = {Algo = MAPALGO_Or, Op = [tunnel, {Algo = MAPALGO_Turbulence, Iterations = 2, Amplitude = [6, 8], Scale = [6, 8], Seed = Random(65536), Op = tunnel}]};
	Draw("Tunnel", tunnel);

    // Draw branches.
	for (var index = 2; index <= nr_steps - 2; index++)
	{
		var x = tunnel_x[index];
		var y = tunnel_y[index];
		var to_x = x + RandomX(-5, 5);
		var to_y = y + (2 * Random(2) - 1) * RandomX(16, 22);
		var tunnel_branch = {Algo = MAPALGO_Polygon, X = [x, to_x], Y = [y, to_y], Wdt = 2, Empty = true, Open = true};
		tunnel_branch = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = 12, Scale = 8, Seed = Random(65536), Op = tunnel_branch};
   		Draw("Tunnel", tunnel_branch);
	}    
	return;
}

public func DrawRockSlabs(proplist map)
{
	var slabs = {Algo = MAPALGO_Lines, X = 6, Y = 1, Distance = 32};
	slabs = {Algo = MAPALGO_And, Op = [slabs, {Algo = MAPALGO_RndChecker, Ratio = 75, Seed = Random(65536)}]};
	slabs = {Algo = MAPALGO_Turbulence, Iterations = 2, Amplitude = [6, 8], Scale = [6, 8], Seed = Random(65536), Op = slabs};
	slabs = {Algo = MAPALGO_And, Op = [slabs, {Algo = MAPALGO_Rect, X = map.Wdt / 3, Y = 0, Wdt = 2 * map.Wdt / 3, Hgt = map.Hgt}]};
	DrawRock(slabs);
	return;
}

public func DrawWaterLake(proplist map, proplist underground_border)
{
	var lake_height = 20;
	var lake_width = 80;
	var waterfall_height = 62;
	var tunnel_height = 6;
	
	// Draw a large lake with tunnel above.
	underground_border = {Algo = MAPALGO_And, Op = [underground_border, {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - lake_height - 2, Wdt = lake_width, Hgt = 6}}]};
	var lake_floor_rock = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Lines, X = 3, Y = 0, Distance = 10}, {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - 7, Wdt = lake_width, Hgt = 7}]};
	var lake_floor_rock = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = 16, Scale = 8, Seed = Random(65536), Op = lake_floor_rock};
	underground_border = {Algo = MAPALGO_Or, Op = [underground_border, lake_floor_rock]};
	DrawRock(lake_floor_rock);
	
	
	var tunnel = {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - lake_height - tunnel_height, Wdt = lake_width, Hgt = tunnel_height};
	tunnel = {Algo = MAPALGO_Or, Op = [tunnel, {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = 16, Scale = 8, Seed = Random(65536), Op = tunnel}]};
	tunnel = {Algo = MAPALGO_And, Op = [tunnel, {Algo = MAPALGO_Not, Op = underground_border}]};
	Draw("Tunnel", tunnel);
	
	var lake = {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - lake_height, Wdt = lake_width, Hgt = lake_height};
	lake = {Algo = MAPALGO_And, Op = [lake, {Algo = MAPALGO_Not, Op = underground_border}]};
	Draw("Water:Water", lake);
	
	var lake_floor = {Algo = MAPALGO_Border, Bottom = 2, Op = lake};
	Draw("Sand", lake_floor);
	
	var lake_boundary = {Algo = MAPALGO_Rect, X = lake_width - 1, Y = map.Hgt - lake_height, Wdt = 2, Hgt = lake_height};
	lake_boundary = {Algo = MAPALGO_Or, Op = [lake_boundary, {Algo = MAPALGO_Turbulence, Iterations = 3, Amplitude = 12, Scale = 12, Seed = Random(65536), Op = lake_boundary}]};
	Draw("Everrock", lake_boundary);
	var lake_boundary_rock = {Algo = MAPALGO_Border, Wdt = -2, Op = lake_boundary};
	DrawRock(lake_boundary_rock);
	
	// Draw a waterfall pooring into the lake.
	var waterfall = {Algo = MAPALGO_Rect, X = 3, Y = map.Hgt - lake_height - waterfall_height, Wdt = 9, Hgt = waterfall_height};
	waterfall = {Algo = MAPALGO_Or, Op = [waterfall, {Algo = MAPALGO_Turbulence, Iterations = 3, Amplitude = 12, Scale = 12, Seed = Random(65536), Op = waterfall}]};
	waterfall = {Algo = MAPALGO_And, Op = [waterfall, {Algo = MAPALGO_Not, Op = underground_border}, {Algo = MAPALGO_Not, Op = lake}]};
	Draw("Tunnel", waterfall);
	return;
}

public func DrawOilLakes(proplist map)
{
	for (var cnt = 0; cnt < 3; cnt++)
	{
		var oil_lake_x = RandomX(map.Wdt / 10, 3 * map.Wdt / 10);
		var oil_lake_y = RandomX(2 * map.Hgt / 7, 5 * map.Hgt / 7);
		var oil_lake = {Algo = MAPALGO_Rect, X = oil_lake_x, Y = oil_lake_y, Wdt = 13, Hgt = 15};
		oil_lake = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = 10, Scale = 10, Seed = Random(65536), Op = oil_lake};
		var oil_lake_filled = {Algo = MAPALGO_And, Op = [oil_lake, {Algo = MAPALGO_Rect, X = 0, Y = oil_lake_y - 3, Wdt = map.Wdt, Hgt = map.Hgt}]};
		Draw("Tunnel", oil_lake);
		Draw("Oil", oil_lake_filled);
		var oil_lake_border = {Algo = MAPALGO_Border, Left = 2, Right = 2, Bottom = 3, Top = 1, Op = oil_lake};
		DrawRock(oil_lake_border);
	}
	return;
}

public func DrawOilWell(proplist map, proplist underground_border)
{
	var oil_field = {Algo = MAPALGO_Polygon, 
		X = [map.Wdt, map.Wdt - 16, map.Wdt - 14, map.Wdt - 10, map.Wdt - 17, map.Wdt - 22, map.Wdt - 22, map.Wdt - 27, map.Wdt - 29], 
		Y = [map.Hgt - 5, map.Hgt - 5, map.Hgt - 12, map.Hgt - 18, map.Hgt - 23, map.Hgt - 20, map.Hgt - 12, map.Hgt - 12, map.Hgt - 16], 
		Wdt = 3, Empty = true, Open = true};
	var oil_field_oil = {Algo = MAPALGO_And, Op = [oil_field, {Algo = MAPALGO_Rect, X = 0, Y = map.Hgt - 8, Wdt = map.Wdt, Hgt = 8}]};
	var oil_field_water = {Algo = MAPALGO_And, Op = [oil_field, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = map.Wdt, Hgt = map.Hgt - 8}]};
	Draw("Oil:Oil", oil_field_oil);
	Draw("Water", oil_field_water);
	var oil_field_border = {Algo = MAPALGO_Border, Left = -3, Right = -3, Top = -3, Bottom = -5, Op = oil_field};
	Draw("Everrock", oil_field_border);
	var oil_field_entrance = {Algo = MAPALGO_Rect, X = map.Wdt - 31, Y = map.Hgt - 22, Wdt = 5, Hgt = 5};
	Draw("Tunnel", oil_field_entrance);
	return;
}

public func DrawLiquidVeins(proplist map, proplist underground_border)
{
	// Draw some gold as a source of wealth.
	var gold = {Algo = MAPALGO_Rect, X = map.Wdt - 60, Y = 10, Wdt = 50, Hgt = 36};
	gold = {Algo = MAPALGO_And, Op = [gold, {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = 12, Scale = 12, Seed = Random(65536), Op = gold}]};
	DrawMaterial("Gold", gold, 4, 30);
		
	// Draw the veins.
	var vein_material = "Water";
	if (SCENPAR_Difficulty == 2)
		vein_material = "Acid";
	if (SCENPAR_Difficulty == 3)
		vein_material = "DuroLava";
	var vein_area = {Algo = MAPALGO_Polygon, X = [map.Wdt - 96, map.Wdt, map.Wdt, map.Wdt - 44], Y = [0, 0, 108, 82]};
	vein_area = {Algo = MAPALGO_And, Op = [vein_area, {Algo = MAPALGO_Not, Op = underground_border}]}; 
	
	var veins = {Algo = MAPALGO_Or, Op = [
		{Algo = MAPALGO_Lines, X = 2, Y = -1, Distance = 12},
		{Algo = MAPALGO_Lines, X = 2, Y = 1, Distance = 12},
		{Algo = MAPALGO_Lines, X = 0, Y = 1, Distance = 16}
	]};
	veins = {Algo = MAPALGO_And, Op = [veins, vein_area]}; 
	veins = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = 12, Scale = 12, Seed = Random(65536), Op = veins};
	
	Draw(vein_material, veins);
	return;
}


/*-- Helper Functions --*/

public func DrawRock(proplist layer)
{
	Draw("Granite", layer);
	DrawMaterial("Rock-rock", layer, 4, 18);
	DrawMaterial("Rock", layer, 4, 18);
	return;
}