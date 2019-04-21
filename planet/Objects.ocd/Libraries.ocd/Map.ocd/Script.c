/*--
		Library_Map
		Authors: Sven2
	
		Utility functions for map drawing in InitializeMap().
--*/

// Returns p if p is an int. If p is an array of two ints, returns a
// random value between both (assuming p[1]>p[0])
func EvalIntPar(p) { if (GetType(p) == C4V_Array) return p[0]+Random(p[1]-p[0]); return p; }

// Returns p if p is an int. If p is an array of two ints, returns the
// medium value of both
func EvalIntParM(p) { if (GetType(p) == C4V_Array) return (p[0]+p[1])/2; return p; }

func DrawVaried(string mat, proplist algo, array rect, vary_mats)
{
	// Draw material and put rndchecker variations of other materials on it
	if (GetType(vary_mats) != C4V_Array || (GetLength(vary_mats)==3 && GetType(vary_mats[1])==C4V_Int)) vary_mats = [vary_mats];
	var ratio = 100/(GetLength(vary_mats)+1);
	var main_shape = this->CreateLayer();
	main_shape->Draw(mat, algo, rect);
	this->Blit(main_shape, rect);
	for (var vary_def in vary_mats)
	{
		var sx=3,sy=3;
		if (GetType(vary_def) == C4V_Array)
		{
			sx=vary_def[1]; sy = vary_def[2];
			vary_def = vary_def[0];
		}
		var rand_algo = {Algo=MAPALGO_RndChecker, Ratio=ratio, Wdt=sx, Hgt=sy};
		var turb_algo = {Algo=MAPALGO_Turbulence, Amplitude=12, Scale=8, Op=rand_algo};
		this->Draw(vary_def, {Algo=MAPALGO_And, Op=[main_shape, turb_algo]});
	}
	return main_shape;
}

func DrawSpots(string mat, int num, sizex, sizey, array rect, inmat, vary_mats)
{
	// Default parameters
	if (!inmat) inmat="Earth";
	if (!sizex) sizex = [5, 20];
	if (!sizey) sizey = [5, 7];
	if (!num) num=Max(this->GetPixelCount(inmat, rect) / (EvalIntParM(sizex)*EvalIntParM(sizey)*10), 1);
	// Draw num spots
	var spot = {Algo=MAPALGO_Ellipse};
	while (num--)
	{
		if (!this->FindPosition(spot, inmat, rect)) break;
		var mask = this->Duplicate(inmat);
		spot.Wdt = EvalIntPar(sizex)/2;
		spot.Hgt = EvalIntPar(sizey)/2;
		var algo = {Algo=MAPALGO_And, Op=[mask, {Algo=MAPALGO_Turbulence, Amplitude=Max(spot.Wdt, spot.Hgt), Scale=Max(spot.Wdt, spot.Hgt), Op=spot}]};
		if (vary_mats)
			DrawVaried(mat, algo, rect, vary_mats);
		else
			this->Draw(mat, algo, rect);
	}
	return true;
}

func DrawCoal(int num, array rect) { return DrawSpots("Coal", num, [20,60], [4,8], rect, nil); }
func DrawFirestone(int num, array rect) { return DrawSpots("Firestone", num, [20,60], [8,12], rect, nil); }
func DrawOre(int num, array rect) { return DrawSpots("Ore", num, [8,12], [14,20], rect, nil); }
func DrawGold(int num, array rect) { return DrawSpots("Gold", num, [10,14], [10,14], rect, nil); }
func DrawRock(int num, array rect) { return DrawSpots("Rock-rock", num, [20,80], [6,8], rect, nil, [["Rock-rock", 3,10], ["Granite", 6,2]]); }

// Draws the given material onto an existing mask with given size and ratio.
public func DrawMaterial(string mat, proplist onto_mask, speck_size, int ratio)
{
	// Defaults and check whether speck size is an array.
	if (!speck_size)
		speck_size = 4;
	if (!ratio)
		ratio = 15;
	if (GetType(speck_size) != C4V_Array) 
		speck_size = [speck_size, speck_size];
	// Use random checker algorithm to draw patches of the material. 
	var rnd_checker = {Algo = MAPALGO_RndChecker, Ratio = ratio, Wdt = speck_size[0], Hgt = speck_size[1]};
	rnd_checker = {Algo = MAPALGO_Turbulence, Iterations = 4, Op = rnd_checker};
	var material_mask = {Algo = MAPALGO_And, Op = [onto_mask, rnd_checker]};
	// Draw the material onto the calling map object.
	this->Draw(mat, material_mask);
	return;
}

