/**
	Tools Workshop

	@author Clonkonaut
*/

#appendto ToolsWorkshop

local is_station;

protected func Initialize()
{
	is_station = true;
	return _inherited();
}

// Appears in the bottom interaction bar, if cable is connected
public func IsInteractable()
{
	if (GetLength(this->~GetDestinations())) return true;
	return _inherited(...);
}

public func GetCableXOffset() { return 15; }
public func GetCableYOffset() { return 5; }