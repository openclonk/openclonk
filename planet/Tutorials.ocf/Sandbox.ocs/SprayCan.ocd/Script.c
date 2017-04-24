/* Spray can */

local last_x, last_y, last_ldx, last_ldy;
local paint_col;
local paint_bg;
local max_dist = 50;
local brushmode = 1; // 1 = Draw Brush, 2 = Quad Brush, 3 = Eraser

func Construction()
{
	SetColor(RGB(Random(256),Random(256),Random(256)));
}

// Impact sound
public func Hit()
{
	Sound("Hits::GeneralHit?");
}

// Item activation
public func ControlUseStart(object clonk, int x, int y)
{
	paint_col = clonk.SelectedBrushMaterial;
	paint_bg = clonk.SelectedBrushBgMaterial;
	brushmode = clonk.SelectedBrushMode;
	return ControlUseHolding(clonk, x, y);
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int new_x, int new_y)
{
	// Out of reach? Stop spraying. // Not for Gods like Sandboxers ;)
	//if (Distance(0,0,new_x,new_y) > max_dist)
	//{
		//SetAction("Idle");
		//return true;
	//}

	// Work in global coordinates
	new_x += GetX(); new_y += GetY();

	// (re-)start spraying
	if (GetAction() != "Spraying") StartSpraying(clonk, new_x, new_y);
	
	// Spray paint if position moved
	if (new_x==last_x && new_y == last_y) return true;
	var wdt = clonk.SelectedBrushSize;
	var dx=new_x-last_x, dy=new_y-last_y;
	var d = Distance(dx,dy);
	var ldx = dy*wdt/d, ldy = -dx*wdt/d;
	if (!last_ldx && !last_ldy) { last_ldx=ldx; last_ldy=ldy; }
	
	if (brushmode == 1)
	{
		DrawMaterialQuad(paint_col, last_x-last_ldx,last_y-last_ldy, last_x+last_ldx,last_y+last_ldy, new_x+ldx,new_y+ldy, new_x-ldx,new_y-ldy, paint_bg);
	}
	
	else if (brushmode == 2)
	{
		DrawMaterialQuad(paint_col, new_x - (wdt / 2), new_y - (wdt / 2), new_x + (wdt / 2), new_y - (wdt / 2), new_x + (wdt / 2), new_y + (wdt / 2), new_x - (wdt / 2), new_y + (wdt / 2), paint_bg );
	}
	
	else if (brushmode == 3)
	{
		// Draw something to set BG Mat to sky (workaround for not being able to draw Sky via DrawMaterialQuad)
		DrawMaterialQuad(paint_col, new_x - (wdt / 2), new_y - (wdt / 2), new_x + (wdt / 2), new_y - (wdt / 2), new_x + (wdt / 2), new_y + (wdt / 2), new_x - (wdt / 2), new_y + (wdt / 2), DMQ_Sky );
		ClearFreeRect(new_x - (wdt / 2), new_y - (wdt / 2), wdt, wdt);
	}
	
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
	last_ldx=last_ldy=0;
	var r = Random(90), wdt = 2;
	var ldx = Sin(r, wdt), ldy = Cos(r, wdt);
	DrawMaterialQuad(paint_col, x-ldx,y-ldy, x-ldy,y+ldx, x+ldx,y+ldy, x+ldy,y-ldx, paint_bg);
	SetAction("Spraying");
	return true;
}


local ActMap = {
	Spraying = {
		Prototype = Action,
		FacetBase = 1,
		Length = 1,
		Delay = 1,
		Name = "Spraying",
		Sound = "SprayCan::SprayCan",
		NextAction = "Spraying",
	}
};

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Rotate(-30,0,1,1),def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
