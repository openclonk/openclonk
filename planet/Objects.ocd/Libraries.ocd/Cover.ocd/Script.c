/**
	Cover Library
	Simple library for objects that provide (visible) cover from other players.
	Object will be invisible to players that have their (crew) cursor inside cover_area (Rectangle).
	
	@author Win, Clonkonaut
*/

local cover_area;

private func Initialize()
{
	AddTimer("CheckCoverage", 1);
	this.Visibility = [VIS_Select];
	_inherited(...);
}

private func CheckCoverage()
{
	if (!cover_area) return;

	for (var i = 0; i < GetPlayerCount(); i++)
	{
		// Visible to all players
		this.Visibility[GetPlayerByIndex(i) + 1] = true;
		var cursor = GetCursor(GetPlayerByIndex(i));
		if (cursor)
			if (cover_area->IsPointContained(cursor->GetX()-GetX(), cursor->GetY()-GetY()))
				// Except when the cursor is inside the cover area
				this.Visibility[GetPlayerByIndex(i) + 1] = false;
	}
}