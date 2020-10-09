/**
	Flint Brawl
	
	@author Zapper
*/

#include Library_Map


// Called be the engine: draw the complete map here.
protected func InitializeMap(proplist map)
{
	// Retrieve the settings according to the MapSize setting.
	var map_size = [50, 25];
	
	// Set the map size.
	map->Resize(map_size[0], map_size[1]);
	
	// Draw the main surface: a rectangle with some turbulence on top makes.
	var rect = {X = 0, Y = 4 * map.Hgt / 10, Wdt = map.Wdt,  Hgt = 6 * map.Hgt / 10};
	var surface = {Algo = MAPALGO_Rect, X = rect.X, Y = rect.Y, Wdt = rect.Wdt, Hgt = 8 * rect.Hgt / 6};
	surface = {Algo = MAPALGO_Turbulence, Iterations = 4, Amplitude = [0, 12], Seed = Random(65536), Op = surface};
	Draw("Earth", surface);	
	
	// Draw materials inside the main surface.
	DrawMaterials(rect, surface);
	
	for (var i = 0; i < 3; ++i)
		DrawCave(RandomX(5, map_size[0] - 10), RandomX(5, map_size[1] - 10), 10, 5);
	
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
	mask = {Algo = MAPALGO_Rect, X = x,  Y = y, Wdt = wdt, Hgt = hgt};
	mask = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = mask};
	mask = {Algo = MAPALGO_And, Op = [surface, mask]}; 
	DrawMaterial("Firestone", mask, 4, 5);
	DrawMaterial("Firestone", mask);
	
	// Some small lakes as well in a second layer .
	mask = {Algo = MAPALGO_Rect, X = x,  Y = y + 1 * hgt / 4, Wdt = wdt, Hgt = hgt / 4};
	mask = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = mask};
	mask = {Algo = MAPALGO_And, Op = [surface, mask]};
	DrawMaterial("Firestone", mask, 4, 5);
	DrawMaterial("Water", mask, 4, 10);
	DrawMaterial("Ashes", mask, 4, 10);

	// The top border consists of top soil and dry earth and a bit of sand.
	var border = {Algo = MAPALGO_Border, Top = 4, Op = surface};
	Draw("Earth", border);
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = 30, Wdt = 2, Hgt = 2};
	var rnd_border = {Algo = MAPALGO_And, Op = [border, rnd_checker]};
	Draw("Ashes", rnd_border);
	Draw("Earth-earth_root", rnd_border);
	return;
}

public func DrawCave(int x, int y, int wdt, int hgt)
{
	var cave_layer = CreateLayer("Gold", wdt, hgt);
	var dug_out = cave_layer->DigOutCave();
	
	cave_layer = {Algo = MAPALGO_Filter, Op = cave_layer, Filter="~Gold"};
	cave_layer = {Algo = MAPALGO_Scale, X = 200, Y = 150, Op = cave_layer, OffX = cave_layer.Wdt/2, OffY = cave_layer.Hgt/2};
	cave_layer = {Algo = MAPALGO_Offset, Op = cave_layer, OffX = x, OffY = y};
	
	Blit(cave_layer);
	//cave_layer = {Algo = MAPALGO_Border, Op = cave_layer, Left = 2, Right = 2, Top = 2, Bottom = 2};
	//Draw("Tunnel", cave_layer);
	return dug_out;
}

public func DigOutCave()
{
	var center_x = this.Wdt / 2;
	var center_y = this.Hgt / 2;
	var runners = [[center_x, center_y]];
	var runner_count = 1;
	
	var dug_out = 0;
	var possible_x_dirs = [-1, 1, -1, 1, 0, 0];
	var possible_y_dirs = [0, 0, 0, 0, -1, 1];
	var possible_dir_count = 6;
	var tunnel = GetMaterialTextureIndex("Tunnel");
	var iterations = 5 * Max(this.Wdt, this.Hgt);
	for (var i = 0; i < iterations; ++i)
	{
		for (var r = 0; r < GetLength(runners); ++r)
		{
			var runner = runners[r];
			if (!runner) continue;
			
			// Get direction.
			var x_dir = 0, y_dir = 0;
			var r_dir = 0;
			if (!Random(9)) r_dir = r % possible_dir_count;
			else r_dir = Random(possible_dir_count);
			x_dir = possible_x_dirs[r_dir];
			y_dir = possible_y_dirs[r_dir];
			
			var x = runner[0] + x_dir;
			var y = runner[1] + y_dir;
			var solid = GetPixel(x, y) != tunnel;
			var out_of_bounds = (x < 0) || (x > this.Wdt) || (y < 0) || (y > this.Hgt);
			var destroy = out_of_bounds;
			
			runners[r] = [x, y];
			
			if (solid)
			{
				++dug_out;
				this->SetPixel(x, y, tunnel);
				if (!Random(1 + 1 * runner_count) && (runner_count < 4))
				{
					var pos = -1;
					for (var c = 0; c < GetLength(runners); ++c)
					{
						if (runners[c]) continue;
						pos = c;
						break;
					}
					if (pos == -1) PushBack(runners, [x, y]);
					else runners[pos] = [x, y];
					
					runner_count += 1;
				}
			}
			else
			{
				destroy = destroy || !Random(10);
			}
			
			if (destroy)
			{
				if (runner_count <= 1)
				{
					runners[r] = [center_x, center_y];
				}
				else
				{
					runners[r] = nil;
					runner_count -= 1;
				}
			}
		}
	}
	
	// Now perform a smoothing step. Just do it in place, meh.
	for (var smooth = 0; smooth < 1; ++smooth)
	{
		for (var x = 1; x < this.Wdt - 1; ++x)
		for (var y = 1; y < this.Hgt - 1; ++y)
		{
			var free_count = 0;
			if (GetPixel(x - 1, y) == tunnel) ++free_count;
			if (GetPixel(x + 1, y) == tunnel) ++free_count;
			if (GetPixel(x, y - 1) == tunnel) ++free_count;
			if (GetPixel(x, y + 1) == tunnel) ++free_count;
		
			if (free_count >= 3) SetPixel(x, y, tunnel);
		}
	}
	return dug_out;
}
