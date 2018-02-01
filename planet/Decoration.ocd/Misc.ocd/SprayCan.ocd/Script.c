/**
	Spray can
	Let out the little artist in you!
	
	@author Sven2	
*/

local last_x, last_y, last_ldx, last_ldy;
local paint_col;
local max_dist = 50;

static SprayCan_last_col;

public func Construction()
{
	SetPaintCol(SprayCan_last_col++);
	return;
}

public func SetPaintCol(int idx)
{
	idx %= 5;
	var tex_name = Format("Paint%s",["Red", "Green", "Teal", "Yellow", "White"][idx]);
	var tex_color = GetAverageTextureColor(tex_name);
	SetColor(tex_color);
	paint_col = Format("Tunnel-%s", tex_name);
	return true;
}

// Impact sound
public func Hit()
{
	Sound("Hits::GeneralHit?");
}

// Item activation
public func ControlUseStart(object clonk, int x, int y)
{
	return ControlUseHolding(clonk, x, y);
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Out of reach? Stop spraying.
	if (Distance(0, 0, new_x, new_y) > max_dist)
	{
		SetAction("Idle");
		return true;
	}

	// Work in global coordinates
	new_x += GetX(); new_y += GetY();

	// (re-)start spraying
	if (GetAction() != "Spraying")
		StartSpraying(clonk, new_x, new_y);
	
	// Spray paint if position moved
	if (new_x == last_x && new_y == last_y)
		return true;
	var wdt = 2;
	var dx = new_x - last_x, dy = new_y - last_y;
	var d = Distance(dx, dy);
	var ldx = dy * wdt / d, ldy = -dx *wdt / d;
	if (!last_ldx && !last_ldy)
	{
		last_ldx = ldx;
		last_ldy = ldy;
	}
	DrawMaterialQuad(paint_col, last_x - last_ldx, last_y - last_ldy, last_x + last_ldx, last_y + last_ldy, new_x + ldx, new_y + ldy, new_x - ldx, new_y - ldy, DMQ_Bridge);
	last_x = new_x; last_y = new_y;
	last_ldx = ldx; last_ldy = ldy;
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	SetAction("Idle");
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	SetAction("Idle");
	return true;
}

private func StartSpraying(object clonk, int x, int y)
{
	// Go into spray mode and place an initial blob
	last_x = x; last_y = y;
	last_ldx = last_ldy = 0;
	var r = Random(90), wdt = 2;
	var ldx = Sin(r, wdt), ldy = Cos(r, wdt);
	DrawMaterialQuad(paint_col, x - ldx, y - ldy, x - ldy, y + ldx, x + ldx, y + ldy, x + ldy, y - ldx, DMQ_Bridge);
	SetAction("Spraying");
	return true;
}

public func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Rotate(-30, 0, 1, 1), def);
}


/*-- Properties --*/

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";

local ActMap = {
	Spraying = {
		Prototype = Action,
		FacetBase = 1,
		Length = 1,
		Delay = 1,
		Name = "Spraying",
		Sound = "Objects::SprayCan",
		NextAction = "Spraying",
	}
};
