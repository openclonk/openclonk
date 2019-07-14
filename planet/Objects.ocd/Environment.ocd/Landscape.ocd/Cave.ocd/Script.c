/**
	Cave
	Creates a nice cave.
*/

/*
	Randomly places a cave without a border unless specified otherwise.
	
	Custom properties are:
	
	width, height:
		size of the cave's bounding box
	borderwidth, borderheight:
		thickness of the border
	bordersize:
		implicit borderwidth and borderheight
	bordermat:
		material of the border, f.e. "Rock"
	bordertex:
		texture of the border, f.e. "rock_cracked"
	
*/
public func Place(int amount, proplist area, proplist settings)
{
/*
	var caves = Landscape_Cave->Place(1, nil, {width = 100, height = 50, borderheight = 5, bordermat = "Earth", bordertex = "earth_topSoil" } );
	if (GetLength(caves) == 0) FatalError("No cave placed!");
*/
	var caves = [];
	amount = amount ?? ((LandscapeHeight() * LandscapeWidth()) / 1000);
	
	var width = settings.width ?? 100;
	var height = settings.height ?? 100;
	var realwidth = 50, realheight = 50;
	
	if (width < height)
	{
		var aspect = (100 * width) / height;
		realwidth = (aspect * realheight) / 100;
	}
	else
	{
		var aspect = (100 * height) / width;
		realheight = (aspect * realwidth) / 100;
	}
	var x = (100 - realwidth) / 2;
	var y = (100 - realheight) / 2;
	
	var borderstring = "";
	if (settings.bordersize || settings.borderwidth || settings.borderheight)
	{
		var bordermat = settings.bordermat ?? "Rock";
		var bordertex = settings.bordertex ?? "rock";
		var borderwidth = settings.borderwidth ?? settings.bordersize ?? 0;
		var borderheight = settings.borderheight ?? settings.bordersize ?? 0;
		borderstring = Format("overlay { mat = %s; tex = %s; algo = border; a = %d; b = %d; loosebounds = 1; };", bordermat, bordertex, borderwidth, borderheight);
	}
	
	var map = Format("overlay { x = %d; y = %d; wdt = %d; hgt = %d; mat = Tunnel; loosebounds = 1; turbulence = 500; %s };", x, y, realwidth, realheight, borderstring);

	var failsafe = 100;
	
	while ((--failsafe > 0) && amount > 0)
	{
		var spot = FindLocation(Loc_Solid(), Loc_Func(Landscape_Cave.IsGoodCaveSpot), Loc_InArea(area));
		if (!spot) continue;
		
		DrawMap(spot.x - width, spot.y - height, 2 * width, 2 * height, Format("map Cave { %s };", map));
		var cave = CreateObjectAbove(Landscape_Cave, spot.x, spot.y, NO_OWNER);
		PushBack(caves, cave);
		
		// project free objects down to the ground
		for (var obj in FindObjects(Find_InRect(spot.x - width, spot.y - height, 2 * width, 2 * height), Find_Category(C4D_Object)))
		{
			var originaly = obj->GetY();
			var max = height;
			while ((!obj->GetContact(-1, CNAT_Bottom)) && !obj->Stuck() && (--max > 0))
			{
				obj->SetPosition(obj->GetX(), obj->GetY() + 1);
			}
			if (max <= 0) obj->SetPosition(obj->GetX(), originaly);
		}
		
		--amount;
	}
	
	return caves;
}

func IsGoodCaveSpot(int x, int y)
{
	// cave must be "far" underground!
	var no_sky_distance = 200;
	for (var yd = 0; yd <= no_sky_distance; yd += 10)
		if (GBackSky(x, y - yd)) return false;
	
	var cave = FindObjects(Find_ID(Landscape_Cave), Sort_Distance(x, y));
	if (GetLength(cave) == 0) return true;
	cave = cave[0];
	return Distance(x, y, cave->GetX(), cave->GetY()) > 100;
}
