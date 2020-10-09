/**
	Basement
	Provides basements to structures, but can also be built as a single object.
	
	@author: Maikel
*/

#include Library_Structure

local parent;
local width;

func Construction()
{
	// Make sure the basement does not move while constructing.
	SetCategory(C4D_StaticBack);
	return _inherited(...);
}

func Initialize()
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

func Destruction()
{
	// Cast a single rock.
	CastObjects(Rock, 1, 15, 0, -5);
	// Set basement to nil in parent.
	if (parent)
		parent->~SetBasement(nil);
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

/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) 
		return false;
	if (parent)
		props->AddCall("BasementParent", this, "SetParent", parent);
	else if (width != GetDefWidth())
		props->AddCall("BasementWidth", this, "SetWidth", width);
	props->Remove("Category");
	return true;
}

/*-- Construction --*/

public func IsHammerBuildable() { return true; }
// It should not be a structure.
public func IsStructure() { return false; }
// But a basement!
public func IsBasement() { return true; }

// Is a construction that is built just below the surface.
public func IsBelowSurfaceConstruction() { return true; }

// This makes it possible to combine basements with each other.
public func IsStructureWithoutBasement() { return true; }

// Sticking to other structures.

public func ConstructionCombineWith() { return "IsStructureWithoutBasement"; }

public func ConstructionCombineDirection(object other)
{
	// All directions are possible for other basements
	if (other && other->~IsBasement())
		return CONSTRUCTION_STICK_Left | CONSTRUCTION_STICK_Right | CONSTRUCTION_STICK_Bottom | CONSTRUCTION_STICK_Top;

	// For everything else, the basement is below.
	return CONSTRUCTION_STICK_Bottom;
}

public func ConstructionCombineOffset(object other)
{
	// Some structures like the elevator require the basement to have an offset.
	return other->~GetBasementOffset();
}

public func NoConstructionFlip() { return true; }

public func AlternativeConstructionPreview(object previewer, int direction, object combine_with)
{
	if (combine_with && combine_with->~IsBasement()) return;

	var wdt = GetSiteWidth(direction, combine_with);
	previewer->SetObjDrawTransform(1000 * wdt / 40, 0, 0, 0, 1000, 0, previewer.GFX_StructureOverlay);
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
	if (combine_with && combine_with->~IsBasement()) return;

	var wdt = GetSiteWidth(direction, combine_with);
	site->SetGraphics(nil, Basement, 1, GFXOV_MODE_Base);
	site->SetClrModulation(RGBa(255, 255, 255, 128), 1);
	site->SetObjDrawTransform(1000 * wdt / 40, 0, 0, 0, 1000, -4000, 1);
	return true;
}

// Set the parent if the basement is attached to a structure.
public func CombineWith(object stick_to)
{
	if (stick_to && stick_to->~IsBasement()) return;

	SetParent(stick_to);
}


/*-- Editor --*/

public func Definition(def, ...)
{
	_inherited(def, ...);
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.width = { Name="$Width$", Set="SetWidth", Type="int", Min = 8, Max = 120 };
}


/*-- Properties --*/

local Name = "$Name$";
local Description ="$Description$";
local HitPoints = 80;
local Plane = 190;
local Components = {Rock = 2};
