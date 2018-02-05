/**
	Stalactite
	Hangs from the ceiling

	@author Armin, Win, Zapper
*/

public func Construction()
{
	this.MeshTransformation = Trans_Rotate(Random(360), 0, 1, 0);
	
	var sibling = nil;
	for (var bone in ["bone1", "bone2", "bone3"])
	{
		var transformation;
		var rand = Random(2);
		if (rand == 0) 
			transformation = Trans_Rotate(RandomX(-20, 20), 1, 0, 0);
		else if(rand == 1)
			transformation = Trans_Rotate(RandomX(-20, 20), 0, 0, 1);
		sibling = TransformBone(bone, transformation, 1, Anim_Const(1000), sibling);
	}
}

private func Hit()
{
	var colour = GetClrModulation();
	var particles = 
	{
		Size = PV_KeyFrames(0, 0, 0, 100, PV_Random(3, 5), 1000, 3),
		R = (colour >> 16) & 0xff,
		G = (colour >>  8) & 0xff,
		B = (colour >>  0) & 0xff,
		Alpha = PV_Linear(255, 0),
		ForceY = PV_Gravity(100),
		CollisionVertex = 0
	};
	
	var width = GetCon() * 7 / 100;
	var height = GetCon() * 60 / 100;
	if (GetR() != 0) height *= -1;
	
	CreateParticle("SmokeDirty", PV_Random(-width, width), PV_Random(0, height), PV_Random(-5, 5), PV_Random(-5, 15), PV_Random(10, 60), particles, 200);
	Sound("Hits::Materials::Rock::Rockfall*");

	for (var i = 0; i < 4; i++)
	{
		var fragment = CreateObject(Rock, 0, height / 2, NO_OWNER);
		fragment.Collectible = false;
		fragment->SetController(GetController());
		fragment->SetCon(50);
		fragment->SetSpeed(RandomX(-15, 15), RandomX(-25, 10));
		fragment->SetRDir(RandomX(-10, 10));
		fragment->FadeOut(390, true);
		fragment->SetClrModulation(this->GetClrModulation());
	}
	RemoveObject();
	return true;
}

public func Place(int amount, proplist rectangle, proplist settings)
{
	// Only allow definition call.
	if (GetType(this) != C4V_Def)
	{
		FatalError("Stalactite::Place must be called as a definition call!");
		return;
	}
	// Default parameters.
	if (!settings) 
		settings = {};
	var loc_area = nil;
	if (rectangle)
		loc_area = Loc_InRect(rectangle);
	var loc_background;
	if (settings.underground == nil)
		loc_background = Loc_Or(Loc_Sky(), Loc_Tunnel());
	else if (settings.underground)
		loc_background = Loc_Tunnel();
	else
		loc_background = Loc_Sky();

	var stalactites = [];
	for (var i = 0; i < amount; i++)
	{
		var loc = FindLocation(loc_background, Loc_Not(Loc_Liquid()), Loc_Wall(CNAT_Top), Loc_Space(40, CNAT_Bottom), loc_area);
		if (!loc)
			continue;
		var mat = MaterialName(GetMaterial(loc.x, loc.y - 1));
		var stalactite = CreateStalactite(loc.x, loc.y - 2, mat);
		
		// Find the ground below and scale down in narrow tunnels.
		var ground_y = nil;
		for (var y = 10; y < 200; y += 10)
		{
			if (!GBackSolid(loc.x, loc.y + y)) continue;
			
			// Search up and find actual surface.
			var up = 0;
			for (; up > -10; up -= 2)
				if (!GBackSolid(loc.x, loc.y + y + up)) break;
			
			ground_y = loc.y + y + up;
			break;
		}
		
		var con = 100;
		// Adjust size if there already is a stalactite very close.
		if (ObjectCount(Find_AtPoint(loc.x, loc.y), Find_ID(this)) > 1)
			con = RandomX(20, 50);
			
		var height;
		if (ground_y)
		{
			height = ground_y - loc.y;
			con = Min(con, BoundBy(100 * height / 120, 25, 100));
			
		}
		
		stalactite->SetCon(con, nil, true);
		
		// Create a stalagmite below?
		if (ground_y && height > 70 && Random(3))
		{
			// And place!
			if (MaterialName(GetMaterial(loc.x, ground_y + 2)) == mat)
			{
				var stalagmite = CreateStalactite(loc.x, ground_y + 2, mat, true);
				stalagmite->SetCon(con);
			}
		}
	}
	return stalactites;
}

private func CreateStalactite(int x, int y, string mat, bool stalagmite)
{
	var stalactite = CreateObject(this, x, y);

	// Ice stalactites are transparent and never use water sources.
	if (mat == "Ice")
	{
		stalactite->SetClrModulation(RGBa(157, 202, 243, 160));
	}
	else
	{
		// Keep colour tone of the material, but increase lightness.
		var colour = GetAverageTextureColor(mat);
		if (colour != nil)
		{
			colour = RGB2HSL(colour);
			var hue = (colour >> 16) & 0xff;
			colour = HSL(hue, 100, 200);
			stalactite->SetClrModulation(colour);
		}
	}

	if (stalagmite)
	{
		stalactite->SetR(180);
		stalactite.MeshTransformation = Trans_Mul(Trans_Translate(0, 10 * stalactite->GetDefHeight() * stalactite->GetCon(), 0), stalactite.MeshTransformation);
	}
	else if (mat != "Ice")
	{
		// Add rain drop effect for stalactites only.
		stalactite->AddRainDropEffect(nil, RandomX(80, 120), "Water", RandomX(1, 5), 0, 4);
	}
	
	return stalactite;
}

public func OnRainDropCreated(effect fx_drop)
{
	if (Random(9))
		return;
	Sound("Liquids::Waterdrop*");
}

local Name = "$Name$";
local Description = "$Description$";
