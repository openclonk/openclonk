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

protected func Initialize()
{
	var wdt = BoundBy(GetObjWidth(), 8, 120);
	if (parent)
		wdt = BoundBy(parent->GetObjWidth(), 8, 120);
	SetWidth(wdt);
	// Move objects out of the basement.
	MoveOutOfBasement();
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

public func  SetParent(object to_parent)
{
	parent = to_parent;
	SetWidth(BoundBy(parent->GetObjWidth(), 8, 120));
	// Notify the parent.
	parent->~SetBasement(this);
	return;
}

public func GetParent() { return parent; }

// Move objects out of the basement.
private func MoveOutOfBasement()
{
	// Find all objects inside the basement which are stuck.
	var wdt = GetObjWidth();
	var hgt = GetObjHeight();
	var lying_around = FindObjects(Find_Or(Find_Category(C4D_Vehicle), Find_Category(C4D_Object), Find_Category(C4D_Living)), Find_InRect(-wdt / 2 - 1, -hgt / 2 - 2, wdt + 2, hgt + 4));
	// Move up these objects.
	for (var obj in lying_around)
	{
		var x = obj->GetX();
		var y = obj->GetY();
		var dif = 0;
		// Move up object until it is not stuck any more.
		while (obj->Stuck() || obj->GetContact(-1, CNAT_Bottom))
		{
			// Only up to 32 pixels.
			if (dif > 32)
			{
				obj->SetPosition(x, y);
				break;
			}
			dif++;
			obj->SetPosition(x, y - dif);
		}
	}
	return;
}

// Is a construction that is built just below the surface.
public func IsBelowSurfaceConstruction() { return true; }

// Sticking to other structures, at the bottom of that structure.
public func ConstructionCombineWith() { return "IsStructureWithoutBasement"; }
public func ConstructionCombineDirection() { return CONSTRUCTION_STICK_Bottom; }

public func NoConstructionFlip() { return true; }

// Don't stick to itself, so it should not be a structure.
public func IsStructure() { return false; }


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) 
		return false;
	if (parent)
		props->AddCall("BasementParent", this, "SetParent", parent);
	else if (width)
		props->AddCall("BasementWidth", this, "SetWidth", width);
	return true;
}


/*-- Proplist --*/

local Name = "$Name$";
local Description ="$Description$";
local HitPoints = 80;
local Plane = 190;