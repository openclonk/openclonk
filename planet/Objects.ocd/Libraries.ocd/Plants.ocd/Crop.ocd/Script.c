/**
	Crop
	(Plants) that are harvestable

	@author Clonkonaut
*/

/** Determines whether a plant may be harvested by the player.
	@return \c true if the plant can be harvested, \c false otherwise (default).
*/
private func IsCrop()
{
	return false;
}

/** Determines whether the plant can only be harvested when using a sickle.
	These are very sturdy or economically important plants (like cotton or wheat).
	@return \c true if the plant must be harvested with a sickle (default), \c false otherwise.
*/
public func SickleHarvesting()
{
	return true;
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
	return clonk->IsWalking() && IsCrop() && !SickleHarvesting() && (IsHarvestable() || _inherited(clonk));
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