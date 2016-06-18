/**
	Basement
	Provides basements to structures, but can also be built as a single object.
	
	@author Maikel
*/

#include Library_Structure

local parent;
local width;

protected func Construction()
{
	// Make sure the basement does not move while constructing.
	SetCategory(C4D_StaticBack);
	return _inherited(...);
}

public func IsHammerBuildable() { return true; }

protected func Initialize()
{
	var wdt = GetObjWidth();
	if (parent)
	{
		wdt = parent->~GetBasementWidth();
		if (wdt == nil)
			wdt = parent->GetObjWidth();
	}
	SetWidth(BoundBy(wdt, 8, 120));
	// Move objects out of the basement.
	MoveOutOfSolidMask();
	return _inherited(...);
}

protected func Destruction()
{
	// Cast a single rock.
	CastObjects(Rock, 1, 15, 0, -5);
	return _inherited(...);
}

// Set the width of the basement.
public func SetWidth(int wdt)
{
	width = wdt;
	SetShape(-wdt / 2, -4, wdt, 8);
	SetSolidMask(0, 0, wdt, 8, 20 - wdt / 2, 0);
	SetObjDrawTransform(1000 * wdt / 40, 0, 0, 0, 1000, 0);
	return;
}

public func GetWidth() { return width; }

// Set the parent if the basement is attached to a structure.
public func CombineWith(object stick_to)
{
	SetParent(stick_to);
	return;
}

public func SetParent(object to_parent)
{
	parent = to_parent;
	var wdt = parent->~GetBasementWidth();
	if (wdt == nil)
		wdt = parent->GetObjWidth();
	SetWidth(BoundBy(wdt, 8, 120));
	// Notify the parent.
	parent->~SetBasement(this);
	return;
}

public func GetParent() { return parent; }

// Is a construction that is built just below the surface.
public func IsBelowSurfaceConstruction() { return true; }

// Sticking to other structures, at the bottom of that structure.
public func ConstructionCombineWith() { return "IsStructureWithoutBasement"; }
public func ConstructionCombineDirection() { return CONSTRUCTION_STICK_Bottom; }
public func ConstructionCombineOffset(object other)
{
	// Some structures like the elevator require the basement to have an offset.
	return other->~GetBasementOffset();
}

public func NoConstructionFlip() { return true; }

public func AlternativeConstructionPreview(object previewer, int direction, object combine_with)
{
	var wdt = GetSiteWidth(direction, combine_with);
	previewer->SetObjDrawTransform(1000 * wdt / 40, 0, 0, 0, 1000, 0, previewer.GFX_StructureOverlay);
	return;
}

public func GetSiteWidth(int direction, object combine_with)
{
	var wdt = GetDefWidth();
	if (combine_with)
	{
		wdt = combine_with->~GetBasementWidth();
		if (wdt == nil)
			wdt = combine_with->GetObjWidth();
	}
	return BoundBy(wdt, 8, 120);
}

public func SetConstructionSiteOverlay(object site, int direction, object combine_with)
{
	var wdt = GetSiteWidth(direction, combine_with);
	site->SetGraphics(nil, Basement, 1, GFXOV_MODE_Base);
	site->SetClrModulation(RGBa(255, 255, 255, 128), 1);
	site->SetObjDrawTransform(1000 * wdt / 40, 0, 0, 0, 1000, -4000, 1);
	return true;
}

// Don't stick to itself, so it should not be a structure.
public func IsStructure() { return false; }


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) 
		return false;
	if (parent)
		props->AddCall("BasementParent", this, "SetParent", parent);
	else if (width != GetObjWidth())
		props->AddCall("BasementWidth", this, "SetWidth", width);
	props->Remove("Category");
	return true;
}


/*-- Proplist --*/

local Name = "$Name$";
local Description ="$Description$";
local HitPoints = 80;
local Plane = 190;
local Components = {Rock = 2};
