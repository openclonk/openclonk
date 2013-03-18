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
	var spot = {Algo=MAPALGO_Ellipsis};
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
func DrawSulphur(int num, array rect) { return DrawSpots("Sulphur", num, [20,60], [8,12], rect, nil); }
func DrawOre(int num, array rect) { return DrawSpots("Ore", num, [8,12], [14,20], rect, nil); }
func DrawGold(int num, array rect) { return DrawSpots("Gold", num, [10,14], [10,14], rect, nil); }
func DrawRock(int num, array rect) { return DrawSpots("Rock-rock", num, [20,80], [6,8], rect, nil, [["Rock-rock_cracked", 3,10], ["Granite", 6,2]]); }

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

func DrawRegularGround(array rect)
{
	// Draw regular (boring) ground level
	if (!rect) rect = [0,0,this.Wdt, this.Hgt];
	var ground_rect = {Algo=MAPALGO_Rect, X=-100, Y=rect[1]+rect[3]/4, Wdt=rect[0]+rect[2]+200, Hgt=rect[3]+100};
	var ground_algo = {Algo=MAPALGO_Turbulence, Amplitude=30, Scale=30, Op=ground_rect};
	var earth_shape = DrawVaried("Earth-earth", ground_algo, rect, [["Earth-earth_dry", 3,10], ["Earth-earth_rough", 6,2]]);
	var top1 = {Algo=MAPALGO_Border, Top=[-10, 3], Op=earth_shape};
	var top2 = {Algo=MAPALGO_Border, Top=[-10, 1], Op=earth_shape};
	this->Draw("Earth-earth_midSoil", {Algo=MAPALGO_And, Op=[earth_shape, {Algo=MAPALGO_Turbulence, Amplitude=4, Scale=10, Op=top1}]});
	this->Draw("Earth-earth_topSoil", {Algo=MAPALGO_And, Op=[earth_shape, {Algo=MAPALGO_Turbulence, Amplitude=4, Scale=10, Op=top2}]});
	return true;
}

func FixLiquidBorders(border_material, lava_border_material)
{
	// Makes sure liquids bordering other liquids as well as liquids
	// bordering background materials are surrounded by fix_material
	// Default border materials
	if (!border_material) border_material = "Earth-earth_topSoil";
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