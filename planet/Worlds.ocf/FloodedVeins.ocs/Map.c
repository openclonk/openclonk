/**
	Flooded Veins
	Dynamic map with a cave leading to gems filled with water deep down.
	
	@author Sven2, Maikel
*/

#include Library_Map


// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Retrieve the settings according to the MapSize setting.
	var map_size;
	if (SCENPAR_MapSize == 1)
		map_size = [100, 150]; 
	if (SCENPAR_MapSize == 2)
		map_size = [100, 175];
	if (SCENPAR_MapSize == 3)
		map_size = [100, 200];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	// The map is completely covered in earth.
	map->Draw("Earth");
	
	// Draw a cavern somewhere at the top where the settlement is located.
	// This takes up the top 25 landscape pixels.
	DrawCavern(map);
	
	// Draw the middle section of the map with useful resources.
	// This takes 45, 50 or 55 landscape pixels.
	DrawMiddle(map, 40 + 5 * SCENPAR_MapSize);
	
	// Draw the gem veins including it being flooded at the bottom of the map.
	// This takes up the remaining landscape pixels, which depends on the map size.
    DrawGemVeins(map, 60 + 20 * SCENPAR_MapSize, SCENPAR_Difficulty);
    
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Draws the top cavern where the player starts it settlement.
public func DrawCavern(proplist map)
{
    var wdt = map.Wdt;
    var cavern_hgt = 20;
    
    // Create a mask for the cavern part.
    var map_top = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = wdt, Hgt = cavern_hgt};
    
    // The top part is mainly granite and rock.
    map->DrawMaterial("Earth-earth_root", map_top, 2, 16);
	map->DrawMaterial("Earth-earth_spongy", map_top, 2, 16);
	map->DrawMaterial("Earth-earth", map_top, 4, 12);
	map->DrawMaterial("Granite", map_top, 3, 60);
	map->DrawMaterial("Tunnel", map_top, 2, 16);
	map->DrawMaterial("Rock-rock", map_top, 3, 20);
	map->DrawMaterial("Rock", map_top, 3, 20);
   
    // The cavern is located a bit to the left and can be entered from the left side of the map.
    var cavern = {Algo = MAPALGO_Ellipse, X = wdt / 2 - 10, Y = cavern_hgt - 7, Wdt = 20, Hgt = 7};
    cavern = {Algo = MAPALGO_Or, Op = [cavern, {Algo = MAPALGO_Turbulence, Iterations = 4, Seed = Random(65536), Op = cavern}]};
    cavern = {Algo = MAPALGO_Or, Op = [cavern, {Algo = MAPALGO_Rect, X = wdt / 2 - 30, Y = cavern_hgt - 7, Wdt = 40, Hgt = 7}]};
    cavern = {Algo = MAPALGO_And, Op = [cavern, map_top]};
    map->Draw("Tunnel", cavern);
    map->DrawMaterial("Tunnel-brickback", cavern, 5, 32);
    map->DrawMaterial("Sky", cavern, 5, 20);
    map->DrawMaterial("Sky", cavern, 4, 20);
    map->DrawMaterial("Sky", cavern, 3, 20);
    var cavern_ground = {Algo = MAPALGO_Border, Bottom = -2, Op = cavern};
    map->DrawMaterial("Brick", cavern_ground, 3, 80);

    // Draw the left tunnel.
    var tunnel_left = {Algo = MAPALGO_Polygon, X = [0, wdt / 2 - 30], Y = [cavern_hgt, cavern_hgt], Wdt = -6, Open = 1, Empty = 1};
    tunnel_left = {Algo = MAPALGO_Or, Op = [tunnel_left, {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 8, Iterations = 4, Seed = Random(65536), Op = tunnel_left}]};
    tunnel_left = {Algo = MAPALGO_And, Op = [tunnel_left, map_top]};
    map->Draw("Tunnel", tunnel_left);
    map->DrawMaterial("Tunnel-brickback", tunnel_left, 3, 32);
    map->DrawMaterial("Sky", tunnel_left, 5, 16);
    map->DrawMaterial("Sky", tunnel_left, 3, 24);
    var tunnel_entrance = {Algo = MAPALGO_And, Op = [tunnel_left, {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = 2, Hgt = cavern_hgt}]};
    map->Draw("Sky", tunnel_entrance);
    var tunnel_left_ground = {Algo = MAPALGO_Border, Bottom = -2, Op = tunnel_left};
    tunnel_left_ground = {Algo = MAPALGO_Or, Op = [tunnel_left, {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 6, Iterations = 2, Seed = Random(65536), Op = tunnel_left_ground}]};
    tunnel_left_ground = {Algo = MAPALGO_And, Op = [tunnel_left_ground, {Algo = MAPALGO_Not, Op = map_top}]};
    map->DrawMaterial("Brick", tunnel_left_ground, 3, 24);

    // Draw the right tunnel.
    var tunnel_right = {Algo = MAPALGO_Polygon, X = [wdt / 2 + 10, wdt / 2 + 30, wdt / 2 + 36], Y = [cavern_hgt - 4, cavern_hgt - 8, cavern_hgt + 6], Wdt = -4, Open = 1, Empty = 1};
    tunnel_right = {Algo = MAPALGO_Or, Op = [tunnel_right, {Algo = MAPALGO_Turbulence, Amplitude = 8, Scale = 6, Iterations = 4, Seed = Random(65536), Op = tunnel_right}]};
    //tunnel_right = {Algo = MAPALGO_And, Op = [tunnel_right, map_top]};
    map->Draw("Tunnel", tunnel_right);
    map->DrawMaterial("Tunnel-brickback", tunnel_right, 3, 24);
    return;
}

