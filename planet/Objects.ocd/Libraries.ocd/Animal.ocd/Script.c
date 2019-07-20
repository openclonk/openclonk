/**
	Animal Library
	Handles reproduction and growth for animals. If included the Construction()
	call must return _inherited(...) for this library to work.
	
	@author Maikel
*/


// This object is an animal.
public func IsAnimal() { return true; }

protected func Construction(...)
{
	// Add a reproduction timer.
	AddReproductionEffect();
	// Add a growth effect.
	StartGrowth(GrowthSpeed());
	_inherited(...);
}

private func AddReproductionEffect()
{
	if (!GetEffect("IntReproduction", this))
		return AddEffect("IntReproduction", this, 100, 72, this);
}

private func RemoveReproductionEffect()
{
	return RemoveEffect("IntReproduction", this);
}


/*-- Growth --*/

// Speed of the growth of an animal.
private func GrowthSpeed() { return 5; }


/*-- Reproduction --*/

local animal_reproduction_area_size = 800;
local animal_reproduction_rate = 67;
local animal_max_count = 10;

// Population control is handled through these variables.
// The area, in which new animals of this kind can appear.
public func ReproductionAreaSize() { return animal_reproduction_area_size; }
public func SetReproductionAreaSize(int v) { animal_reproduction_area_size = v; return true; }
// The chance that reproduction takes place in one timer interval. From 0 to 10000.
// The higher this value the more likely it is to reproduce. Special: If it is zero, reproduction is off.
public func ReproductionRate() { return animal_reproduction_rate; }
public func SetReproductionRate(int v)
{
	animal_reproduction_rate = v;
	if (v) AddReproductionEffect(); else RemoveReproductionEffect();
	return true;
}
// The maximal animal count in the area.
public func MaxAnimalCount() { return animal_max_count; }
public func SetMaxAnimalCount(int v) { animal_max_count = v; return true; }

// Special reproduction method (e.g. with egg).
private func SpecialReproduction()
{
	// You can have a special kind of reproduction implemented here.
	// If you do so return true in this function.
	return false;
}

// Standard conditions for reproduction.
private func ReproductionCondition()
{
	return GetAlive() && GetCon() >= 100;
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
	if (!ReproductionCondition()) 
		return FX_OK;
	// Apply the reproduction rate.
	if (Random(10000) >= ReproductionRate())
		return FX_OK;
	// Special conditions not fulfilled? Don't do anything either.
	if (!SpecialReproductionCondition()) 
		return FX_OK;
	// Check whether there are already enough animals of this kind.
	if (CountAnimalsInArea() >= MaxAnimalCount())
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



/* Editor */

public func Definition(def, ...)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.animal_reproduction_area_size = { Name="$ReproductionAreaSize$", EditorHelp="$ReproductionAreaSizeHelp$", Type="int", Min = 0, AsyncGet="ReproductionAreaSize", Set="SetReproductionAreaSize", Save="Reproduction" };
	def.EditorProps.animal_reproduction_rate = { Name="$ReproductionRate$", EditorHelp="$ReproductionRateHelp$", Type="int", Min = 0, Max = 10000, AsyncGet="ReproductionRate", Set="SetReproductionRate", Save="Reproduction" };
	def.EditorProps.animal_max_count = { Name="$MaxCount$", EditorHelp="$MaxCountHelp$", Type="int", Min = 1, AsyncGet="MaxAnimalCount", Set="SetMaxAnimalCount", Save="Reproduction" };
	return _inherited(def, ...);
}