func DrawWaterVeins(int num, array rect)
{
	while (num--) DrawLiquidVein("Water", 3, 5, rect);
	return true;
}

func DrawLiquidVein(string mat, int wdt, int spread, array rect, inmat)
{
	if (!rect) rect = [0,0,this.Wdt, this.Hgt];
	if (!inmat) inmat="Earth";
	var mask = this->Duplicate(inmat);
	var x1 = rect[0]-rect[2]+Random(rect[2]*3);
	var y1 = rect[1]-rect[3]+Random(rect[3]*3);
	var x2 = rect[0]+Random(rect[2]);
	var y2 = rect[1]+Random(rect[3]);
	var water = {Algo=MAPALGO_Polygon, X=[x1,x2], Y=[y1,y2], Wdt=wdt};
	var water_rand = {Algo=MAPALGO_Turbulence, Amplitude=30, Scale=30, Op=water};
	var water_rand2 = {Algo=MAPALGO_Turbulence, Amplitude=spread*2, Scale=2, Op=water_rand};
	this->Draw(mat, {Algo=MAPALGO_And, Op=[mask, water_rand2]});
	return true;
}

func DrawRegularGround(array rect, yoff, turbulence)
{
	// Draw regular (boring) ground level
	if (!rect) rect = [0,0,this.Wdt, this.Hgt];
	if (GetType(yoff) == C4V_Nil) yoff = rect[3]/4;
	if (GetType(turbulence) == C4V_Nil) turbulence = 30;
	var ground_rect = {Algo=MAPALGO_Rect, X=-100, Y=rect[1]+yoff, Wdt=rect[0]+rect[2]+200, Hgt=rect[3]+100};
	var ground_algo = {Algo=MAPALGO_Turbulence, Amplitude=turbulence, Scale=30, Op=ground_rect};
	var earth_shape = DrawVaried("Earth-earth", ground_algo, rect, [["Earth-earth_root", 3,10], ["Earth-earth_spongy", 6,2]]);
	var top1 = {Algo=MAPALGO_Border, Top=[-10, 3], Op=earth_shape};
	var top2 = {Algo=MAPALGO_Border, Top=[-10, 1], Op=earth_shape};
	this->Draw("Earth-earth", {Algo=MAPALGO_And, Op=[earth_shape, {Algo=MAPALGO_Turbulence, Amplitude=4, Scale=10, Op=top1}]});
	this->Draw("Earth-earth", {Algo=MAPALGO_And, Op=[earth_shape, {Algo=MAPALGO_Turbulence, Amplitude=4, Scale=10, Op=top2}]});
	return true;
}

func FixLiquidBorders(border_material, lava_border_material)
{
	// Makes sure liquids bordering other liquids as well as liquids
	// bordering background materials are surrounded by fix_material
	// Default border materials
	if (!border_material) border_material = "Earth-earth";
	if (!lava_border_material) lava_border_material = "Rock";
	// Find liquid-to-background borders
	var liquids = this->Duplicate("Liquid");
	var liquid_borders = {Algo=MAPALGO_Border, Op=liquids, Wdt=-1, Top=0 };
	var background = this->Duplicate("Background");
	background->Draw("Sky", {Algo=MAPALGO_Not, Op=this});
	this->Draw(border_material, {Algo=MAPALGO_And, Op=[liquid_borders, background]});
	// Put lava on top of other liquids
	var lava_borders = {Algo=MAPALGO_Border, Op=this->Duplicate(["Lava", "DuroLava"])};
	this->Draw(lava_border_material, {Algo=MAPALGO_And, Op=[lava_borders, liquids]});
	// Put acid on top of water
	var acid_borders = {Algo=MAPALGO_Border, Op=this->Duplicate("Acid")};
	this->Draw(border_material, {Algo=MAPALGO_And, Op=[acid_borders, liquids]});
	return true;
}

