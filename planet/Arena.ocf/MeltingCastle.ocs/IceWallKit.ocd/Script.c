/* Wall kit */
// Modified to draw Ice + not allowed too close to flag

#include WallKit

public func ControlUseStop(object clonk, int x, int y, ...)
{
	StopPreview(clonk);
	if (!IsIceBridgeAllowed(clonk, x, y))
	{
		clonk->PlaySoundDoubt();
		clonk->Message("$NoWall$");
		return true;
	}
	else
	{
		return inherited(clonk, x, y, ...);
	}
}

private func IsIceBridgeAllowed(object clonk, int x, int y)
{
	var c = Offset2BridgeCoords(clonk, x, y);
	// Must not intersect a flag
	// Search from clonk context because that's the coordinate space returned by Offset2BridgeCoords
	var flag = clonk->FindObject(Find_ID(Goal_Flag), clonk->Find_AtRect(Min(c.x1, c.x2)-5, Min(c.y1, c.y2)-5, Abs(c.x1 - c.x2)+11, Abs(c.y1 - c.y2)+11));
	return !flag;
}

private func SetPreview(object clonk, int x, int y, ...)
{
	if (!preview) AddTimer(this.UpdateIcePreviewColor, 3);
	this.ice_last_clonk = clonk;
	this.ice_last_x = x;
	this.ice_last_y = y;
	var r = inherited(clonk, x, y, ...);
	UpdateIcePreviewColor();
	return r;
}

private func UpdateIcePreviewColor()
{
	if (preview)
	{
		var ok = IsIceBridgeAllowed(this.ice_last_clonk, this.ice_last_x, this.ice_last_y);
		if (ok)
			preview->SetColor(0xff80ffff);
		else
			preview->SetColor(0xffff0000);
	}
	return true;
}

private func StopPreview(object clonk, ...)
{
	RemoveTimer(this.UpdateIcePreviewColor);
	return inherited(clonk, ...);
}


/* Status */

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local BridgeLength = 15;
local BridgeThickness = 5;
local BridgeMaterial = "Ice-ice";
