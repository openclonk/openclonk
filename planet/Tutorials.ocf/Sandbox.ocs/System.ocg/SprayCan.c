// Convert spray can into material draw tool.

#appendto SprayCan

local paint_bg;
local brush_mode = 1; // 1 = Draw Brush, 2 = Quad Brush, 3 = Eraser

public func Construction()
{
	SetColor(RGB(Random(256), Random(256), Random(256)));
	return;
}

// Item activation
public func ControlUseStart(object clonk, int x, int y)
{
	paint_col = clonk.SelectedBrushMaterial;
	paint_bg = clonk.SelectedBrushBgMaterial;
	brush_mode = clonk.SelectedBrushMode;
	return ControlUseHolding(clonk, x, y);
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Work in global coordinates.
	new_x += GetX(); new_y += GetY();

	// (re-)start spraying.
	if (GetAction() != "Spraying")
		StartSpraying(clonk, new_x, new_y);
	
	// Spray paint if position moved.
	if (new_x == last_x && new_y == last_y)
		return true;
	var wdt = clonk.SelectedBrushSize;
	var dx = new_x - last_x, dy = new_y - last_y;
	var d = Distance(dx, dy);
	var ldx = dy * wdt / d, ldy = -dx * wdt / d;
	if (!last_ldx && !last_ldy)
	{
		last_ldx = ldx;
		last_ldy = ldy;
	}
	
	if (brush_mode == 1)
	{
		DrawMaterialQuad(paint_col, last_x - last_ldx, last_y - last_ldy, last_x + last_ldx, last_y + last_ldy, new_x + ldx,new_y + ldy, new_x - ldx, new_y - ldy, paint_bg);
	}
	
	else if (brush_mode == 2)
	{
		DrawMaterialQuad(paint_col, new_x - (wdt / 2), new_y - (wdt / 2), new_x + (wdt / 2), new_y - (wdt / 2), new_x + (wdt / 2), new_y + (wdt / 2), new_x - (wdt / 2), new_y + (wdt / 2), paint_bg);
	}
	
	else if (brush_mode == 3)
	{
		// Draw something to set BG Mat to sky (workaround for not being able to draw Sky via DrawMaterialQuad)
		DrawMaterialQuad(paint_col, new_x - (wdt / 2), new_y - (wdt / 2), new_x + (wdt / 2), new_y - (wdt / 2), new_x + (wdt / 2), new_y + (wdt / 2), new_x - (wdt / 2), new_y + (wdt / 2), DMQ_Sky);
		ClearFreeRect(new_x - (wdt / 2), new_y - (wdt / 2), wdt, wdt);
	}
	
	last_x = new_x; last_y = new_y;
	last_ldx = ldx; last_ldy = ldy;
	return true;
}

public func QueryRejectDeparture(object clonk)
{
	return true;
}

public func Departure(object clonk)
{
	RemoveObject();
	return;
}

