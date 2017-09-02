/**
	Seed

	Logic for plant seed that will eventually sprout a plant if left alone long enough.
	Seeds may also be manually planted by using leftclick.

	@author Clonkonaut
*/

local Description = "$Description$";

// The last known position of the seed
local lib_seed_pos;
// The timer cycles the seed has been lying idly (until it has reached 10)
local lib_seed_idletime = 0;
// The time it takes until a new plant might be created, feel free to reassign
local lib_seed_planttime = 10500;
// Maximum lifetime until this seeds becomes un-collectible and destroys itself (not running if contained)
local lib_seed_lifetime = 0; // If zero, seed never decays
// Creation frame
local lib_seed_creationtime;

// ****** Must be the id of the plant to create. Do not leave empty.
local lib_seed_plant;

// Can be planted manually?
local lib_seed_can_plant_manually = true;

/* Reproduction control */

/** Chance of seeded plant to reproduce again (does not affect the seed itself)
	@return the chance, higher = more chance. 0 = does not reproduce.
*/
local plant_seed_chance = 20;

public func SeedChance()
{
	return plant_seed_chance;
}

public func SetSeedChance(int v)
{
	plant_seed_chance = v;
	return true;
}


/** Distance the seeds may travel. Default is 250.
	@return the maximum distance.
*/

local plant_seed_area = 250;

public func SeedArea()
{
	return plant_seed_area;
}

public func SetSeedArea(int v)
{
	plant_seed_area = v;
	return true;
}


/** The amount of plants allowed within SeedAreaSize. Default is 10.
	@return the maximum amount of plants.
*/

local plant_seed_amount = 10;

private func SeedAmount()
{
	return plant_seed_amount;
}

public func SetSeedAmount(int v)
{
	plant_seed_amount = v;
	return true;
}


/** The closest distance a new plant may seed to its nearest neighbour. Default is 20.
	@return the maximum amount of plants.
*/

local plant_seed_offset = 20;

public func SeedOffset()
{
	return plant_seed_offset;
}

public func SetSeedOffset(int v)
{
	plant_seed_offset = v;
	return true;
}


/* Confinement: Restrict area in which the plant can grow and re-seed */

local Confinement;

public func SetConfinement(proplist v)
{
	Confinement = v;
	return true;
}


/* Reproduction */

private func Initialize()
{
	AddTimer("CheckSprout", lib_seed_planttime / 10);
	lib_seed_pos =  { x = GetX(), y = GetY() };
	lib_seed_creationtime = FrameCounter();
}

// Check whether a plant should be created
private func CheckSprout()
{
	if (Contained()) return;
	if (OnFire()) return;
	if (lib_seed_lifetime && FrameCounter() - lib_seed_creationtime >= lib_seed_lifetime)
	{
		// Took too long. Decay.
		this.Collectible = 0;
		this->DoCon(-5);
		return;
	}
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
	// or underwater
	if (GBackLiquid(0,-15)) return false;
	// Too many plants around
	var size = SeedArea();
	if (ObjectCount(Find_ID(lib_seed_plant), Find_InRect(-size / 2, -size / 2, size, size)) > SeedAmount()) return false;
	// Another plant too close
	var max_offset = SeedOffset();
	if (max_offset)
	{
		var neighbour = FindObject(Find_ID(lib_seed_plant), Find_Distance(max_offset, 0, -lib_seed_plant->GetDefHeight()/2));
		if (neighbour)
			return false;
	}
	// Check confinement
	if (this.Confinement && !this.Confinement->IsPointContained(GetX(), GetY()))
		return false;
	// Alright, can plant here!
	return true;
}


/** Overload as necessary. Should remove the seed.
*/
private func Sprout()
{
	var ground = GetGroundPos();
	var plant = CreateObjectAbove(lib_seed_plant, 0, ground, GetOwner());
	if (plant)
	{
		plant->SetCon(1);
		// Copy confinement, seed area, etc. from seed
		plant->InitChild(this);
	}
	// Always remove, even if planting failed. Otherwise we may accumulate crazy numbers of seeds.
	return RemoveObject();
}


/* Planting */


// When the clonk is able to use the item
public func RejectUse(object clonk)
{
	return _inherited(clonk) || (lib_seed_can_plant_manually && !clonk->IsWalking());
}

public func ControlUse(object clonk, int x, int y, bool box)
{
	var used = _inherited(clonk, x, y, box, ...);
	if (!used && lib_seed_can_plant_manually)
	{
		return PlantManually(clonk);
	}
	return used;
}


private func PlantManually(object clonk)
{
	var ground = GetGroundPos();
	if (ground == -1)
	{
		return CustomMessage(Format("$TxtBadGround$", GetName()), clonk, clonk->GetController(), 0, 0, 0xff0000);
	}

	// Soil is needed
	if (GetMaterialVal("Soil", "Material", GetMaterial(0,ground)) != 1)
	{
		return CustomMessage(Format("$TxtBadGround$", GetName()), clonk, clonk->GetController(), 0, 0, 0xff0000);
	}

	// Plant!
	clonk->DoKneel();
	Sprout();
	return true;
}


/* Editor */

private func GetDefaultConfinement(plant, def_val)
{
	// Default confinement is seed size
	var x, y, size;
	if (plant)
	{
		x = plant->GetX();
		y = plant->GetY();
		size = plant->SeedArea();
	}
	else
	{
		// Doesn't matter.
		// This point is only reached if the plant got deleted between value change in editor and execution in network
		x=LandscapeWidth()/2;
		y=LandscapeHeight()/2;
		size=200;
	}
	return Shape->Rectangle(x - size / 2, y - size / 2, size, size);
}

private func SetConfinementRect(to_rect)
{
	// Editor sets properties only; convert to rectangle.
	if (to_rect) to_rect = Shape->Rectangle(to_rect.x, to_rect.y, to_rect.wdt, to_rect.hgt);
	this.Confinement = to_rect;
	return true;
}

public func AddSeedEditorProps(def)
{
	// Seed props used by seed and plant
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.plant_seed_chance = { Name="$SeedChance$", EditorHelp="$SeedChanceHelp$", Type="int", Min=0, Max=10000, Asyncget="SeedChance", Set="SetSeedChance", Save="Seed" };
	def.EditorProps.plant_seed_area = { Name="$SeedArea$", EditorHelp="$SeedAreaHelp$", Type="int", Min=0, Asyncget="SeedArea", Set="SetSeedArea", Save="Seed" };
	def.EditorProps.plant_seed_amount = { Name="$SeedAmount$", EditorHelp="$SeedAmountHelp$", Type="int", Min=0, Asyncget="SeedAmount", Set="SetSeedAmount", Save="Seed" };
	def.EditorProps.plant_seed_offset = { Name="$SeedOffset$", EditorHelp="$SeedOffsetHelp$", Type="int", Min=0, Asyncget="SeedOffset", Set="SetSeedOffset", Save="Seed" };
	def.EditorProps.Confinement = { Name="$Confinement$", EditorHelp="$ConfinementHelp$", Type="enum", Set="SetConfinementRect", Save="Seed", Options = [
		{ Name="$NoConfinmenet$" },
		{ Name="$Rect$", OptionKey="Type", DefaultValueFunction=Library_Seed.GetDefaultConfinement, Value={ Type="rect" }, Delegate={ Type="rect", Relative=false, Storage="proplist", Color=0x30ff30, Set="SetConfinementRect" } }
		// other shapes not supported for now
		] };
	def.SetConfinementRect = Library_Seed.SetConfinementRect;
	return true;
}

public func Definition(def, ...)
{
	AddSeedEditorProps(def);
	return _inherited(def, ...);
}
