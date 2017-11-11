/*-- Large Cave Mushroom --*/

#include Library_Plant
#include Library_Tree

/*-- Placement --*/

public func Place(int amount, proplist area, proplist settings)
{
	if (settings == nil) settings = {};
	// Default behaviour
	var plants = [];
	var terraform = settings.terraform ?? true;
	
	var max_tries = 2 * amount;
	var loc_area = nil;
	if (area) loc_area = Loc_InArea(area);
	
	// look for free underground spot to place groups of three or so..
	while ((amount > 0) && (--max_tries > 0))
	{
		var spot = FindLocation(Loc_Tunnel(), Loc_Wall(CNAT_Bottom, Loc_MaterialVal("Soil", "Material", nil, 1)), Loc_Space(20, CNAT_Top), Loc_Func(LargeCaveMushroom.GoodSpot), loc_area);
		// can't place more
		if (!spot && !terraform) return plants;
		
		if (!spot)
		{
			// note that the border is temporarily off until the bug with a turbulent-parent-overlay is fixed
			var cave = Landscape_Cave->Place(1, area, {width = 200 + Random(50), height = 100 + Random(50), borderheight = 0, bordermat = "Earth", bordertex = "earth" });
			if (GetLength(cave) == 0) return plants; // can't place more
			cave = cave[0];
			spot = { x = cave->GetX(), y = cave->GetY() };
		}
		
		// have center-spot now
		// search random spots in vicinity and place mushrooms
		var failsafe = 100;
		for (var i = RandomX(2, 4); (i >= 0) && (failsafe > 0); --failsafe)
		{
			var angle = RandomX(-100, 100);
			var r = RandomX(30, 50);
			var end_x = spot.x + Sin(angle, r);
			var end_y = spot.y - Cos(angle, r);
			var distance = 0;
			
			while (!GBackSolid(end_x, end_y) && distance < 100) {++distance; ++end_y;}
			
			if (GBackSemiSolid(end_x, end_y - 1)) continue;
			if (GetMaterial(end_x, end_y - 1) != Material("Tunnel")) continue;
			
			// enough space?
			if (!PathFree(end_x, end_y - 1, end_x, end_y - 15)) continue;
			
			var plant = CreateConstruction(this, end_x, end_y - 1, NO_OWNER, 20 + Random(10), false, false);
			if (!plant) continue;
			PushBack(plants, plant);
			--amount;
			--i;
		}
	}
	
	if (GetLength(plants) == 0) return plants;

	for (var plant in plants)
		if (!Random(3))
			plant.Plane = 510;
	return plants;
}

// for the placement
func GoodSpot(int x, int y)
{
	return ObjectCount(Find_Distance(100, x, y), Find_ID(LargeCaveMushroom)) < 5;
}

/* Plant / Tree libraries */

local plant_seed_chance = 50;
local plant_seed_area = 200;
local plant_seed_amount = 5;

private func Construction()
{
	AddTimer("Growing", 80);

	// every mushroom is unique!
	var len = GetAnimationLength("Rotate1"); // length of all animations is the same
	PlayAnimation("Pitch1", 1, Anim_Const(RandomX(0, len)), Anim_Const(500));
	PlayAnimation("Pitch2", 2, Anim_Const(RandomX(0, len)), Anim_Const(500));
	PlayAnimation("Rotate1", 2, Anim_Const(RandomX(0, len)), Anim_Const(500));

	PlayAnimation("Head1", 2, Anim_Const(RandomX(0, len)), Anim_Const(500));
	PlayAnimation("Head2", 2, Anim_Const(RandomX(0, len)), Anim_Const(500));
	PlayAnimation("Head3", 2, Anim_Const(RandomX(0, len)), Anim_Const(500));
	PlayAnimation("Head4", 2, Anim_Const(RandomX(0, len)), Anim_Const(500));

	var brightness = Random(25);
	SetClrModulation(RGB(200 + brightness + Random(30), 200 + brightness + Random(30), 200 + brightness + Random(30)));

	inherited(...);
	StopGrowth();
}

private func Growing()
{
	if(GetCon() >= Min(RandomX(80, 500), 100))
		return RemoveTimer("Growing");

	if(!Random(4)) return;

	var top = ((-36) * GetCon()) / 100;
	if(GBackSolid(0, Min(-5, top))) return;
	DoCon(1);
}

public func ChopDown()
{
	// Stop growing
	RemoveTimer("Growing");

	_inherited(...);
}

local burned;

private func Damage(int change, int cause)
{
	_inherited(change, cause);

	if (this && !burned && GetDamage() > MaxDamage()/3 && OnFire())
	{
		SetClrModulation(RGB(100, 100, 100));
		RemoveTimer("Growing");
		burned = true;
	}
}

private func MaxDamage()
{
	return 150;
}

// called from the plant library
private func Seed()
{
	if(GetCon() < 20) return;

	var size = SeedArea();
	// Place a plant if we are lucky
	if (CheckSeedChance())
	{
		// select random angle at which we can throw the seed
		var angle = RandomX(100, 140);
		if(!Random(2)) angle = 360 - angle;
		
		var okay = false, x, y;
		for (var i = 0; i < 3; ++i) // three is the magic number of tries
		{
			var point = PathFree2(GetX(), GetY() - (25 * GetCon()) / 100, GetX() + Sin(angle, size), GetY() - Cos(angle, size));
			if (!point) continue;
			x = point[0];
			y = point[1];
			
			if (Distance(GetX(), GetY(), x, y) < size/4) continue;
			x -= GetX();
			y -= GetY();
			if (!GBackSolid(x, y + 2)) continue;
			if (GetMaterial(x, y - 2) != Material("Tunnel")) continue;
			
			var mat = GetMaterial(x, y + 2);
			if (GetMaterialVal("Soil", "Material", mat) != 1) continue;
			var neighbour = FindObjects(Find_Distance(size, x,  y), Find_ID(GetID()), Find_Exclude(this), Sort_Distance(x, y));
			if (GetLength(neighbour) > 0)
			{
				var distance = Distance(GetX() + x, GetY() + y, neighbour[0]->GetX(), neighbour[0]->GetY());
				var len = GetLength(neighbour);
				var amount = SeedAmount();
				var c = (size * distance * amount) / (len * len * len);
				if (!Random(c))
					continue;
			}
			
			if (this.Confinement)
			{
				if (!this.Confinement->IsPointContained(x, y)) continue;
			}

			okay = true;
			break;
		}
		if (!okay) return;
		// Place the plant but check if it is not close to another one.	
		var plant = CreateConstruction(GetID(), x, y, GetOwner(), 3, false, false);

		if (plant)
		{
			if (this.Confinement)
				plant->KeepArea(this.Confinement);
		}
	}
	return plant;
}

local Name = "$Name$";
local Touchable = 0;
local BlastIncinerate = 2;
local ContactIncinerate = 6;
local NoBurnDecay = true;
local Components = {Wood = 4};