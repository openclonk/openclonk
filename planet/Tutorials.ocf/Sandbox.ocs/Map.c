

func InitializeMap(proplist map)
{
	if (!MapGenPreset) MapGenPreset = "FlatLand"; // For the initial map on scenario start
	
	// These were set in the MapGen UI
	Resize(MapGenSizeWidth ?? 80, MapGenSizeHeight ?? 50);
	
	if (MapGenPreset == "FlatLand")
	{
		Draw("Earth", { Algo = MAPALGO_Rect, X = 0, Y = MapGenSizeHeight / 2, Wdt = MapGenSizeWidth, Hgt = MapGenSizeHeight / 2 + 1 } );
	}
	else if (MapGenPreset == "Skylands")
	{
		var islands =
		{
			Algo = MAPALGO_Turbulence,
			Op = { Algo=MAPALGO_RndChecker, Wdt=5, Hgt=2, Ratio = 10 }
		};
		
		Draw("Earth", islands);
	}
	else if (MapGenPreset == "Caves")
	{
		Draw("Earth", { Algo = MAPALGO_Rect, X = 0, Y = 0, Wdt = MapGenSizeWidth, Hgt = MapGenSizeHeight } );
		
		
		for (var i = 0; i < 3; i++)
		{
			var tunnels = 
			{
				Algo = MAPALGO_Polygon,
				X = [Random(MapGenSizeWidth),Random(MapGenSizeWidth)],
				Y = [Random(MapGenSizeHeight),Random(MapGenSizeHeight)],
				Wdt = Random(10) + 3,
				Empty = true,
				Open = true
			};
			
			Draw("Tunnel", tunnels);
		}
	}
	
	
	return true;
}

global func PostMapGen()
{
	// This is called after the map is generated. Should be used to place environment objects.
}