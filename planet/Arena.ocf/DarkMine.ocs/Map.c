/**
	Dark Mine
	A single large cave surrounded by lots of small caves, connected through
	narrow mine shafts.
	
	@authors Maikel
*/

#include Library_Map


// List for storing the different large caves.
static cave_list;

// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Map size: all other settings depend on this value.
	// The map size depends on the number of players.
	var plr_cnt = GetStartupPlayerCount() - 2;
	var map_size = BoundBy(140 + plr_cnt * 6, 140, 240);
	
	// Set the map size, which is always square.
	map->Resize(map_size, map_size);
	
	// The map consists of one large cave, multiple small caves and connections.
	var large_cave = FindLargeCave(map, BoundBy(map_size / 8, 18, 24));
	var small_caves = FindSmallCaves(map, large_cave, map_size / 4);
	var connections = FindCaveConnections(small_caves, BoundBy(map_size / 5, 24, 36));
	
	// Store small cave positions in a static variable for the scenario script.
	cave_list = [];
	for (var cave in small_caves)
		PushBack(cave_list, [cave.X, cave.Y]);
	
	// Draw the background materials for the whole map.
	DrawBackground();
	// Draw the smaller caves.
	DrawSmallCaves(small_caves);
	// Draw the connections
	DrawConnections(connections);
	// Draw the large caves, last because they should be drawn over the 
	// small caves functioning as entrances.
	DrawLargeCave(large_cave);

	// Return true to tell the engine a map has been successfully created.
	return true;
}


/*-- Cave Creation --*/

// Returns the large cave in the center of the map.
public func  FindLargeCave(proplist map, int size)
{
	// Get the map coordinates.
	var x = map.Wdt / 2;
	var y = map.Hgt / 2;
	var cave = {Algo = MAPALGO_Ellipse, X = x, Y = y, Wdt = size, Hgt = 2 * size / 3};
	return cave;
}

public func FindSmallCaves(proplist map, proplist large_cave, int nr_caves)
{
	// Prepare a mask out of the map and the large cave in which to search for smaller ones.
	var mask = map->CreateLayer();
	mask->Draw("Rock");
	// Contruct the doughnut like shape in which the small caves may be found.
	var x = map.Wdt / 2;
	var y = map.Hgt / 2;
	mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = x, Y = y, Wdt = 24, Hgt = 24});
	mask->Draw("Tunnel", {Algo = MAPALGO_Not, Op = {Algo = MAPALGO_Ellipse, X = x, Y = y, Wdt = x - 2, Hgt = y - 2}});
	mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = large_cave.X, Y = large_cave.Y, Wdt = 3 * large_cave.Wdt / 2, Hgt = 14 * large_cave.Hgt / 10});
	// Array for the small caves.	
	var caves = [];	
	// Add caves for the exit points of the large caves.
	PushBack(caves, {X = large_cave.X - large_cave.Wdt - 1, Y = large_cave.Y, block_dir = COMD_Right});
	PushBack(caves, {X = large_cave.X + large_cave.Wdt + 1, Y = large_cave.Y, block_dir = COMD_Left});
	PushBack(caves, {X = large_cave.X, Y = large_cave.Y - large_cave.Hgt - 1, block_dir = COMD_Down});
	PushBack(caves, {X = large_cave.X, Y = large_cave.Y + large_cave.Hgt + 1, block_dir = COMD_Up});
	// Add caves at random locations around the map.
	var border = 6;
	var cave_dist = 20;
	for (var i = 0; i < nr_caves; i++)
	{
		var cave = {};
		if (!mask->FindPosition(cave, "Rock", [border, border, map.Wdt - border * 2, map.Hgt - border * 2]))
			continue;
		mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = cave.X, Y = cave.Y, Wdt = cave_dist, Hgt = cave_dist});	
		PushBack(caves, cave);
	}
	return caves;
}

public func FindCaveConnections(array small_caves, int max_length)
{
	var connections = [];
	for (var cave in small_caves)
		cave.conn_count = 0;	
	for (var i = 0; i < GetLength(small_caves) - 1; i++)
	{
		var from_cave = small_caves[i];
		for (var j = i + 1; j < GetLength(small_caves); j++)
		{
			var to_cave = small_caves[j];
			// Check for the maximum connections per cave.
			if (from_cave.conn_count >= 4 || to_cave.conn_count >= 4)
				continue;
			// Cave line parameters.
			var fx = from_cave.X;
			var fy = from_cave.Y;
			var tx = to_cave.X;
			var ty = to_cave.Y;
			// Check for the maximum line distance.
			if (Distance(fx, fy, tx, ty) > max_length)
				continue;
			// Check for block line directions.
			if (from_cave.block_dir != nil && IsBlockedDirection(from_cave.block_dir, fx, fy, tx, ty))
				continue;
			if (to_cave.block_dir != nil && IsBlockedDirection(to_cave.block_dir, tx, ty, fx, fy))
				continue;
			// Check for overlap in existing connections.
			var has_overlap = false;
			for (var line in connections)
				if (IsLineOverlap(fx, fy, tx, ty, line.X[0], line.Y[0], line.X[1], line.Y[1]))	
				{
					has_overlap = true;
					break;
				}
			if (has_overlap)
				continue;
			// Determine tunnel width and make the tunnel.
			var tunnel_wdt = 3;
			if (!Random(10))
				tunnel_wdt = 2;
			var tunnel = {Algo = MAPALGO_Polygon, X = [fx, tx], Y = [fy, ty], Wdt = tunnel_wdt, Open = 1, Empty = 1};
			PushBack(connections, tunnel);
			small_caves[i].conn_count++;
			small_caves[j].conn_count++;
		}
	}
	return connections;
}

