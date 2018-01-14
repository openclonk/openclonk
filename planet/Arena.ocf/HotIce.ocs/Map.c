/*
	Hot Ice
	Ice islands above a lava lake
	
	@authors Sven2
*/

static g_theme;

// Called be the engine: draw the complete map here.
public func InitializeMap(proplist map)
{
	InitTheme();

	// Map type 0: One big island; more small islands above
	// Map type 1: Only many small islands
	var t = SCENPAR_MapType;
	var w = map.Wdt, h=map.Hgt;
	g_map_width = w;
	
	if (g_theme.BackgroundMat)
		map->Draw(g_theme.BackgroundMat, nil, [0,0,w,h]);
	// Bottom lava lake
	map->Draw(g_theme.LavaMat, nil, [0,h*4/5,w,h/5]);
	
	if (t == 0) DrawBigIslandMap(map);
	if (t == 1) DrawSmallIslandsMap(map);
	
	// Alternate texctures
	var icealt_tex = { Algo=MAPALGO_RndChecker, Wdt=2, Hgt=3, Ratio=g_theme.AltMatRatio };
	icealt_tex = { Algo=MAPALGO_Turbulence, Op=icealt_tex };
	icealt_tex = { Algo=MAPALGO_And, Op=[Duplicate(RegexMatch(g_theme.IceMats[1], "\\w+", Regex_FirstOnly)[0][0]), icealt_tex]};
	map->Draw(g_theme.IceMats[0], icealt_tex);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

func InitTheme()
{
	var theme = SCENPAR_Theme;
	if (theme == 0) // Random Ice
		theme = 1 + Random(3);
	if (theme == 1) // Hot Ice
		return g_theme = HotIce;
	if (theme == 2) // Miami Ice
		return g_theme = MiamiIce;
	if (theme == 3) // Eci Toh
		return g_theme = EciToh;
}

func SpawnPositionCount()
{
	return Max(Max(GetStartupPlayerCount(), GetPlayerCount()), 2);
}

func DrawBigIslandMap(proplist map)
{
	var w = map.Wdt, h=map.Hgt;
	// Draw one big island as the ground and some smaller islands floating above
	// Big
	var island = { Algo=MAPALGO_Polygon, X=[0,w,w*6/8,w*2/8], Y=[h*4/10,h*4/10,h*7/10,h*7/10] };
	island = { Algo=MAPALGO_Turbulence, Op=island, Amplitude=[0, 8] };
	map->Draw(g_theme.IceMats[1], island, [w/10,h*13/20,w*8/10,h*3/20]); 
	// Make sure one row of inner island is drawn because it's used for player spawns
	map->Draw(g_theme.IceMats[1], nil, [w*3/10,h*13/20,w*4/10+1,1]); 
	// Smaller floating
	var n_islands = 12;
	while(n_islands--)
	{
		var x = w*1/10 + Random(w*8/10);
		var y = h*2/10 + Random(h*3/10);
		map->Draw(g_theme.IceMats[1], nil, [x,y,1,1]);
	}
	// Player spawns simply in middle of big island
	var plrcnt = SpawnPositionCount();
	g_player_spawn_positions = CreateArray(plrcnt);
	for (var i = 0; i < plrcnt; ++i)
	{
		g_player_spawn_positions[i] = [w*3/10 + i*w*4/10/(plrcnt-1), h*13/20-1];
	}
	return true;
}

func DrawSmallIslandsMap(proplist map)
{
	var w = map.Wdt, h=map.Hgt, x, y, szx, szy;
	// Islands in center of map
	var n_islands = 35;
	while(n_islands--)
	{
		y = h*3/10 + Random(h*5/10 - 3);
		var xrange = w * (y)/(h*9/10);
		x = w/2 - xrange/2 + Random(xrange);
		szx = Random(3);
		szy = 1;
		if (y > h/2) szy += Random(2); // lower islands sometimes taller
		if (Abs(x-w/2) < w/10) szx += Random(3); // central islands sometimes wider
		map->Draw(g_theme.IceMats[1], nil, [x-szx,y,1+2*szx,szy]);
	}
	// Balloon spawn: do nothing further
	if (SCENPAR_SpawnType == 1)
		return true;
	// Starting islands for player spawns
	var spawn_island_count = SpawnPositionCount();
	g_player_spawn_positions = CreateArray(spawn_island_count);
	for (var i = 0; i < spawn_island_count; ++i)
	{
		var x = w*2/10 + i * (w*6/10) / (spawn_island_count - 1);
		var y = Max(1, h/10) + Abs(x-w/2) * 3*h/10/w;
		if (SCENPAR_Weapons) y += 4; // Grenade launcher mode needs lower starting islands to prevent camping
		map->Draw(g_theme.IceMats[1], nil, [x,y,1,1]);
		g_player_spawn_positions[i] = [x, y-1];
	}
	return true;
}
