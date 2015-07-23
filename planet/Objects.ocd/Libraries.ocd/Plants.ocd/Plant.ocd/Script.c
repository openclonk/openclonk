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

// This is a plant and not a tree
public func IsTree()
{
	return false;
}

/** Automated positioning via RootSurface, make sure to call this if needed (in case Construction is overloaded)
*/
protected func Construction()
{
	Schedule(this, "RootSurface()", 1);
	AddTimer("Seed", 72);
	_inherited(...);
}

/* Placement */

/** Places the given amount of plants inside the area. If no area is given, the whole landscape is used.
	@param amount The amount of plants to be created (not necessarily every plant is created).
	@param rectangle The area where to put the plants.
	@param settings A proplist defining further setttings: { growth = 100000, keep_area = false }. Growth will get passed over to PlaceVegetation, keep_area will confine the plants and their offspring to rectangle.
	@return Returns an array of all objects created.
*/
public func Place(int amount, proplist rectangle, proplist settings)
{
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
				plant->KeepArea(rectangle);
		}
		plant = nil;
	}
	return plants;
}

/* Reproduction */

/** Will confine the the plant and its offspring to a certain area.
	@params rectangle The confinement area.
*/
func KeepArea(proplist rectangle)
{
	this.Confinement = rectangle;
}

/** Chance to reproduce plant. Chances are one out of return value. Default is 500.
	@return the chance, higher = less chance.
*/
private func SeedChance()
{
	return 500;
}

/** Distance the seeds may travel. Default is 250.
	@return the maximum distance.
*/
private func SeedArea()
{
	return 250;
}

/** The amount of plants allowed within SeedAreaSize. Default is 10.
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

/** Reproduction of plants: Called every 2 seconds by a timer.
*/
private func Seed()
{
	if (OnFire()) return;

	// Find number of plants in seed area.
	var size = SeedArea();
	var amount = SeedAmount();
	var area = Rectangle(size / -2, size / -2, size, size);
	if (this.Confinement)
		area = RectangleEnsureWithin(area, this.Confinement);
	var plant_cnt = ObjectCount(Find_ID(GetID()), Find_InRect(area.x, area.y, area.w, area.h));
	// If there are not much plants in the seed area compared to seed amount
	// the chance of seeding is improved, if there are much the chance is reduced.
	var chance = SeedChance();
	var chance = chance / Max(1, amount - plant_cnt) + chance * Max(0, plant_cnt - amount);
	// Place a plant if we are lucky, but no more than seed amount.
	var plant;
	if (plant_cnt < amount && !Random(chance))
	{
		// Place the plant but check if it is not close to another one.	
		plant = PlaceVegetation(GetID(), area.x, area.y, area.w, area.h, 3);
		if (plant)
		{
			var neighbour = FindObject(Find_ID(GetID()), Find_Exclude(plant), Sort_Distance(plant->GetX() - GetX(), plant->GetY() - GetY()));
			var distance = ObjectDistance(plant, neighbour);
			// Closeness check
			if (distance < SeedOffset())
				plant->RemoveObject();
			else if (this.Confinement)
				plant->KeepArea(this.Confinement);
		}
	}
	return plant;
}