public func IsBlockedDirection(int dir, int x1, int y1, int x2, int y2)
{
	// Use COMD dir constants.
	if (dir == COMD_Right && x2 > x1)
		return true;
	if (dir == COMD_Left && x2 < x1)
		return true;
	if (dir == COMD_Down && y2 > y1)
		return true;
	if (dir == COMD_Up && y2 < y1)
		return true;
	return false;
}


/*-- Map Drawing --*/

public func DrawBackground()
{
	Draw("Rock");
	DrawVariations("Rock", 50, 5, 15);
	DrawVariations("Ore", 10, 8, 8);
	DrawVariations("Firestone", 8, 12, 3);
	DrawVariations("Earth", 3, 8, 3);
	DrawVariations("Earth-earth", 3, 8, 3);
	DrawVariations("Earth-earth", 3, 8, 3);
	DrawVariations("Earth-earth_root", 3, 8, 3);
	DrawVariations("Earth-earth_spongy", 3, 8, 3);
	DrawVariations("Firestone", 6, 12, 3);
	DrawVariations("Coal", 8, 8, 3);
	DrawVariations("Gold", 5, 4, 4);
	DrawVariations("Granite", 14, 15, 5);
	DrawVariations("Granite", 14, 5, 15);
	return true;
}

public func DrawLargeCave(proplist large_cave)
{
	// Draw the first large cave and then duplicate the others.
	var cave_border = {Algo = MAPALGO_Border, Left = 5, Right = 5, Bottom = 3, Top = 5, Op = large_cave};
	var border_holes = {Algo = MAPALGO_Rect, X = large_cave.X - large_cave.Wdt, Y = large_cave.Y - 1, Wdt = 2 * large_cave.Wdt, Hgt = 5};
	border_holes = {Algo = MAPALGO_Or, Op = [border_holes, {Algo = MAPALGO_Rect, X = large_cave.X - 2, Y = large_cave.Y - large_cave.Hgt, Wdt = 5, Hgt = 2 * large_cave.Hgt}]};
	cave_border = {Algo = MAPALGO_And, Op = [cave_border, {Algo = MAPALGO_Not, Op = border_holes}]};
	var lower_half = {Algo = MAPALGO_Rect, X = large_cave.X - large_cave.Wdt, Y = large_cave.Y + 6, Wdt = 2 * large_cave.Wdt, Hgt = large_cave.Hgt};
	lower_half = {Algo = MAPALGO_And, Op = [large_cave, lower_half]};
	
	// Fill all the algorithms.
	Draw("Tunnel", large_cave);
	DrawMaterial("Tunnel-brickback", large_cave, 3, 30);
	Draw("Earth", lower_half);
	DrawMaterial("Earth-earth", lower_half, 3, 20);
	DrawMaterial("Earth-earth", lower_half, 3, 20);
	DrawMaterial("Earth-earth_root", lower_half, 3, 20);
	DrawMaterial("Ore", lower_half, 3, 20);
	DrawMaterial("Firestone", lower_half, 3, 20);
	DrawMaterial("Coal", lower_half, 3, 20);
	Draw("Granite", cave_border);
	return;
}

public func DrawSmallCaves(array small_caves)
{
	for (var cave in small_caves)
	{
		var cave_algo = {Algo = MAPALGO_Ellipse, X = cave.X, Y = cave.Y, Wdt = RandomX(4, 6), Hgt = RandomX(4, 6)};
		var turb_cave = {Algo = MAPALGO_Turbulence, Amplitude = 12, Scale = 8, Op = cave_algo};
		Draw("Tunnel", turb_cave);
		DrawMaterial("Tunnel-brickback", turb_cave, 2, 15);
	}
	return;
}

public func DrawConnections(connections)
{
	for (var con in connections)
	{
		con = {Algo = MAPALGO_Turbulence, Amplitude = 5, Scale = 3, Op = con};
		Draw("Tunnel", con);
		DrawMaterial("Tunnel-brickback", con, 2, 15);
	}
	return;
}


/*-- Helper Functions --*/

public func DrawVariations(string mat, int ratio, int sx, int sy)
{
	var rand_algo = {Algo=MAPALGO_RndChecker, Ratio = ratio, Wdt = sx, Hgt = sy};
	var turb_algo = {Algo=MAPALGO_Turbulence, Amplitude = 12, Scale = 8, Op = rand_algo};
	return Draw(mat, turb_algo);
}
