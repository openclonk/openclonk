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
	return true;
}

/** Determines whether the plant can only be harvested when using a sickle.
	These are very sturdy or economically important plants (like cotton or wheat).
	@return \c true if the plant must be harvested with a sickle (default), \c false otherwise.
*/
public func SickleHarvesting()
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

/** By default, plants are interactable if SickleHarvesting is false. The plant should be picked on interaction.
*/
public func IsInteractable(object clonk)
{
	return clonk->IsWalking() && !SickleHarvesting() && IsHarvestable();
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

/** Default will call Harvest if IsHarvestable.
*/
public func Interact(object clonk)
{
	if (IsHarvestable())
		return Harvest(clonk);
	else
		return false;
}

/** Called when the plant is harvested. Default is Split2Components.
	@param The harvesting clonk
	@return \c true is successfully harvested
*/
public func Harvest(object clonk)
{
	Split2Components();
	return true;
}

/* Watering */

/** This is optional behaviour. Plant will grow faster if there is a little water around. The plant may also shrink if its submerged.
To activate, use AddTimer("WaterCheck", 70 + Random(10)); or similar in Construction.
Plant must have the following properties defined:
growth: Usual growth factor
fastgrowth: Growth factor when water was absorbed (until it is check again)

Optionally you can also define degrowth. If != 0 the plant will start shrinking if the liquid is more than 10 pixels deep.
*/
private func WaterCheck()
{
	// Fully grown
	if (GetCon() >= 100) return RemoveTimer("WaterCheck");
	if (OnFire()) return;

	// Center must be clear
	if (GBackSolid()) return;

	var next_growth = this.growth;

		// I still have water
	if (this.watered)
	{
		next_growth = this.fastgrowth;
		this.watered--;
	}
	else
	{
		var skip_extract = false;
		var my_width = GetObjWidth();
		var my_height = GetObjHeight() / 2;
		var water = 0;
		if (this.degrowth)
		{
			// Do a simple check if there is too much water present
			if (GBackLiquid(0, my_height) && GBackLiquid(0, my_height - 10))
			{
				next_growth = this.degrowth;
				skip_extract = true;
			}
		}
		if (!skip_extract)
		{
			// Check for water
			for (var i = 0; i < my_width + 1; i++)
			{
				var y = 0;
				var x = i - my_width/2;
				while (!GBackLiquid(x, my_height - y) && y < my_height + 1)
					y++;
				if (MaterialName(GetMaterial(x, y)) == "Water")
					if (ExtractLiquid(x, y))
						water++;
				if (water >= 5) // Extract a maximum of 5 pixels of water
					break;
			}
			if (water)
			{
				next_growth = this.fastgrowth;
				// Save how much water has been consumed
				this.watered = water;
			}
		}
	}

	var grow_effect = GetEffect("IntGrowth", this);
	if (!grow_effect) StartGrowth(next_growth);
	else if (grow_effect.growth != next_growth)
		grow_effect.growth = next_growth;
}
