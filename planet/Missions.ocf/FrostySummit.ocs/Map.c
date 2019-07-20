#include Library_Map

func InitializeMap(map)
{
	// Mountain top
	var mountain_shape = { Algo = MAPALGO_Polygon, X=[-100, 0, 10, 30, 50, 46, 46,-100], Y=[80, 80, 70, 90, 90, 100, 200, 200] };
	DrawVaried("Ice-ice", { Algo = MAPALGO_Turbulence, Op={Algo = MAPALGO_Offset, OffY=-5, Op = mountain_shape}, Amplitude=[10, 0], Scale = 20}, nil, "Ice-ice2");
	Draw("Snow", {Algo = MAPALGO_Border, Op = Duplicate(), Top = 1});
	DrawVaried("Rock", { Algo = MAPALGO_Turbulence, Op = mountain_shape, Amplitude=[20, 0], Scale = 10}, nil, "Earth");
	
	// Sky islands
	var sky_islands = CreateLayer();
	var n_islands = 10, x, y;
	for (var i = 0; i<n_islands; ++i)
	{
		x = 50 + Random(10)+Cos(180 + i*90/n_islands, 190)+190;
		y = 75 + Random(3)-Sin(i*90/n_islands, 65);
		var wdt = 2 + Random(5), hgt = 2 + Random(2);
		if (i>=n_islands/2) wdt = 2 + Random(wdt);
		sky_islands->DrawVaried("Granite", {Algo = MAPALGO_Turbulence, Op={Algo = MAPALGO_Ellipse, X = x, Y = y, Wdt = wdt, Hgt = hgt}, Amplitude = 10, Scale = 5}, nil, "Rock");
	}
	
	// Final ruby island
	x = 230 + Random(15); y = 15;
	sky_islands->Draw("Granite", {Algo = MAPALGO_Ellipse, X = x, Y = y, Wdt = 5, Hgt = 2});
	sky_islands->Draw("Ruby", {Algo = MAPALGO_Ellipse, X = x, Y = y + 3, Wdt = 3, Hgt = 2});
	sky_islands->Draw("Brick", nil, [x-3, y + 2, 7, 1]);
	
	// Blit islands + snow
	Blit(sky_islands);
	Draw("Snow", {Algo = MAPALGO_Border, Op = sky_islands, Top=-1});
	
	// Starting platform
	DrawPlatform("Brick", 34, 90, 20, 10, 10);
	Draw("Snow", nil, [34, 89, 5, 1]);
	
	return true;
}