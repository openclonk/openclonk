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

/** Reproduction of plants: Called every 2 seconds by a timer.
*/
public func Seed()
{
	// Find number of plants in seed area.
	var size = SeedArea();
	var amount = SeedAmount();
	var area = Rectangle(AbsX(size / -2), AbsY(size / -2), size, size);
	if (this.Confinement)
		area = RectangleEnsureWithin(area, this.Confinement);
	var plant_cnt = ObjectCount(Find_ID(GetID()), Find_InRect(area.x - GetX(), area.y - GetY(), area.w, area.h));
	// If there are not much plants in the seed area compared to seed amount
	// the chance of seeding is improved, if there are much the chance is reduced.
	var chance = SeedChance();
	var chance = chance / Max(1, amount - plant_cnt) + chance * Max(0, plant_cnt - amount);
	// Place a plant if we are lucky, in principle there can be more than seed amount.
	if (!Random(chance))
	{
		// Place the plant but check if it is not close to another one.	
		var plant = PlaceVegetation(GetID(), area.x - GetX(), area.y - GetY(), area.w, area.h, 3);
		if (plant)
		{
			var neighbour = FindObject(Find_ID(GetID()), Find_Exclude(plant), Sort_Distance(plant->GetX() - GetX(), plant->GetY() - GetY()));
			var distance = ObjectDistance(plant, neighbour);
			// Closeness determined by seedarea and amount.
			if (!Random(distance / (size/amount)))
				plant->RemoveObject();
			else if (this.Confinement)
				plant->KeepArea(this.Confinement);
		}
	}
	return;
}

/* Chopping */

/** Determines whether this plant gives wood (we assume that are 'trees').
	@return \c true if the plant is choppable by the axe, \c false otherwise (default).
*/
public func IsTree()
{
	return false;
}

/** Determines whether the tree can still be chopped down (i.e. has not been chopped down).
	@return \c true if the tree is still a valid axe target.
*/
public func IsStanding()
{
	return GetCategory() & C4D_StaticBack;
}

/** Maximum damage the tree can take before it falls. Each blow from the axe deals 10 damage.
	@return \c the maximum amount of damage.
*/
private func MaxDamage()
{
	return 50;
}

protected func Damage()
{
	// do not grow for a few seconds
	var g = GetGrowthValue();
	if(g)
	{
		StopGrowth();
		ScheduleCall(this, "RestartGrowth", 36 * 10, 0, g);
	}
	
	// Max damage reached -> fall down
	if (GetDamage() > MaxDamage() && IsStanding()) ChopDown();
	_inherited(...);
}

// restarts the growing of the tree (for example after taking damage)
func RestartGrowth(int old_value)
{
	var g = GetGrowthValue(); // safety
	if(g) StopGrowth();
	g = Max(g, old_value);
	StartGrowth(g);
}

/** Called when the trees shall fall down (has taken max damage). Default behaviour is unstucking (5 pixel movement max) and removing C4D_StaticBack.
*/
public func ChopDown()
{
	// stop growing!
	ClearScheduleCall(this, "RestartGrowth");
	StopGrowth();
	this.Touchable = 1;
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
	SetRDir(10);
	if (Random(2)) SetRDir(-10);
	// Crack!
	if (GetCon() > 50) Sound("TreeDown?");
}

/* Harvesting */

/** Determines whether a plant may be harvested by the player.
	@return \c true if the plant can be harvested, \c false otherwise (default).
*/
private func IsCrop()
{
	return false;
}

/** Determines whether the plant is harvestable right now (i.e. is fully grown).
	@return \c true if the plant is ready to be harvested.
*/
public func IsHarvestable()
{
	return GetCon() >= 100;
}

public func IsInteractable(object clonk)
{
	return clonk->IsWalking() && IsCrop() && IsHarvestable() || _inherited(clonk);
}

public func GetInteractionMetaInfo(object clonk)
{
	if (IsCrop())
	{
		if (IsHarvestable())
			return { Description = Format("$Harvest$", GetName()) };
		else
			return _inherited(clonk);
	}
	return _inherited(clonk);
}

public func Interact(object clonk)
{
	if (IsCrop())
	{
		if (IsHarvestable())
			return Harvest(clonk);
		else
			return _inherited();
	}
	return _inherited(clonk); 
}

/** Called when the plant is harvested
	@param The harvesting clonk
	@return \c true is successfully harvested
*/
public func Harvest(object clonk)
{
	Split2Components();
	return true;
}