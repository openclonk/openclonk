/**
	Plant
	Basic functionality for all plants

	@author Clonkonaut
*/

/* Global */

/** Pulls the plant above ground if it was buried by PlaceVegetation. A plant must have 'Bottom' and 'Center' CNAT to use this (bottom is the point which should be buried, center the lowest point that must not be buried)
*/
global func RootSurface()
{
	while(GetContact(-1) & CNAT_Center) SetPosition(GetX(),GetY()-1); //Move up if too far underground
	var moved_down = false;
	if (!(GetContact(-1) & CNAT_Bottom)) moved_down = true;
	while(!(GetContact(-1) & CNAT_Bottom)) SetPosition(GetX(),GetY()+1); //Move down if in midair

	if (moved_down) SetPosition(GetX(),GetY()+1); // make the plant stuck, not just contact with surface (in case it was moved down)
}

/* Local */

// This is a plant
public func IsPlant()
{
	return true;
}

/** Chance to reproduce plant. Chances are one out of return value. Default is 500.
	@return the chance, higher = less chance.
*/
private func SeedChance()
{
	return 500;
}
/** Distance the seeds may travel. Default is 500.
	@return the maximum distance.
*/
private func SeedAreaSize()
{
	return 300;
}
/** The amount of plants allowed within SeedAreaSize. Default is 8.
	@return the maximum amount of plants.
*/
private func SeedAmount()
{
	return 8;
}

/** Automated positioning via RootSurface, make sure to call this if needed (in case Construction is overloaded)
*/
protected func Construction()
{
	Schedule(this, "RootSurface()", 1);
	_inherited(...);
}

/** Reproduction. Make sure to call via TimerCall=Seed and Timer=350 (duration should be fixed so SeedChance controls the chance to reproduce)
*/
private func Seed()
{
	if(!Random(SeedChance()))
	{
		var iSize = SeedAreaSize();
		var iOffset = iSize / -2;
		if(ObjectCount(Find_ID(this->GetID()), Find_Distance(iSize)) < SeedAmount())
		{
			PlaceVegetation(GetID(), iOffset, iOffset, iSize, iSize, 3);
		}
	}
}

/* Harvesting */

/** Determines whether a plant may be harvested by the player.
	@return \c true if the plant can be harvested, \c false otherwise (default).
*/
private func CanBeHarvested()
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
	return clonk->GetProcedure() == "WALK" && CanBeHarvested() && IsHarvestable() || _inherited(clonk);
}

public func GetInteractionMetaInfo(object clonk)
{
	if (CanBeHarvested())
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
	if (CanBeHarvested())
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