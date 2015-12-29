/**
	Rock Fragment
	Falls down and destroys objects on its path.

	@author Pyrit
*/

protected func Construction()
{
	SetGraphics(Format("%d", RandomX(1, 3)));
	SetRDir(RandomX(-20, 20));
}

protected func Hit(x, y)
{
	if (GetCon() <= 30)
	{
		RemoveObject();
		return;
	}
	
	// Do damage to bridges, according to the size of the rock.
	var bridge = FindObject(Find_AtPoint(0, 10), Find_Func("IsBridge"));
	if (bridge)
		bridge->DoDamage(10 * GetCon() / 16);
	
	// Dust and effects.
	StonyObjectHit(x, y);

	//Ignore SolidMask materials...
	if (GetMaterial(0, GetBottomEdge() - GetY() + 2) != Material("Vehicle"))
	{
		var clr = GetAverageTextureColor(GetTexture(0, GetBottomEdge() - GetY() + 2));
		var particles =
		{
			Prototype = Particles_Dust(),
			R = (clr >> 16) & 0xff,
			G = (clr >> 8) & 0xff,
			B = clr & 0xff,
			Size = PV_KeyFrames(0, 0, 0, 150, 20, 500, 15),
		};
		CreateParticle("Dust", 0, 8, PV_Random(-3, 3), PV_Random(-3, 3), PV_Random(18, 1 * 36), particles, 3);
	}
	
	// Leave little craters in materials.
	BlastFree(GetX(), GetY(), GetCon() / 12);
	
	// Break into smaller pieces.
	for (var i = 0; i < 2; i++)
	{
		var small = CreateObject(RockFragment);
		small->SetVelocity(RandomX(-50, 50), 40);
		small->SetCon(10 * GetCon() / 15);
	}
	RemoveObject();
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";