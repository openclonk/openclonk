/* A map for testing the Random() number generator. */

#include Library_Map

// Overload Random to make it show up in the script profiler.
func Random()
{
	return inherited(...);
}

func InitializeMap(proplist map)
{
	Resize(800, 800);
	var layer1 = CreateLayer(nil, map.Wdt, map.Hgt / 2);
	var layer2 = CreateLayer(nil, map.Wdt, map.Hgt / 2);
	StartScriptProfiler();
	DrawRandomPattern(layer1, "Water");
	DrawRandomPattern2(layer2);
	StopScriptProfiler();
	Blit(layer1, [0, 0, layer1.Wdt, layer1.Hgt]);
	Blit({Algo = MAPALGO_Offset, OffY = map.Hgt / 2, Op = layer2});
	return true;
}

// For each pixel: either fill or don't fill.
func DrawRandomPattern(proplist layer, string mat)
{
	for (var y = 0; y < layer.Hgt; y++)
		for (var x = 0; x < layer.Wdt; x++)
			if (!Random(2))
				layer->SetPixel(x, y, mat);
}

// Fill each pixel with a random material.
func DrawRandomPattern2(proplist layer)
{
	var mats = [ "Acid", "Amethyst", "Ashes", "Brick", "BrickSoft", "Coal", "DuroLava", "Earth", "Everrock", "Firestone", "Gold", "Granite", "HalfVehicle", "Ice", "Lava", "ORE", "Rock", "Ruby", "SandDry", "Sand", "Snow", "Tunnel", "Vehicle", "Water" ];
	var n = GetLength(mats);

	for (var y = 0; y < layer.Hgt; y++)
		for (var x = 0; x < layer.Wdt; x++)
			layer->SetPixel(x, y, mats[Random(n)]);
}
