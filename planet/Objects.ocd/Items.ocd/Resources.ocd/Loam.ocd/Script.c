/* Loam */

local loamused;       // amound of loam already used

static const LOAM_Bridge_Amount = 37; // bridge length in pixels

protected func Construction()
{
	var graphic = Random(5);
	if (graphic)
		SetGraphics(Format("%d",graphic));
}

// Impact sound
func Hit()
{
	Sound("GeneralHit?");
}

// Item activation
func ControlUseStart(object clonk, int x, int y)
{
	// Clonk must stand on ground. Allow during SCALE; but Clonk won't keep animation if he's not actually near the ground
	var clnk_proc = clonk->GetProcedure();
	if (clnk_proc != "WALK" && clnk_proc != "SCALE")
	{
		clonk->CancelUse();
		return true;
	}

	// Gfx
	clonk->SetAction("Bridge");
	clonk->SetComDir(COMD_Stop);
	clonk->SetXDir(0);
	clonk->SetYDir(0);
	// Add bridge effect and pass target coordinates.
	AddEffect("IntBridge", clonk, 1, 1, this, nil, x, y);
	
	return true;
}

func HoldingEnabled() { return true; }

func FxIntBridgeStart(object clonk, proplist effect, int temp, int x, int y)
{
	if (temp)
		return FX_OK;
	// Drawing times.
	effect.Begin = 0;
	effect.Last = 0;
	// Last bridge coordinates.
	effect.LastX = GetX();
	effect.LastY = clonk->GetDefBottom() + 3;
	// Target coordinates.
	effect.TargetX = x;
	effect.TargetY = y;
	return FX_OK;
}

func FxIntBridgeTimer(object clonk, proplist effect, int time)
{
	// something happened - don't try to dig anymore
	if (clonk->GetAction() != "Bridge")
	{
		clonk->CancelUse();
		return true;
	}

	// clonk faces bridge direction
	var tdir = 0;
	// get global drawing coordinates
	var x = effect.TargetX + GetX();
	var y = effect.TargetY + GetY();
	if (x > 0) ++tdir;
	clonk->SetDir(tdir);

	// bridge speed: Build in smaller steps when briding upwards so Clonk moves up with bridge
	var min_dt = 3;
	if (effect.TargetY < -20 && !Abs(effect.TargetX*5/effect.TargetY))
		min_dt = 2;

	// bridge speed by dig physical
	var speed = clonk.ActMap.Dig.Speed/6;

	// build bridge in chunks (for better angle precision)
	var dt = time - effect.Last;
	if (dt < min_dt) return FX_OK;
	effect.Last += dt;

	// draw loam (earth) line
	var line_wdt = 3;
	var line_len = speed * dt;
	var last_x = effect.LastX;
	var last_y = effect.LastY;
	var dx = x-last_x, dy=y-last_y, d=Distance(dx, dy);	
	// Quantize angle as a multiple of 45 degrees.
	var quant = 30;
	var angle = Angle(0, 0, dx, dy);
	angle = angle + quant/2 - Sign(angle-quant/2)*((angle-quant/2) % quant);
	dx = Sin(angle, d);
	dy = -Cos(angle, d);

	// Don't use up loam if the mouse position is reached...
	// wait for the mouse being moved and then continue bridging
	// into that direction
	if(!d) return FX_OK;

	var ox = dy * line_wdt / d, oy = -dx * line_wdt / d;
	dx = dx * line_len / (d*10);
	dy = dy * line_len / (d*10);
	DrawMaterialQuad("Earth-earth", last_x-ox,last_y-oy, last_x+dx-ox,last_y+dy-oy, last_x+dx+ox,last_y+dy+oy, last_x+ox,last_y+oy, DMQ_Bridge);
	effect.LastX += dx;
	effect.LastY += dy;

	// bridge time is up?
	loamused += Max(line_len/10,1);
	if (loamused >= LOAM_Bridge_Amount)
	{
		clonk->CancelUse();
	}
	return FX_OK;
}

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	var effect = GetEffect("IntBridge", clonk);
	if (!effect)
		return true;
	// Update target coordinates in bridge effect.
	effect.TargetX = new_x;
	effect.TargetY = new_y;	
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	LoamDone(clonk);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	LoamDone(clonk);
	return true;
}

private func LoamDone(object clonk)
{
	// Get out of animation
	if (clonk->GetAction() == "Bridge")
	{
		clonk->SetAction("Walk");
		clonk->SetComDir(COMD_Stop);
	}
	// Remove Effect
	RemoveEffect("IntBridge", clonk);
	// Remove loam object if most of it has been consumed
	if (loamused > LOAM_Bridge_Amount - 10)
		RemoveObject();
	return;
}

public func IsFoundryProduct() { return true; }
public func GetLiquidNeed() { return ["Water", 150]; }
public func GetMaterialNeed() { return ["Earth", 25]; }

public func GetMaterialIcon(string mat) { return Earth; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
