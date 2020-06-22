/**
	Ownership Area Block
	Allows to draw ownership areas which block areas from being conquered.
	
	@author Maikel
*/

local lib_block_area;

protected func Construction()
{
	// Initialize the flag list if not done already.
	if (GetType(LIB_FLAG_FlagList) != C4V_Array)
		LIB_FLAG_FlagList = [];
	// Add blocking area to list of flags.
	if (GetIndexOf(LIB_FLAG_FlagList, this) == -1)
		PushBack(LIB_FLAG_FlagList, this);
	// Initialize the single proplist for the this library.
	if (lib_block_area == nil)
		lib_block_area = {};
	// Initialize some variables for this library.
	lib_block_area.markers = [];
	lib_block_area.rectangle = nil;
	return;
}

public func BlockRectangle(proplist rect)
{
	if (this != Library_BlockOwnershipArea)
		return;
	var block_area = CreateObject(Library_BlockOwnershipArea, rect.x + rect.wdt / 2, rect.y + rect.hgt / 2);
	block_area.lib_block_area.rectangle = rect;
	block_area->UpdateMarkers();
	return;
}

public func HasCoordinatesInControlArea(int x, int y)
{
	if (!Inside(x, lib_block_area.rectangle.x, lib_block_area.rectangle.x + lib_block_area.rectangle.wdt))
		return false;
	if (!Inside(y, lib_block_area.rectangle.y, lib_block_area.rectangle.y + lib_block_area.rectangle.hgt))
		return false;
	return true;
}


/*-- Flag Library Overloads --*/

// Always return zero since these areas are supposed to block player flags.
public func GetFlagConstructionTime() { return 0; }

public func RedrawFlagRadius() { return; }

public func GetLinkedFlags() { return []; }

public func GetPowerHelper() { return; }

public func SetPowerHelper() { return; }


/*-- Markers --*/

// Removes all the ownership markers for this flag.
private func ClearMarkers()
{
	for (var marker in lib_block_area.markers)
		if (marker) 
			marker->RemoveObject();
	lib_block_area.markers = [];
	return;
}

public func UpdateMarkers()
{
	var step_size = 30;
	var min_amount = 3;
	var x_amount = Max(min_amount, lib_block_area.rectangle.wdt / step_size);
	var y_amount = Max(min_amount, lib_block_area.rectangle.hgt / step_size);
	var marker_positions = [];
	// Construct sides.
	for (var index = 1; index < x_amount; index++)
	{
		var x = lib_block_area.rectangle.x - GetX() + index * lib_block_area.rectangle.wdt / x_amount;
		PushBack(marker_positions, {x = x, y = lib_block_area.rectangle.y - GetY(), r = 0});
		PushBack(marker_positions, {x = x, y = lib_block_area.rectangle.y + lib_block_area.rectangle.hgt - GetY(), r = 180});
	}
	for (var index = 1; index < y_amount; index++)
	{
		var y = lib_block_area.rectangle.y - GetY() + index * lib_block_area.rectangle.hgt / y_amount;
		PushBack(marker_positions, {x = lib_block_area.rectangle.x - GetX(), y = y, r = -90});
		PushBack(marker_positions, {x = lib_block_area.rectangle.x + lib_block_area.rectangle.wdt - GetX(), y = y, r = 90});	
	}
	// Construct edges.
	PushBack(marker_positions, {x = lib_block_area.rectangle.x - GetX(), y = lib_block_area.rectangle.y - GetY(), r = -45});
	PushBack(marker_positions, {x = lib_block_area.rectangle.x + lib_block_area.rectangle.wdt - GetX(), y = lib_block_area.rectangle.y - GetY(), r = 45});
	PushBack(marker_positions, {x = lib_block_area.rectangle.x - GetX() - GetX(), y = lib_block_area.rectangle.y + lib_block_area.rectangle.hgt - GetY(), r = -135});
	PushBack(marker_positions, {x = lib_block_area.rectangle.x + lib_block_area.rectangle.wdt - GetX(), y = lib_block_area.rectangle.y + lib_block_area.rectangle.hgt - GetY(), r = 135});
	for (var marker_pos in marker_positions)
	{
		var other_flag = GetFlagpoleForPosition(marker_pos.x + GetX(), marker_pos.y + GetY());
		if (other_flag && (other_flag != this) && (other_flag->GetFlagConstructionTime() <= GetFlagConstructionTime()))
			continue;
		var marker = CreateObject(Library_Flag_Marker, marker_pos.x, marker_pos.y);
		marker->FadeIn();
		marker->SetR(marker_pos.r);	
	}
	return;
}
