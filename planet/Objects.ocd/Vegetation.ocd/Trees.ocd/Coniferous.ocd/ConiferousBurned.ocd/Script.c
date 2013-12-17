/*-- Coniferous Tree --*/

// Overloaded from the plant library to add the foreground parameter, foreground = true will roughly make every 3rd tree foreground (not the offspring though)
func Place(int amount, proplist rectangle, proplist settings, bool foreground)
{
	// Plant
	// No calls to objects, only definitions
	if (GetType(this) == C4V_C4Object) return;
	// Default parameters
	if (!settings) settings = { growth = 100000, keep_area = false };
	if (!settings.growth) settings.growth = 100000;
	if (!rectangle)
		rectangle = Rectangle(0,0, LandscapeWidth(), LandscapeHeight());

	var plants = CreateArray(), plant;
	for (var i = 0 ; i < amount ; i++)
	{
		plant = PlaceVegetation(this, rectangle.x, rectangle.y, rectangle.w, rectangle.h, settings.growth);
		if (plant)
		{
			plants[GetLength(plants)] = plant;
			if (settings.keep_area)
				plant->~KeepArea(rectangle);
		}
	}
	
	// Coniferous
	// Default behaviour
	var trees = plants;
	if (GetLength(trees) < 1) return trees;

	for (var tree in trees)
		if (!Random(3))
			tree.Plane = 510;
	return trees;
}

public func IsPlant() { return true; }
public func IsTree() { return true; }

func Construction()
{
	var scale = 1100;
	var yoff = (110 * scale / 1000) - 110;
	SetObjDrawTransform(scale, 0, 0, 0, scale, -yoff * 1000);
}

public func IsStanding()
{
	return GetCategory() & C4D_StaticBack;
}

private func MaxDamage()
{
	return 50;
}

protected func Damage()
{
	// Max damage reached -> fall down
	if (GetDamage() > MaxDamage() && IsStanding()) ChopDown();
	if(!IsStanding() && OnFire() && GetDamage() > MaxDamage() * 2)
		BurstIntoAshes();
	_inherited(...);
}


public func ChopDown()
{
	// Use Special Vertex Mode 1 (see documentation) so the removed vertex won't come back when rotating the tree.
	SetVertex(0, VTX_Y, 0, 1);
	// Remove the bottom vertex
	RemoveVertex(0);
	
	this.Touchable = 1;
	this.Plane = 300;
	SetCategory(GetCategory()&~C4D_StaticBack);
	if (Stuck())
	{
		var i = 5;
		while(Stuck() && i)
		{
			SetPosition(GetX(), GetY()-1);
			i--;
		}
	}
	Sound("TreeCrack");
	AddEffect("TreeFall", this, 1, 1, nil, Library_Plant);
}

func BurstIntoAshes()
{
	var particles =
	{
		Prototype = Particles_Dust(),
		R = 50, G = 50, B = 50,
		Size = PV_KeyFrames(0, 0, 0, 200, PV_Random(2, 10), 1000, 0),
	};
	
	var r = GetR();
	var size = GetCon() * 110 / 100;
	
	for(var cnt = 0; cnt < 10; ++cnt)
	{
		var distance = Random(size/2);
		var x = Sin(r, distance);
		var y = -Cos(r, distance);
		
		for(var mirror = -1; mirror <= 1; mirror += 2)
		{
			CreateParticle("Dust", x * mirror, y * mirror, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 1 * 36), particles, 2);
			CastPXS("Ashes", 5, 30, x * mirror, y * mirror);
		}
	}
	RemoveObject();
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 1;
local ContactIncinerate = 3;