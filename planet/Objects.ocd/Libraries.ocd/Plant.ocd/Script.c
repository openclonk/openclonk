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
	if (HasCNAT(CNAT_Center))
	{
		var i = 0;
		while(GetContact(-1) & CNAT_Center && i < GetObjHeight()/2) { SetPosition(GetX(),GetY()-1); i++; } //Move up if too far underground
	}
	if (HasCNAT(CNAT_Bottom))
	{
		i = 0;
		while(!(GetContact(-1) & CNAT_Bottom) && i < GetObjHeight()/2) { SetPosition(GetX(),GetY()+1); i++; } //Move down if in midair

		if (!Stuck()) SetPosition(GetX(),GetY()+1); // try make the plant stuck
	}
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
private func SeedChance() {	return 500; }

/** Distance the seeds may travel. Default is 300.
	@return the maximum distance.
*/
private func SeedAreaSize() { return 300; }

/** The amount of plants allowed within SeedAreaSize. Default is 8.
	@return the maximum amount of plants.
*/
private func SeedAmount() { return 8; }

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
	// Max damage reached -> fall down
	if (GetDamage() > MaxDamage() && IsStanding()) ChopDown();
	_inherited(...);
}

/** Called when the trees shall fall down (has taken max damage). Default behaviour is unstucking (5 pixel movement max) and removing C4D_StaticBack.
*/
public func ChopDown()
{
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