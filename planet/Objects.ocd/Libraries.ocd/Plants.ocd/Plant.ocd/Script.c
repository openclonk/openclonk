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

/** The closest distance a new plant may seed to its nearest neighbour. Default is 20.
	@return the maximum amount of plants.
*/
private func SeedOffset()
{
	return 20;
}

/** Reproduction of plants: Called every 2 seconds by a timer.
*/
public func Seed()
{
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
	if (plant_cnt < amount && !Random(chance))
	{
		// Place the plant but check if it is not close to another one.	
		var plant = PlaceVegetation(GetID(), area.x, area.y, area.w, area.h, 3);
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
	AddEffect("TreeFall", this, 1, 1, this);
}

// determine a random falling direction and passes it on to the FxTreeFallTimer.
func FxTreeFallStart(object target, proplist effect)
{
	effect.direction = Random(2); 
	if (effect.direction == 0) effect.direction -= 1;
}

/* animates the falling of the tree: First 10 slow degress then speed up and play the landing sound at 80+ degrees. 
remember that degrees range from -180 to 180. */
func FxTreeFallTimer(object target, proplist effect)
{
	//simple falling if tree is not fully grown
	if (target->GetCon() <= 50)
	{
		target->SetRDir(effect.direction * 10);
	} 
	//else rotate slowly first until about 10 degree. This will be the time needed for the crack sound and makes sense as a tree will start falling slowly.
	else
	{
		if (Abs(target->GetR()) < 10) 
		{
			target->SetRDir(effect.direction * 1);
			//Turn of gravity so the tree doesn't get stuck before its done falling.
			target->SetYDir(0);
		} 
		else 
		{
			//Then speed up and let gravity do the rest.
			target->SetRDir(effect.direction * 10);
		}	
	}
	//if the tree does not lend on a cliff or sth. (is rotated more then 80 degrees in the plus or minus direction) Play the landing sound of the tree.
	if (Abs(target->GetR()) > 80)
	{
		target->SetRDir(0);
		if (target->GetCon() > 50) target->Sound("TreeLanding", false);
		return -1;
	}
	//check every frame if the tree is stuck and stop rotation in that case this is necessary as a tree could get stuck before reaching 80 degrees
	if ((target->GetContact(-1, CNAT_Left) | target->GetContact(-1, CNAT_Right)) > 0)
	{
		target->SetRDir(0);
		return -1;
	}
}
