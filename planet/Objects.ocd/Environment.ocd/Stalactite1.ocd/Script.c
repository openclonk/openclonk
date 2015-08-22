/**
	Stalactite1
	Hangs from the ceiling

	@author Armin, Win
*/

private func Initialize()
{
	SetGraphics(Format("%d", Random(6)));
	SetProperty("MeshTransformation", Trans_Mul(Trans_Scale(600 + Random(900)), Trans_Rotate(-25 + Random(50), 0, 1, 0)));
}

private func Hit()
{
	//Sound();
	RemoveObject();
	return true;
}

public func Place(int amount, proplist rectangle, proplist settings)
{
	// Only allow definition call.
	if (this != Stalactite1)
		return;
	// Default parameters.
	if (!settings)
		settings = { size = [100, 100] };
	if (!settings.size)
		settings.size = [100, 100];
	var loc_area = nil;
	if (rectangle)
		loc_area = Loc_InRect(rectangle);
	var loc_background = Loc_Tunnel();
	if (settings.underground)
		loc_background = Loc_Tunnel();

	var stalactites = [];
	for (var i = 0; i < amount; i++)
	{
		var size = RandomX(settings.size[0], settings.size[1]);
		var loc = FindLocation(loc_background, Loc_Not(Loc_Liquid()), Loc_Wall(CNAT_Right), Loc_Wall(CNAT_Top), loc_area);
		if (!loc)
			continue;
		var mat = MaterialName(GetMaterial(loc.x, loc.y-30));
		CreateStalactite(loc.x, loc.y + 25, mat);

		// If possible, try to create a stalagmite below the stalactite.
		if (Random(3))
		{
			var xy = FindConstructionSite(Stalactite1, loc.x, loc.y+60);
			if (xy)
				if (MaterialName(GetMaterial(loc.x, xy[1] + 2 + 30)) == mat)
					CreateStalactite(loc.x, xy[1]-28, mat, true);
		}
	}
	return stalactites;
}

private func CreateStalactite(int x, int y, string mat, bool stalagmite)
{
	var stalactite = CreateObject(Stalactite1, x, y);
	var tinys = CreateObject(TinyStalactite, x, y - 22);
	tinys->SetChild(stalactite);

	// Ice stalactites are transparent and never use water sources.
	if (mat == "Ice")
	{
		stalactite->SetClrModulation(RGBa(157, 202, 243, 160));
		tinys->SetClrModulation(RGBa(157, 202, 243, 160));
	}
	else
	{
		stalactite->SetClrModulation(GetAverageTextureColor(mat));
		tinys->SetClrModulation(GetAverageTextureColor(mat));
		if (!stalagmite && Random(2))
			tinys->DrawWaterSource();
	}

	if (stalagmite)
	{
		stalactite->SetR(180);
		tinys->SetPosition(x, y + 22);
		tinys->SetR(180);
	}
	// todo stalactite->AdjustPosition();
}

local Name = "$Name$";
local Description = "$Description$";
