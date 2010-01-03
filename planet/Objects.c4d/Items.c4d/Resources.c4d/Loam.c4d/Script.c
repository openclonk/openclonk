/* Loam */

local last_x, last_y, last_frame, begin_frame;

static const LOAM_Bridge_Time = 70; // frames

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}

// Impact sound
func Hit()
{
  Sound("WoodHit");
}

// Item activation
func ControlUse(object clonk, int x, int y)
{
  // Clonk must stand on ground
	if(clonk->GetAction() == "Walk")
	{
	  // Gfx
		clonk->SetAction("Bridge");
		clonk->SetComDir(COMD_None);
		clonk->SetXDir(0);
		clonk->SetYDir(0);
		last_x = BoundBy(x,-0,0)+GetX(); last_y = clonk->GetDefBottom()+3;
		last_frame = begin_frame = FrameCounter();
	}
	return true;
}

func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int x, int y)
{
	// something happened - don't try to dig anymore
	if(clonk->GetAction() != "Bridge") return -1;
	
	// clonk faces bridge direction
	var tdir = 0;
	if (x > 0) ++tdir;
	clonk->SetDir(tdir);
	
	// bridge speed: Build in smaller steps when briding upwards so Clonk moves up with bridge
  var min_dt = 3;
  if (y < -20 && !Abs(x*5/y)) min_dt=1;
  
  // get global drawing coordinates
	x += GetX(); y += GetY();
	
	// bridge speed by dig physical
	var speed = clonk->GetPhysical("Dig")/3500;
	
	// build bridge in chunks (for better angle precision)
  var dt = FrameCounter() - last_frame;
  if (dt < min_dt) return true;
  last_frame += dt;
  
  // draw loam (earth) line
  var line_wdt = 4;
  var line_len = speed * dt;
  var dx = x-last_x, dy=y-last_y, d=Distance(dx, dy);
  var ox = dy * line_wdt / d, oy = -dx * line_wdt / d;
  dx = dx * line_len / (d*10);
  dy = dy * line_len / (d*10);
  DrawMaterialQuad("Earth-earth", last_x-ox,last_y-oy, last_x+dx-ox,last_y+dy-oy, last_x+dx+ox,last_y+dy+oy, last_x+ox,last_y+oy);
  last_x += dx;
  last_y += dy;
  
  // bridge time is up?
  if (last_frame - begin_frame >= LOAM_Bridge_Time) return -1;
  
  return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
  RemoveObject();
  
	if(clonk->GetAction() != "Bridge") return true;
	clonk->SetAction("Walk");
	clonk->SetComDir(COMD_Stop);

	return true;
}

public func IsTool() { return 1; }