func DrawPlatform(string material, int x0, int y0, int wdt, int hgt_up, int hgt_down)
{
	var ground_mask = this->Duplicate("~Background");
	for (var x=x0; x<x0+wdt; ++x)
	{
		// Adjust materials bottom
		var y, px;
		for (y=y0+1; y<=y0+hgt_down; ++y)
		{
			px = ground_mask->GetPixel(x,y);
			if (px) break;
		}
		if (px) for (--y; y>y0; --y) this->SetPixel(x,y,px);
	}
	// Free top
	if (hgt_up) this->Draw("Sky", nil, [x0,y0-hgt_up,wdt,hgt_up]);
	// Draw platform itself
	if (material) this->Draw(material, nil, [x0,y0,wdt,1]);
}

func FindBottomPeaks(array rect, int min_dx)
{
	// Find downwards peaks in bottom-most nonzero pixels in rect
	// Peaks are minimum min_dx apart horizontally
	var x1,y1,x2,y2;
	if (rect)
		{ x1=rect[0]; y1=rect[1]; x2=x1+rect[2]; y2=y1+rect[3]; }
	else
		{ x2=this.Wdt; y2=this.Hgt; }
	var last_y = y2+1, last_dy = 0, num_dy_0 = 0, last_peak_x = -min_dx-1;
	var peaks = [];
	for (var x=x1; x<x2; ++x)
	{
		var y = y2;
		while (!this->GetPixel(x,y)) if (y1>=--y) break;
		var dy = y - last_y;
		if (!dy)
		{
			++num_dy_0;
		}
		else
		{
			if (dy < 0 && last_dy > 0)
			{
				var peak_x = x-1-num_dy_0/2;
				if (num_dy_0%2 && !Random(2)) --peak_x;
				if (peak_x-last_peak_x < min_dx)
				{
					if (!Random(2)) peaks[GetLength(peaks)-1] = {X=peak_x, Y=last_y};
				}
				else
				{
					peaks[GetLength(peaks)] = {X=peak_x, Y=last_y};
				}
				last_peak_x = peaks[GetLength(peaks)-1].X;
			}
			num_dy_0 = 0;
			last_dy = dy;
		}
		last_y = y;
	}
	return peaks;
}

func FillLiquid(string mat, int x, int y, max_wdt, int max_hgt)
{
	// Fill area downwards and sideways of given x and y with mat
	// Stay within rectangle x-max_wdt and x+max_wdt horizontally and within y and y+max_hgt vertically
	var background_mask = this->CreateMatTexMask("Background");
	var open_ranges = [[x,x]], n_open = 1;
	max_hgt = BoundBy(max_hgt, 0, this.Hgt-y);
	if (GetType(max_wdt) != C4V_Array) max_wdt = [max_wdt,max_wdt];
	var min_x1 = Max(x-max_wdt[0]), min_x2 = Max(x-max_wdt[1]);
	var max_x1 = Min(x+max_wdt[0], this.Wdt-1), max_x2 = Min(x+max_wdt[1], this.Wdt-1);
	var min_x = (min_x1+min_x2)/2, max_x = (max_x1+max_x2)/2;
	var n_pix_set = 0;
	while (n_open && max_hgt)
	{
		var next_ranges = [], n_next = 0;
		x = -1;
		for (var range in open_ranges)
		{
			var x1=range[0], x2=range[1];
			if (x>x1) continue; // two or more paths merged
			while (x1>min_x && background_mask[this->GetPixel(x1-1,y)]) --x1;
			while (x2<max_x && background_mask[this->GetPixel(x2+1,y)]) ++x2;
			var below_was_background = false;
			for (x=x1; x<=x2; ++x)
			{
				this->SetPixel(x,y,mat); ++n_pix_set;
				var below_is_background = background_mask[this->GetPixel(x,y+1)];
				if (below_is_background)
				{
					if (!below_was_background)
						next_ranges[n_next++] = [x,x];
					else
						++next_ranges[n_next-1][1];
				}
				below_was_background = below_is_background;
			}
		}
		open_ranges = next_ranges;
		n_open = n_next;
		++y; --max_hgt;
		min_x = BoundBy(min_x+Random(3)-1, min_x1,min_x2);
		max_x = BoundBy(max_x+Random(3)-1, max_x1,max_x2);
	}
	return n_pix_set;
}
