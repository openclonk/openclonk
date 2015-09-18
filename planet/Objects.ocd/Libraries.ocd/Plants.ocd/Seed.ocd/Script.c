/**
	Seed

	Logic for plant seed that will eventually sprout a plant if left alone long enough.
	Seeds may also be manually planted by using leftclick.

	@author Clonkonaut
*/

// The last known position of the seed
local lib_seed_pos;
// The timer cycles the seed has been lying idly (until it has reached 10)
local lib_seed_idletime = 0;
// The time it takes until a new plant might be created, feel free to reassign
local lib_seed_planttime = 10500;

// ****** Must be the id of the plant to create. Do not leave empty.
local lib_seed_plant;

/* Reproduction control */

/** Distance the seeds checks for plants. Default is 250.
	@return the maximum distance.
*/
private func SeedArea()
{
	return 250;
}
/** The amount of plants allowed within SeedArea. Default is 10.
	@return the maximum amount of plants.
*/
private func SeedAmount()
{
	return 10;
}
/** The closest distance a new plant may seed to its nearest neighbour. Default is 20.
	@return the maximum amount of plants.
*/
private func SeedOffset()
{
	return 20;
}

/* Reproduction */

private func Initialize()
{
	AddTimer("CheckSprout", lib_seed_planttime / 10);
	lib_seed_pos =  { x = GetX(), y = GetY() };
}

// Check whether a plant should be created
private func CheckSprout()
{
	if (Contained()) return;
	if (OnFire()) return;
	if (!CheckPosition()) return;

	lib_seed_idletime++;
	if (lib_seed_idletime >= 10)
	{
		// Is this place nice to make a new plant?
		if (!CheckPlantConditions())
			lib_seed_idletime = 0;
		else
			Sprout();
	}
}

// Check whether the seed was moved
private func CheckPosition()
{
	if (GetX() != lib_seed_pos.x || GetY() != lib_seed_pos.y)
	{
		lib_seed_pos.x = GetX();
		lib_seed_pos.y = GetY();
		lib_seed_idletime = 0;
		return false;
	}
	return true;
}

/* Sprouting */

// Returns the relative y to the ground from center to max 20 pixels.
// If no ground is found returns -1, also if the center is underground (stuck in material)
private func GetGroundPos()
{
	if (GBackSolid()) return -1;

	var y = 0;
	while (!GBackSolid(0,y) && y < 21)
		y++;
	if (y >= 21) return -1;
	return y;
}

/** Overload as necessary to check for further conditions. Return true if a new plant should be created.
*/
private func CheckPlantConditions()
{
	var ground = GetGroundPos();
	// No solid ground
	if (ground == -1) return false;
	// No fertile ground
	if (GetMaterialVal("Soil", "Material", GetMaterial(0,ground)) != 1) return false;
	// Do not grow underground
	if (!GBackSky(0, ground-1) && !GBackSky(0,0) && !GBackSky(0,-30)) return false;
	// Too many plants around
	var size = SeedArea();
	if (ObjectCount(Find_ID(lib_seed_plant), Find_InRect(-size / 2, -size / 2, size, size)) > SeedAmount()) return false;
	// Another plant too close
	var neighbour = FindObject(Find_ID(lib_seed_plant), Sort_Distance());
	if (neighbour)
		if (ObjectDistance(neighbour) < SeedOffset())
			return false;

	return true;
}

/** Overload as necessary. Should remove the seed.
*/
private func Sprout()
{
	var ground = GetGroundPos();
	var plant = CreateObjectAbove(lib_seed_plant, 0, ground, GetOwner());
	if (!plant) return; // What now?
	plant->SetCon(1);
	RemoveObject();
}

/* Planting */

public func ControlUse(object clonk, int x, int y, bool box)
{
	if(!clonk->~IsWalking()) return true;
	var ground = GetGroundPos();
	if (ground == -1)
		return CustomMessage(Format("$TxtBadGround$", GetName()), clonk, clonk->GetController(), 0, 0, 0xff0000);

	// Soil is needed
	if (GetMaterialVal("Soil", "Material", GetMaterial(0,ground)) != 1)
		return CustomMessage(Format("$TxtBadGround$", GetName()), clonk, clonk->GetController(), 0, 0, 0xff0000);

	// Plant!
	clonk->DoKneel();
	Sprout();
	return true;
}

local UsageHelp = "$UsageHelp$";