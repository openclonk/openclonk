/**
	Animal Library
	Handles reproduction and growth for animals. If included the Construction()
	call must return _inherited(...) for this library to work.
	
	@author Maikel
*/


// This object is an animal.
public func IsAnimal() { return true; }

protected func Construction()
{
	// Add a reproduction timer.
	AddEffect("IntReproduction", this, 100, 72, this);
	// Add a growth effect.
	StartGrowth(GrowthSpeed());
	_inherited(...);
}


/*-- Growth --*/

// Speed of the growth of an animal.
private func GrowthSpeed() { return 5; }


/*-- Reproduction --*/

// Population control is handled through these variables.
// The area, in which new animals of this kind can appear.
private func ReproductionAreaSize() { return 800; }
// The chance that reproduction takes place in one timer interval.
// The higher this value the less likely it is to reproduce.
private func ReproductionRate() { return 150; }
// The maximal animal count in the area.
private func MaxAnimalCount() { return 10; }

// Special reproduction method (e.g. with egg).
private func SpecialReproduction()
{
	// You can have a special kind of reproduction implemented here.
	// If you do so return true in this function.
	return false;
}

// Special conditions which needs to be fulfilled (e.g. a fish should have the swim action).
private func SpecialReproductionCondition()
{
	// You can implement a special condition for reproduction here.
	// Return false if this condition is not satisfied.
	return true;
}

// Count animals in the reproduction area.
private func CountAnimalsInArea()
{
	var reprod_size = ReproductionAreaSize();
	var reprod_size_half = reprod_size / 2;
	return ObjectCount(Find_ID(GetID()), Find_InRect(-reprod_size_half, -reprod_size_half, reprod_size , reprod_size), Find_OCF(OCF_Alive));
}

public func FxIntReproductionTimer(object target, proplist effect, int time)
{
	// Already dead or not full grown? Don't do anything.
	if (!GetAlive() || GetCon() < 100) 
		return FX_OK;
	// Special conditions not fulfilled? Don't do anything either.
	if (!SpecialReproductionCondition()) 
		return FX_OK;
	// Check whether there are already enough animals of this kind.
	if (CountAnimalsInArea() > MaxAnimalCount())
		return FX_OK;
	// Then apply the reproduction rate.
	if (Random(ReproductionRate()))
		return FX_OK;
	// Reproduction: first try special reproduction, otherwise normal.
	if (!SpecialReproduction())
	{
		// Normal reproduction.
		var child = CreateConstruction(GetID(), 0, 0, NO_OWNER, 40);
		child->~Birth(this);
	}
	return FX_OK;
}

// Callback in the animal on its birth.
public func Birth(object parent)
{
	SetAction("Walk");
	if (Random(2)) 
		SetComDir(COMD_Left);
	else
		SetComDir(COMD_Right);
	return;
}


/*-- Collection --*/

protected func RejectEntrance(object container)
{
	// From one container to another is not blocked by this library.
	if (Contained()) 
		return _inherited(container, ...);
	// Neither are dead animals.
	if (!GetAlive()) 
		return _inherited(container, ...);
	// For all other cases the entrance is blocked.
	return true;
}
