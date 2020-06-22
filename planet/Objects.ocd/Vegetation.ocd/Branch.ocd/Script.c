/** 
	Branch 
	A single branch from a tree which can be placed attached to ceilings or walls.
	
	@author Maikel, Randrian
*/

protected func Initialize()
{
	this.MeshTransformation = Trans_Mul(Trans_Scale(1000, 1400, 1000), Trans_Translate(0, 3500, 0), Trans_Rotate(RandomX(0, 359), 0, 1, 0));
	SetR(RandomX(-30, 30));
	return;
}

protected func Damage()
{	
	if (GetDamage() > 15)
	{
		CastObjects(Wood, 1, 25);
		RemoveObject();
	}
	return;
}


/*-- Placement --*/

// Place an amount of branches in the specified area. Settings:
// size = [min, max]: Random size (con) between min and max.
// underground = true/false: whether to place only underground.
public func Place(int amount, proplist area, proplist settings)
{
	// Only allow definition call.
	if (this != Branch) 
		return;
	// Default parameters.
	if (!settings) 
		settings = { size = [80, 120] };
	if (!settings.size) 
		settings.size = [80, 120];
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
	var loc_inmat = Loc_Or(Loc_Material("Granite"), Loc_Material("Rock"), Loc_MaterialVal("Soil", "Material", nil, 1));
	if (settings.in_mat)
		loc_inmat = Loc_Material(settings.in_mat);
	var branches = [];	
	for (var i = 0; i < amount; i++)
	{
		var size = RandomX(settings.size[0], settings.size[1]);
		var loc = FindLocation(loc_background, Loc_Not(Loc_Liquid()), Loc_Wall(CNAT_Left | CNAT_Right | CNAT_Top, loc_inmat), loc_area);
		if (!loc)
			continue;
		var branch = CreateObject(Branch);
		branch->SetPosition(loc.x, loc.y);
		branch->SetCon(size);
		if (!Random(3))
			branch.Plane = 510; 
		// Adjust orientation and position with respect to landscape.
		branch->AdjustOrientation();
		branch->AdjustPosition();
		// Retry if the center is add a solid location.
		if (branch->GBackSolid())
		{
			branch->RemoveObject();
			i--;
			continue;		
		}		
		PushBack(branches, branch);	
	}
	return branches;
}

// Adjust the orientation of the branch with respect to material.
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
	for (var d = 0; d < 15 * GetCon() / 100; d++)
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
	var size = 12 * GetCon() / 100;
	SetPosition(GetX() - Sin(angle, d - size), GetY() + Cos(angle, d - size));
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Placement = 4;