// Draws the middle section of the map with useful resources.
public func DrawMiddle(proplist map, int size)
{
    var wdt = map.Wdt;

    // Create a mask for the middle part.
    var map_middle = {Algo = MAPALGO_Rect, X = 0, Y = 25, Wdt = wdt, Hgt = size};
    
    // Fill the middle part with resources.
    map->DrawMaterial("Earth-earth_root", map_middle, 2, 16);
	map->DrawMaterial("Earth-earth_spongy", map_middle, 2, 16);
	map->DrawMaterial("Earth-earth", map_middle, 4, 12);
	map->DrawMaterial("Granite", map_middle, 3, 10);
	map->DrawMaterial("Tunnel", map_middle, 2, 8);
	map->DrawMaterial("Rock-rock", map_middle, 3, 8);
	map->DrawMaterial("Rock", map_middle, 3, 8);
   	map->DrawMaterial("Ore", map_middle, 6, 16);
   	map->DrawMaterial("Firestone", map_middle, 5, 12);
   	map->DrawMaterial("Coal", map_middle, 6, 16);
    
	// Create a tunnel system which covers the middle part.
	var tunnel = {Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = 0, Hgt = 0};
	for (var i = 0; i < 5; i++)
	{
		var x1 = RandomX(8, wdt - 8);
		var y1 = RandomX(30, 10 + size);
		var dir = RandomX(-15, 15) + Random(2) * 180;
		var dist = RandomX(20, 28);
		var x2 = BoundBy(x1 + Cos(dir, dist), 4, wdt - 4);
		var y2 = BoundBy(y1 + Sin(dir, dist), 30, 15 + size);
		var add_tunnel = {Algo = MAPALGO_Polygon, X = [x1, x2], Y = [y1, y2], Wdt = -RandomX(3, 4), Open = 1, Empty = 1};
		var tunnel = {Algo = MAPALGO_Or, Op = [tunnel, add_tunnel]};
	}
	tunnel = {Algo = MAPALGO_Turbulence, Amplitude = 6, Scale = 8, Iterations = 4, Seed = Random(65536), Op = tunnel};
	map->Draw("Tunnel", tunnel);
	var tunnel_ground = {Algo = MAPALGO_Border, Bottom = -2, Op = tunnel};
	map->Draw("Earth", tunnel_ground);
	map->DrawMaterial("Earth-earth_root", tunnel_ground, 2, 16);
	map->DrawMaterial("Earth-earth_spongy", tunnel_ground, 2, 16);
	map->DrawMaterial("Earth-earth", tunnel_ground, 4, 12);
	return;
}

// Draws the gem veins including it being flooded at the bottom of the map.
public func DrawGemVeins(proplist map, int size, int difficulty)
{
    var wdt = map.Wdt;
    var hgt = map.Hgt;
    
    // Create a mask for the bottom part.
    var map_bottom = {Algo = MAPALGO_Rect, X = 0, Y = hgt - size, Wdt = wdt, Hgt = size};
    
    // Fill the bottom with mostly granite materials.
    map->Draw("Granite", map_bottom);
    map->DrawMaterial("Firestone", map_bottom, 2, 6);
	map->DrawMaterial("Rock-rock", map_bottom, 3, 12);
	map->DrawMaterial("Rock", map_bottom, 3, 12);
	map->DrawMaterial("Earth-earth_root", map_bottom, 2, 8);
	map->DrawMaterial("Earth-earth_spongy", map_bottom, 2, 8);
	map->DrawMaterial("Earth-earth", map_bottom, 4, 6);
	
	// Draw a labyrinth using nodes and connections and make it out of tunnel.
	var nodes = FindVeinNodes(map, size + 12, size / 2);
	var connections = FindNodeConnections(nodes, 24);
	for (var con in connections)
		map->Draw("Tunnel", con);
	
	// Make three out of the five gems nodes contain gems and protect with granite.
	var cnt = 0, i = 0;
	while (cnt < 3 && i < 5)
	{
		var node = nodes[i];
		i++;
		if (node.conn_count == 0)
			continue;
		var tunnel = node.tunnels[0];
		var gem_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Left = 2, Right = 2, Op = tunnel}, {Algo = MAPALGO_Rect, X = 0, Y = node.Y - 3, Wdt = wdt, Hgt = 9}]};
		var granite_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Left = 2, Right = 2, Op = tunnel}, {Algo = MAPALGO_Rect, X = 0, Y = node.Y - 5, Wdt = wdt, Hgt = 2}]};
		map->Draw(["Ruby", "Amethyst"][Random(2)], gem_border);
		map->Draw("Granite", granite_border);
		cnt++;
	}
	
	// Replace the tunnels with water up to a certain level, depends on difficulty.
	var water_level = size - 8 * (3 - difficulty);
	var tunnels = Duplicate("Tunnel");
	var tunnels_algo = {Algo = MAPALGO_Layer, Layer = tunnels};
	tunnels_algo = {Algo = MAPALGO_And, Op = [tunnels_algo, {Algo = MAPALGO_Rect, X = 0, Y = hgt - water_level, Wdt = wdt, Hgt = water_level}]}; 
	map->Draw("Water", tunnels_algo);
	
	// On insane the background material of the bottom part is water.    
    if (difficulty == 3)
    {
    	for (var x = 0; x < wdt; x++)
    	{
    		for (var y = water_level; y < hgt; y++)
    		{
    			// TODO: wait for background pixel setting.
    		}   
    	}
    }
    return;
}


