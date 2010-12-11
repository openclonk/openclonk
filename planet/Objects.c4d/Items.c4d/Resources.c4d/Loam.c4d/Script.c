/* Loam */

local last_x, last_y; // Last drawing position of bridge (global coordinates)
local last_frame;     // Last frame during which a bridge chunk was drawn
local begin_frame;    // Starting frame of briding process
local target_x, target_y; // local target coordinates during bridging
local loamused;       // amound of loam already used

static const LOAM_Bridge_Amount = 37; // bridge length in pixels

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

// Impact sound
func Hit()
{
	Sound("WoodHit");
}

// Item activation
func ControlUseStart(object clonk, int x, int y)
{
	// Clonk must stand on ground. Allow during SCALE; but Clonk won't keep animation if he's not actually near the ground
	var clnk_proc = clonk->GetProcedure();
	if(clnk_proc != "WALK" && clnk_proc != "SCALE")
	{
		clonk->CancelUse();
		return true;
	}

	// Gfx
	clonk->SetAction("Bridge");
	clonk->SetComDir(COMD_None);
	clonk->SetXDir(0);
	clonk->SetYDir(0);
	last_x = BoundBy(x,-0,0)+GetX(); last_y = clonk->GetDefBottom()+3;
	last_frame = begin_frame = FrameCounter();
	
	target_x = x; target_y = y;

	AddEffect("IntBridge", clonk, 1, 1, this);
	
	return true;
}

func HoldingEnabled() { return true; }

func FxIntBridgeTimer(clonk, number)
{
	// something happened - don't try to dig anymore
	if(clonk->GetAction() != "Bridge")
	{
		clonk->CancelUse();
		return true;
	}

	// clonk faces bridge direction
	var tdir = 0;
	if (x > 0) ++tdir;
	clonk->SetDir(tdir);

	// bridge speed: Build in smaller steps when briding upwards so Clonk moves up with bridge
	var min_dt = 3;
	if (target_y < -20 && !Abs(target_x*5/target_y)) min_dt=2;

	// get global drawing coordinates
	var x = target_x + GetX(), y = target_y + GetY();

	// bridge speed by dig physical
	var speed = clonk.ActMap.Dig.Speed/6;

	// build bridge in chunks (for better angle precision)
	var dt = FrameCounter() - last_frame;
	if (dt < min_dt) return true;
	last_frame += dt;

	// draw loam (earth) line
	var line_wdt = 3;
	var line_len = speed * dt;
	var dx = x-last_x, dy=y-last_y, d=Distance(dx, dy);

	// Don't use up loam if the mouse position is reached...
	// wait for the mouse being moved and then continue bridging
	// into that direction
	if(!d) return true;

	var ox = dy * line_wdt / d, oy = -dx * line_wdt / d;
	dx = dx * line_len / (d*10);
	dy = dy * line_len / (d*10);
	DrawMaterialQuad("Earth-earth", last_x-ox,last_y-oy, last_x+dx-ox,last_y+dy-oy, last_x+dx+ox,last_y+dy+oy, last_x+ox,last_y+oy, DMQ_Bridge);
	last_x += dx;
	last_y += dy;

	// bridge time is up?
	loamused += Max(line_len/10,1);
	if(loamused >= LOAM_Bridge_Amount)
	{
		clonk->CancelUse();
	}
}

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	target_x = new_x;
	target_y = new_y;
	
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
	if(clonk->GetAction() == "Bridge")
	{
		clonk->SetAction("Walk");
		clonk->SetComDir(COMD_Stop);
	}
	// Remove loam object if most of it has been consumed
	if(loamused > LOAM_Bridge_Amount - 10)
	{
		RemoveObject();
	}
	// Remove Effect
	RemoveEffect("IntBridge", clonk);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
