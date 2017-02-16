/* Default scenario map */
// Use gold rush map with minor adjustments

#include Library_Map

// Called be the engine: draw the complete map here.
public func InitializeMap(proplist map)
{
	// Draw the main surface: a rectangle with some turbulence on top makes.
	var rect = {X = 0, Y = Min(map.Hgt / 7 + 5, map.Hgt / 3), Wdt = map.Wdt,  Hgt = 6 * map.Hgt / 10};
	rect.Hgt = map.Hgt - rect.Y;
	var surface = {Algo = MAPALGO_Rect, X = rect.X, Y = rect.Y, Wdt = rect.Wdt, Hgt = 8 * rect.Hgt / 6};
	surface = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = [0, 12], Seed = Random(65536), Op = surface};
	Draw("Earth", surface);	
	
	// Draw materials inside the main surface.
	DrawMaterials(rect, surface);
	
	// Return true to tell the engine a map has been successfully created.
	return true;
}

// Draws materials on the given surface.
public func DrawMaterials(proplist rect, proplist surface)
{
	var mask;
	var x = rect.X;
	var y = rect.Y;
	var wdt = rect.Wdt;
	var hgt = rect.Hgt;
	
	// A bit of different types of earth all around the surface.
	mask = {Algo = MAPALGO_Rect, X = x,  Y = y, Wdt = wdt, Hgt = hgt};
	mask = {Algo = MAPALGO_And, Op = [surface, mask]};
	DrawMaterial("Earth-earth", mask, 4, 12);
	DrawMaterial("Earth-earth_root", mask, 2, 16);
	DrawMaterial("Earth-earth_spongy", mask, 2, 16);
	DrawMaterial("Earth-earth", mask, 4, 12);

	// Coal and surface in the first layer.
	mask = {Algo = MAPALGO_Rect, X = x,  Y = y, Wdt = wdt, Hgt = hgt / 4};
	mask = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = mask};
	mask = {Algo = MAPALGO_And, Op = [surface, mask]}; 
	DrawMaterial("Firestone", mask, 4, 5);
	DrawMaterial("Coal", mask, 4, 5);
	DrawMaterial("Firestone", mask);
	DrawMaterial("Coal", mask);
	
	// Some small lakes as well in a second layer .
	mask = {Algo = MAPALGO_Rect, X = x,  Y = y + 1 * hgt / 4, Wdt = wdt, Hgt = hgt / 4};
	mask = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = mask};
	mask = {Algo = MAPALGO_And, Op = [surface, mask]};
	DrawMaterial("Coal", mask, 3, 8);
	DrawMaterial("Firestone", mask, 4, 5);
	DrawMaterial("Water", mask, 4, 10);
	
	// Ore and rock in the third layer.
	mask = {Algo = MAPALGO_Rect, X = x,  Y = y + 2 * hgt / 4, Wdt = wdt, Hgt = hgt / 4};
	mask = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = mask};
	mask = {Algo = MAPALGO_And, Op = [surface, mask]}; 
	DrawMaterial("Ore", mask, 3, 10);
	DrawMaterial("Rock", mask, 2, 8);
	DrawMaterial("Granite", mask, 2, 8);
	DrawMaterial("Rock", mask);
	DrawMaterial("Ore", mask);
	
	// Gold in the last layer.
	mask = {Algo = MAPALGO_Rect, X = x,  Y = y + 3 * hgt / 4, Wdt = wdt, Hgt = hgt / 4};
	mask = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = mask};
	mask = {Algo = MAPALGO_And, Op = [surface, mask]}; 
	DrawMaterial("Gold", mask, 2, 5);
	DrawMaterial("Coal", mask, 2, 10);
	DrawMaterial("Gold", mask, 2, 5);
	DrawMaterial("Gold", mask, 5, 10);

	// The top border consists of top soil and dry earth and a bit of sand.
	var border = {Algo = MAPALGO_Border, Top = 4, Op = surface};
	Draw("Earth", border);
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = 30, Wdt = 2, Hgt = 2};
	var rnd_border = {Algo = MAPALGO_And, Op = [border, rnd_checker]};
	Draw("Sand", rnd_border);
	Draw("Earth-earth_root", rnd_border);
	return;
}

