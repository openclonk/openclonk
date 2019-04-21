/**
	Blue lake
	Dynamic map with a big lake containing islands of material
	Plus lava basins with gems at the bottom
	Parts adapted from Gem Grabbers by Maikel
	
	@authors Sven2
*/

#include Library_Map

// zoomed coordinates for scenario script
static main_island_x, main_island_y;
static goal_platform_x, goal_platform_y;

// set after intro to force map creation
// g_intro_done;

// Called be the engine: draw the complete map here.
public func InitializeMap(proplist map)
{
	if (!g_intro_done && !SCEN_TEST) return true;
	Resize(300,400);
	this.sea_y = 50;
	this.ground_y = 350;
	this.map_zoom = 7;
	main_island_x = this.Wdt/2 * this.map_zoom;
	main_island_y = this.sea_y * this.map_zoom;
	
	Draw("Water", nil, [0,this.sea_y,this.Wdt,this.Hgt]);
	DrawMainIsland(80);
	DrawGround();
	
	// Regular resource islands
	DrawSecondaryIslands(3, 15, [["Ore", 50], ["Coal", 40]], true);
	DrawSecondaryIslands(10, 6, [["Firestone", 70]], false);
	DrawSecondaryIslands(3, 8, [["Gold", 40]], true);
	
	// Amethyst islands
	var i=0, imod=Random(2);
	while (i<3 || GetPixelCount("Amethyst")<15)
	{
		DrawSecondaryIsland(8, [["Amethyst", 70]], true, [0, this.Wdt-70][(i+imod)%2], 70, this.sea_y+50);
		++i;
	}
	
	FixLiquidBorders("Earth");
	
	// Ensure that left and right side are always open because they're used for water refill
	Draw("Sky", nil, [0,0,1,this.sea_y]);
	Draw("Sky", nil, [this.Wdt-1,0,1,this.sea_y]);
	Draw("Water", nil, [0,this.sea_y,1,70]);
	Draw("Water", nil, [this.Wdt-1,this.sea_y,1,70]);
	
	// Top level of water has sky background
	Draw("^Water", Duplicate("Water"), [0,this.sea_y,this.Wdt,11]);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Draws the main island with all basic resources
private func DrawMainIsland(int size)
{
	// Shape of the main island. Shape taken from Gem Grabbers.
	var island_algo = {Algo = MAPALGO_Polygon};
	var x = this.Wdt / 2;
	var y = this.sea_y;
	island_algo.X = [x-size/3, x-size/2, x-size/3, x-size/6, x+size/6, x+size/3, x+size/2, x+size/3];
	island_algo.Y = [y-size/6, y, y+size/3, y+size/6, y+size/6, y+size/3, y, y-size/6];
	
	// Draw the earth patch of the island.
	island_algo = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = island_algo};
	var island = CreateLayer();
	island->Draw("Earth", island_algo);
	
	// Draw goal platform shape
	while (island->GetPixel(x,y)) ++x; // Find right side of island at sea level
	var platform_algo = {Algo = MAPALGO_Polygon};
	platform_algo.X = [x-5,x+14,x+14,x+7,x  ,x-5];
	platform_algo.Y = [y  ,y   ,y+ 1,y+2,y+4,y  ];
	island->Draw("Earth", platform_algo);
	
	// Preserve drawn island shape for border algorithms
	var island_shape = island->Duplicate();
		
	// Overlay a set of materials inside the island.
	DrawIslandMat("Earth-earth_root", island, 4, 30, true);
	DrawIslandMat("Earth-earth", island, 3, 30, true);
	DrawIslandMat("Tunnel", island, 3, 10, true);
	//DrawIslandMat("Water", island, 4, 8);
	//DrawIslandMat("Gold", island, 3, 6);
	DrawIslandMat("Ore", island, 6, 18, true);
	DrawIslandMat("Coal", island, 6, 18, true);
	DrawIslandMat("Firestone", island, 6, 12, true);
	
	// Draw a top border out of sand and top soil.
	var sand_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Op = island_shape, Top = [-1,2]}, {Algo = MAPALGO_RndChecker, Ratio = 50, Wdt = 4, Hgt = 3}]};
 	var topsoil_border = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Border, Op = island_shape, Top = [-1,3]}, {Algo = MAPALGO_RndChecker, Ratio = 40, Wdt = 4, Hgt = 2}]};
	island->Draw("Sand", sand_border);
	island->Draw("Earth-earth", topsoil_border);
	
	// Draw a bottom border out of granite and rock.
	var granite_border = {Algo = MAPALGO_Border, Op = island_shape, Bottom = [-4,3]};
	island->Draw("Granite", granite_border);
	var rock_border = {Algo = MAPALGO_RndChecker, Ratio = 20, Wdt = 2, Hgt = 2};
	island->Draw("Rock", {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
	island->Draw("Rock-rock", {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
	
	// Draw goal platform
	island->Draw("Sky", nil, [x,y-10,14,10]);
	island->Draw("Brick", nil, [x,y,14,2]);
	goal_platform_x = (x+7)*this.map_zoom;
	goal_platform_y = y    *this.map_zoom;
	
	// Draw island onto main map
	Blit(island);
	
	return true;
}

// Draws multiple underwater resource islands
private func DrawSecondaryIslands(int n, ...)
{
	for (var i=0; i<n; ++i) DrawSecondaryIsland(...);
	return true;
}

// Draws underwater resource island
private func DrawSecondaryIsland(int size, array materials, bool has_border, int xmin, int xwdt, int ymin)
{
	// Find a free spot underwater
	var x,y;
	var border = size; // left and right border
	if (!xwdt) xwdt = this.Wdt;
	if (!ymin) ymin = this.sea_y;
	var i_tries = 200;
	while (i_tries--)
	{
		var x = Random(xwdt - border*2) + border + xmin;
		var y = Random(this.ground_y - ymin - size) + ymin + size/2;
		if (GetPixelCount("Solid", [x-size,y-size,size,size])) continue;
		break;
	}
	
	// Shape of the resource island
	var island_algo = {Algo = MAPALGO_Ellipse, X=x, Y=y, Wdt=size, Hgt=size};
	island_algo = {Algo = MAPALGO_Turbulence, Amplitude = [20,5], Iterations = 3, Op = island_algo};
	var island = CreateLayer();
	island->Draw("Earth", island_algo);
	var island_shape = island->Duplicate();
	
	DrawIslandMat("Earth-earth_spongy", island, 4, 30);
	DrawIslandMat("Earth-earth", island, 3, 30);
		
	// Overlay a set of materials inside the island.
	for (var mat in materials)
	{
		DrawIslandMat(mat[0], island, 3, mat[1], has_border);
	}
	
	// Draw a border out of granite and rock.
	if (has_border)
	{
		var island_shape = island->Duplicate();
		var granite_border = {Algo = MAPALGO_Border, Op = island_shape, Top = [-2,2]};
		island->Draw("Granite", granite_border);
		var rock_border = {Algo = MAPALGO_RndChecker, Ratio = 20, Wdt = 2, Hgt = 2};
		island->Draw("Rock", {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
		island->Draw("Rock-rock", {Algo = MAPALGO_And, Op = [granite_border, rock_border]});
	}
	
	// Draw island onto main map
	Blit(island);
	
	return true;
}

private func DrawGround()
{
	// Bottom of the sea
	var ground_algo = { Algo = MAPALGO_Rect, X=-100, Y=this.ground_y, Wdt=this.Wdt+200, Hgt=this.Hgt-this.ground_y+1000 };
	ground_algo = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = [10,100], Scale = 20, Op = ground_algo };
	var ground2_algo = { Algo = MAPALGO_Rect, X=-100, Y=this.Hgt-10, Wdt=this.Wdt+200, Hgt=this.Hgt-this.ground_y+1000 };
	ground2_algo = {Algo = MAPALGO_Turbulence, Iterations = 2, Amplitude = 10, Scale = 30, Op = ground2_algo };
	var ground = CreateLayer();
	// Ensure lots of earth
	while (true)
	{
		ground->Draw("Earth", ground_algo);
		ground->Draw("Granite", ground2_algo);
		if (ground->GetPixelCount("Earth") > 10000) break;
	}
	ground->Draw("DuroLava", {Algo=MAPALGO_Turbulence, Amplitude=10, Scale=20, Iterations=3, Op={Algo=MAPALGO_And, Op=[ground->Duplicate("Granite"), {Algo = MAPALGO_RndChecker, Ratio=50, Wdt=30, Hgt=2}]}});
	// Granite/Rock top border
	ground->Draw("Granite", {Algo = MAPALGO_Turbulence, Amplitude = 5, Iterations = 1, Op = {Algo = MAPALGO_Border, Op = ground->Duplicate(), Top= [-2,2]}});
	ground->Draw("Rock", {Algo=MAPALGO_And, Op=[ground->Duplicate("Granite"), {Algo = MAPALGO_RndChecker, Ratio = 40, Wdt = 2, Hgt = 2}]});
	// Alterations in earth material
	DrawIslandMat("Earth-earth_spongy", ground, 12, 60, false);
	DrawIslandMat("Earth-earth", ground, 8, 60, false);
	// Gem spots
	var gem_spots = CreateLayer();
	var earth_mats = CreateMatTexMask("Earth");
	var i=0;
	while (i<3 || gem_spots->GetPixelCount("Ruby") < 15)
	{
		var gem_mat = "Ruby";
		// Find an earth spot
		var pos = {X=Random(this.Wdt), Y=this.Hgt/2+Random(this.Hgt/2)};
		ground->FindPosition(pos, "Earth");
		// Find center of this earth area
		var x=pos.X, y=pos.Y;
		var x0=x-1, x1=x+1, y0=y-1, y1=y+1;
		while (earth_mats[ground->GetPixel(x,y0)]) --y0;
		while (earth_mats[ground->GetPixel(x,y1)]) ++y1;
		y=Min((y0+y1)/2, this.Hgt-6);
		while (earth_mats[ground->GetPixel(x0,y)]) --x0;
		while (earth_mats[ground->GetPixel(x1,y)]) ++x1;
		x=(x0+x1)/2;
		var size = 9;
		// Lava basin here
		var lava_algo = {Algo = MAPALGO_Ellipse, X=x, Y=y, Wdt=size, Hgt=size};
		lava_algo = {Algo = MAPALGO_Turbulence, Amplitude = 5, Iterations = 5, Op = lava_algo};
		gem_spots->Draw("DuroLava", lava_algo);
		// Gems at bottom center
		y += 2;
		size = 3;
		var gem_algo = {Algo = MAPALGO_Ellipse, X=x, Y=y, Wdt=size, Hgt=size};
		gem_algo = {Algo = MAPALGO_Turbulence, Amplitude = 3, Iterations = 1, Op = gem_algo};
		gem_spots->Draw(gem_mat, gem_algo);
		// Draw to map
		ground->Blit(gem_spots);
		++i;
	}
	// Lava basins surrounded by granite
	ground->Draw("Rock", {Algo=MAPALGO_And, Op=[{Algo=MAPALGO_Not, Op=gem_spots}, {Algo = MAPALGO_Turbulence, Amplitude = 5, Iterations = 5, Op = {Algo = MAPALGO_Border, Op = gem_spots, Wdt=-4}}]});
	ground->Draw("Granite", {Algo = MAPALGO_Border, Op = gem_spots, Wdt=-1});
	// Lots of rocks
	DrawIslandMat("Rock", ground, 2, 20, false);
	DrawIslandMat("Rock", ground, 2, 20, false);
	// And some lava
	DrawIslandMat("DuroLava", ground, 2, 20, false);
	// Other materials (rare)
	DrawIslandMat("Ore", ground, 12, 8, false);
	DrawIslandMat("Coal", ground, 12, 8, false);
	DrawIslandMat("Gold", ground, 12, 8, false);
	DrawIslandMat("Firestone", ground, 12, 8, false);
	Blit(ground);
	return true;
}

// Draws some material inside an island.
private func DrawIslandMat(string mat, proplist onto_mask, int speck_size, int ratio, has_border)
{
	if (!speck_size)
		speck_size = 3;
	if (!ratio)
		ratio = 20;
	// Use random checker algorithm to draw patches of the material. 
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = ratio, Wdt = speck_size, Hgt = speck_size};
	rnd_checker = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = rnd_checker};
	var algo;
	if (has_border)
	{
		var mask_border = {Algo = MAPALGO_Border, Op = onto_mask, Wdt = 3};
		algo = {Algo = MAPALGO_And, Op = [{Algo = MAPALGO_Xor, Op = [onto_mask->Duplicate("Earth"), mask_border]}, rnd_checker]};
	}
	else
	{
		algo = {Algo = MAPALGO_And, Op = [onto_mask->Duplicate("Earth"), rnd_checker]};
	}
	onto_mask->Draw(mat, algo);
	 
	return true;
}
