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

private func DrawMaterialInRange(string mat, int ymin, int ymax, spot_size, int ratio)
{
	// Create mask
	var mask = {Algo = MAPALGO_Rect, X = this.mat_rect.X,  Y = this.mat_rect.Y + ymin * this.mat_rect.Hgt / 100, Wdt = this.mat_rect.Wdt, Hgt = (ymax-ymin) * this.mat_rect.Hgt / 100 };
	mask = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = BoundBy(this.mat_rect.Hgt, 10, 50), Op = mask};
	mask = {Algo = MAPALGO_And, Op = [this.mat_surface, mask]}; 
	// Draw on it
	return DrawMaterial(mat, mask, spot_size, ratio);
}

// Draws materials on the given surface.
public func DrawMaterials(proplist rect, proplist surface)
{
	this.mat_rect = rect;
	this.mat_surface = surface;
	
	// A bit of different types of earth all around the surface.
	DrawMaterialInRange("Earth-earth_root", 0, 100, 2, 16);
	DrawMaterialInRange("Earth-earth_spongy", 0, 100, 2, 16);
	DrawMaterialInRange("Earth-earth", 0, 100, 4, 12);

	// Coal and firestone mostly in upper areas
	DrawMaterialInRange("Coal", 0, 25, 4, 5);
	DrawMaterialInRange("Coal", 25, 100, 4, 8);
	DrawMaterialInRange("Firestone", 0, 60, 4, 5);
	DrawMaterialInRange("Firestone", 60, 100, 2, 10);
	
	// Some small lakes in mid layers
	DrawMaterialInRange("Water", 25, 55, [5, 20], 20);
	
	// Ore and rock starting mostly further down
	DrawMaterialInRange("Ore", 30, 80, 3, 10);
	DrawMaterialInRange("Rock", 25, 30, 2, 4);
	DrawMaterialInRange("Rock", 30, 90, 5, 8);
	
	// A few tunnels deep down
	DrawMaterialInRange("Tunnel", 50, 75, [20, 12], 12);
	
	// Granite even further down
	DrawMaterialInRange("Granite", 50, 80, 2, 10);
	DrawMaterialInRange("Granite", 80, 100, [25, 2], 15);
	
	// Gold near the bottom only
	DrawMaterialInRange("Gold", 70, 90, 2, 8);
	
	// Lava near at bottom
	DrawMaterialInRange("DuroLava", 70, 90, [2, 40], 10);
	DrawMaterialInRange("DuroLava", 90, 120, [10, 15], 25);

	// The top border consists of top soil and dry earth and a bit of sand.
	var border = {Algo = MAPALGO_Border, Top = 4, Op = surface};
	Draw("Earth", border);
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = 30, Wdt = 2, Hgt = 2};
	var rnd_border = {Algo = MAPALGO_And, Op = [border, rnd_checker]};
	Draw("Sand", rnd_border);
	Draw("Earth-earth_root", rnd_border);
	
	// Make sure we don't have hanging liquids
	FixLiquidBorders();
	
	return;
}