/*-- Nodes and Connections --*/

public func FindVeinNodes(proplist map, int size, nr_nodes)
{
	var wdt = map.Wdt;
    var hgt = map.Hgt;
	
	// Prepare a mask out of the map to search for nodes.
	var mask = map->CreateLayer();
	mask->Draw("Rock");

	// Array for the nodes and definition for a minimum distance.	
	var nodes = [];
	var node_dist = 8;

	// The first five nodes are at the bottom of the map and are not to be connected.
	for (var i = 0; i < 5; i++)
	{
		var node = {};
		if (!mask->FindPosition(node, "Rock", [4, hgt - 10, wdt - 8, 6]))
			continue;
		mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = node.X, Y = node.Y, Wdt = node_dist, Hgt = node_dist});
		node.is_gem = true;
		node.tunnels = [];
		PushBack(nodes, node);	
	}
	
	// The other nodes are scattered randomly in the lower areas
	for (var i = 5; i < nr_nodes; i++)
	{
		var node = {};
		if (!mask->FindPosition(node, "Rock", [4, hgt - size + 4, wdt - 8, size - 18]))
			continue;
		mask->Draw("Tunnel", {Algo = MAPALGO_Ellipse, X = node.X, Y = node.Y, Wdt = node_dist, Hgt = node_dist});
		node.tunnels = [];
		PushBack(nodes, node);
	}
	return nodes;
}

public func FindNodeConnections(array nodes, int max_length)
{
	var connections = [];
	for (var node in nodes)
		node.conn_count = 0;	
	for (var i = 0; i < GetLength(nodes) - 1; i++)
	{
		var from_node = nodes[i];
		for (var j = i + 1; j < GetLength(nodes); j++)
		{
			var to_node = nodes[j];
			// Check for the maximum connections per cave.
			if (from_node.conn_count >= RandomX(3,4) || to_node.conn_count >= RandomX(3,4))
				continue;
			// Check for two gem nodes which may not connect.
			if (from_node.is_gem && to_node.is_gem)
				continue;
			// Gem nodes may have at most one connection.
			if ((from_node.is_gem && from_node.conn_count >= 1) || (to_node.is_gem && to_node.conn_count >= 1))
				continue;
			// Cave line parameters.
			var fx = from_node.X;
			var fy = from_node.Y;
			var tx = to_node.X;
			var ty = to_node.Y;
			// Check for the maximum line distance.
			if (Distance(fx, fy, tx, ty) > max_length)
				continue;
			// Check for overlap in existing connections.
			var has_overlap = false;
			for (var line in connections)
				if (IsLineOverlap(fx, fy, tx, ty, line.Op.X[0], line.Op.Y[0], line.Op.X[1], line.Op.Y[1]))	
				{
					has_overlap = true;
					break;
				}
			if (has_overlap)
				continue;
			var tunnel_width = 2;
			// Change tunnel width if not connecting to gem node.
			if (!from_node.is_gem && !to_node.is_gem)
			{
				if (!Random(4))
					tunnel_width = 3;
				if (!Random(4))
					tunnel_width = 1;
			}
			var tunnel = {Algo = MAPALGO_Polygon, X = [fx, tx], Y = [fy, ty], Wdt = tunnel_width, Open = 1, Empty = 1};
			tunnel = {Algo = MAPALGO_Turbulence, Amplitude = 5, Scale = 3, Seed = Random(65536), Op = tunnel};
			PushBack(connections, tunnel);
			PushBack(from_node.tunnels, tunnel);
			PushBack(to_node.tunnels, tunnel);
			nodes[i].conn_count++;
			nodes[j].conn_count++;
		}
	}
	return connections;
}
