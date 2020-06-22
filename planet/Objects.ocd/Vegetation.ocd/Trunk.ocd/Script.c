/** 
	Trunk 
	Dead trunk which was once part of a tree.
	
	@author Maikel, Randrian
*/


protected func Initialize()
{
	SetProperty("MeshTransformation", Trans_Rotate(RandomX(0, 359), 0, 1, 0));
	return;
}

public func IsPlant() { return true; }
public func IsTree() { return true; }

// Not supported by this tree
public func CreateObjectInTreetop() { return nil; }

public func IsStanding() { return GetCategory() & C4D_StaticBack; }

public func ChopDown()
{
	// Use Special Vertex Mode 1 (see documentation) so the removed vertex won't come back when rotating the tree.
	SetVertex(3, VTX_Y, 0, 1);
	// Remove the bottom vertex
	RemoveVertex(3);
	// Make pushable
	this.Touchable = 1;
	this.Plane = 300;
	SetCategory(GetCategory()&~C4D_StaticBack);
	if (Stuck())
	{
		var i = 5;
		while (Stuck() && i)
		{
			SetPosition(GetX(), GetY()-1);
			i--;
		}
	}
	// Effect
	Sound("Environment::Tree::Crack");
	AddEffect("TreeFall", this, 1, 1, nil, Library_Plant);
}

private func MaxDamage()
{
	return 50;
}

protected func Damage()
{
	// Max damage reached -> fall down
	if (GetDamage() > MaxDamage() && IsStanding()) ChopDown();
	if (!IsStanding() && OnFire() && GetDamage() > MaxDamage() * 2)
		BurstIntoAshes();
	_inherited(...);
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
	
	for (var cnt = 0; cnt < 10; ++cnt)
	{
		var distance = Random(size/2);
		var x = Sin(r, distance);
		var y = -Cos(r, distance);
		
		for (var mirror = -1; mirror <= 1; mirror += 2)
		{
			CreateParticle("Dust", x * mirror, y * mirror, PV_Random(-3, 3), PV_Random(-3, -3), PV_Random(18, 1 * 36), particles, 2);
			CastPXS("Ashes", 5, 30, x * mirror, y * mirror);
		}
	}
	RemoveObject();
}


/*-- Placement --*/

// Place an amount of trunks in the specified rectangle. Settings:
// size = [min, max]: Random size (con) between min and max.
// underground = true/false: whether to place only underground.
public func Place(int amount, proplist area, proplist settings)
{
	// Only allow definition call.
	if (this != Trunk) 
		return;
	// Default parameters.
	if (!settings) 
		settings = { size = [80, 100] };
	if (!settings.size) 
		settings.size = [80, 100];
	var loc_area = nil;
	if (area) 
		loc_area = Loc_InArea(area);
	var loc_background;
	if (settings.underground == nil)
		loc_background = Loc_Or(Loc_Sky(), Loc_Tunnel());
	else if (settings.underground)
		loc_background = Loc_Tunnel();
	else
		loc_background = Loc_Sky();	
		
	var trunks = [];	
	for (var i = 0; i < amount; i++)
	{
		var size = RandomX(settings.size[0], settings.size[1]);
		var loc = FindLocation(loc_background, Loc_Not(Loc_Liquid()), Loc_Wall(CNAT_Left | CNAT_Right | CNAT_Top, Loc_Or(Loc_Material("Granite"), Loc_Material("Rock"), Loc_MaterialVal("Soil", "Material", nil, 1))), loc_area);
		if (!loc)
			continue;
		var trunk = CreateObject(Trunk);
		trunk->SetPosition(loc.x, loc.y);
		trunk->SetCon(size);
		if (!Random(3))
			trunk.Plane = 510; 
		// Adjust orientation and position with respect to landscape.
		trunk->AdjustOrientation();
		trunk->AdjustPosition();
		// Retry if the center is add a solid location.
		if (trunk->GBackSolid())
		{
			trunk->RemoveObject();
			i--;
			continue;		
		}		
		PushBack(trunks, trunk);	
	}
	return trunks;
}

// Adjust the orientation of the trunk with respect to material.
public func AdjustOrientation()
{
	// Make sure it is not stuck in solid with its center.
	var dx = 0;
	if (GBackSolid(-2, 0) && !GBackSolid(2, 0))
		dx = 3;
	if (GBackSolid(2, 0) && !GBackSolid(-2, 0))	
		dx = -3;
	var dy = 0;
	if (GBackSolid(0, -2) && !GBackSolid(0, 2))
		dy = 3;
	if (GBackSolid(0, 2) && !GBackSolid(0, -2))	
		dy = -3;
	SetPosition(GetX() + dx, GetY() + dy);
	
	// Check for increasing radius.
	for (var d = 0; d < 30 * GetCon() / 100; d++)
		// Check 8 directions.
		for (var angle = 0; angle < 360; angle += 45)
			if (GBackSolid(-Sin(angle, d), Cos(angle, d)))
				return SetR(RandomX(angle - 10, angle + 10));
	return;
}

// Adjust position with respect to material.
public func AdjustPosition()
{
	var angle = GetR();
	// Find distance to material.
	var d = 0;
	while (!GBackSolid(-Sin(angle, d), Cos(angle, d)) && d < 24 * GetCon() / 100)
		d++;
	// Adjust position.
	var size = 15 * GetCon() / 100;
	SetPosition(GetX() - Sin(angle, d - size), GetY() + Cos(angle, d - size));
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Placement = 4;
local Components = {Wood = 3};
