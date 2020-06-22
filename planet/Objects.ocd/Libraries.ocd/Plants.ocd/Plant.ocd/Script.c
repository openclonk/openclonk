/**
	Plant
	Basic functionality for all plants

	@author Clonkonaut
*/

// This is a plant
public func IsPlant()
{
	return true;
}

/** Automated positioning via RootSurface, make sure to call this if needed (in case Construction is overloaded)
*/
protected func Construction(...)
{
	Schedule(this, "RootSurface()", 1);
	UpdateSeedTimer();
	AddTimer("Seed", 72 + Random(10));
	_inherited(...);
}

public func InitChild(object parent)
{
	// Copy settings from parent plant
	KeepArea(parent.Confinement);
	SetSeedChance(parent->SeedChance());
	SetSeedArea(parent->SeedArea());
	SetSeedAmount(parent->SeedAmount());
	SetSeedOffset(parent->SeedOffset());
	return true;
}

/* Placement */

/** Places the given amount of plants inside the area. If no area is given, the whole landscape is used.
	@param amount The amount of plants to be created (not necessarily every plant is created).
	@param rectangle The area where to put the plants.
	@param settings A proplist defining further setttings: { growth = 100000, keep_area = false }. Growth will get passed over to PlaceVegetation, keep_area will confine the plants and their offspring to rectangle.
	@return Returns an array of all objects created.
*/
public func Place(int amount, proplist area, proplist settings)
{
	// No calls to objects, only definitions
	if (GetType(this) == C4V_C4Object) return;
	// Default parameters
	if (!settings) settings = { growth = 100000, keep_area = false };
	if (!settings.growth) settings.growth = 100000;
	var rectangle;
	if (area) rectangle = area->GetBoundingRectangle(); else rectangle = Shape->LandscapeRectangle();

	var plants = CreateArray(), plant;
	for (var i = 0 ; i < amount ; i++)
	{
		plant = PlaceVegetation(this, rectangle.x, rectangle.y, rectangle.wdt, rectangle.hgt, settings.growth, area);
		if (plant)
		{
			plants[GetLength(plants)] = plant;
			if (settings.keep_area && area)
				plant->KeepArea(area);
		}
		plant = nil;
	}
	return plants;
}

/* Reproduction */

/** Will confine the the plant and its offspring to a certain area.
	@params area The confinement area.
*/
func KeepArea(proplist area)
{
	this.Confinement = area;
}

/** Chance to reproduce plant. Chances are one out of return value. From 0 to 10000. Default is 20.
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
	return UpdateSeedTimer();
}

private func UpdateSeedTimer()
{
	RemoveTimer("Seed");
	if (plant_seed_chance) AddTimer("Seed", 72 + Random(10));
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
	@return the closest distance to another plant.
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

/** Evaluates parameters for this definition to determine if seeding should occur.
 @par offx X offset added to context position for check.
 @par offy Y offset added to context position for check.
 @plant_id plant to check for whether it's already too crowded. Default to GetID().
 @return true iff seeding should occur
*/
public func CheckSeedChance(int offx, int offy, id plant_id)
{
	// Find number of plants in seed area.
	// Ignored confinement - that's only used for actual placement
	var size = this->SeedArea();
	var amount = this->SeedAmount();
	var plant_cnt = ObjectCount(Find_ID(plant_id ?? GetID()), Find_InRect(offx - size / 2, offy - size / 2, size, size));
	// Increase seed chance by number of missing plants to reach maximum amount
	// Note the chance will become negative if the maximum has been reached, in which case the random check will never succeed.
	// That's intended
	var chance = this->SeedChance() * (amount - plant_cnt);
	if (!chance) return;
	// Place a plant if we are lucky
	return (Random(10000) < chance);
}

/** Reproduction of plants: Called every 2 seconds by a timer.
*/
private func Seed()
{
	if (OnFire()) return;

	// Place a plant if we are lucky, but no more than seed amount.
	var plant;
	if (CheckSeedChance())
	{
		plant = DoSeed(true);
		// Check if it is not close to another one.
		if (plant)
		{
			var neighbours = FindObjects(Find_Func("IsPlant"), Find_Exclude(plant),
			                           Sort_Multiple(Sort_Distance(plant->GetX() - GetX(), plant->GetY() - GetY()), Sort_Reverse(Sort_Func("SeedOffset"))));
			// Only check the nearest 3 plants
			var too_close = false;
			for (var i = 0; i < GetLength(neighbours) && i < 3; i++)
			{
				var neighbour = neighbours[i];
				var x_distance = plant->SeedOffset() + 1;
				var y_distance = 151;
				if (neighbour)
				{
					x_distance = Abs(neighbour->GetX() - plant->GetX());
					y_distance = Abs(neighbour->GetY() - plant->GetY());
				}
				if ((x_distance < plant->SeedOffset() || x_distance < neighbour->~SeedOffset()) && y_distance < 151)
				{
					too_close = true;
					break;
				}
			}
			// Closeness check
			if (too_close)
				plant->RemoveObject();
			else
				plant->InitChild(this);
		}
	}
	return plant;
}

/** Forcefully places a seed of the plant, without random chance
    or other sanity checks. This is useful for testing.
 */
public func DoSeed(bool no_init)
{
	// Apply confinement for plant placement
	var size = SeedArea();
	var area = Shape->Rectangle(GetX() - size / 2, GetY() - size / 2, size, size);
	var confined_area = nil;
	if (this.Confinement)
	{
		confined_area = Shape->Intersect(this.Confinement, area);
		// Quick-check if intersection to confinement yields an empty area
		// to avoid unnecessery search by PlaceVegetation
		area = confined_area->GetBoundingRectangle();
		if (area.wdt <= 0 || area.hgt <= 0) return;
	}
	else
	{
		// Place the new plant in the original area
		confined_area = area;
	}
	// Place the plant
	var plant = PlaceVegetation(GetID(), 0, 0, 0, 0, 3, confined_area);
	if (!no_init && plant)
		plant->InitChild(this);

	return plant;
}

private func RemoveInTunnel()
{
	if (GetMaterial() == Material("Tunnel") || GetMaterial(0, -10) == Material("Tunnel"))
	{
		RemoveObject();
	} 
}

/* Editor */

public func Definition(def, ...)
{
	Library_Seed->AddSeedEditorProps(def);
	return _inherited(def, ...);
}
