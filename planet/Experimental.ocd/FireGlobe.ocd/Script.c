/*
	FireGlobe
	Author: Newton

*/


local sx, sy, ex, ey;
local vis;
local aimed;

func Construction()
{
	vis = nil;
	aimed = false;
}

func ControlUse(object clonk, int x, int y)
{
	if (!aimed) return false;
	
	// fire fireball
	var angle = Angle(0, 0, x, y);
	Exit();
	Launch(angle, 120, clonk);
	SetDivert(sx, sy, ex, ey);
	
	return true;
}

func ControlUseStart(object clonk, int x, int y)
{
	if (aimed) return false;

	sx = x + clonk->GetX();
	sy = y + clonk->GetY();
	
	if (vis) vis->RemoveObject();
	vis = CreateObjectAbove(VisualPath, 0, 0, clonk->GetOwner());
	vis->Set(sx, sy, x + clonk->GetX(),y + clonk->GetY());
	vis["Visibility"]=VIS_Owner;

	return true;
}

func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int x, int y)
{
	if (aimed) return false;
	
	if (vis) vis->Set(sx, sy, x + clonk->GetX(),y + clonk->GetY());
}

func ControlUseStop(object clonk, int x, int y)
{
	if (aimed) return false;
	
	ex = x + clonk->GetX();
	ey = y + clonk->GetY();
	
	vis->Set(sx, sy, ex, ey);
	aimed = true;
	
	return true;
}

public func DelLine()
{
	if (vis) vis->RemoveObject();
}

public func Deselection()
{
	DelLine();
}

public func Destruction()
{
	DelLine();
}


public func Launch(int angle, int str, object shooter)
{
	var xdir = Sin(angle, str);
	var ydir = Cos(angle,-str);
	SetXDir(xdir);
	SetYDir(ydir);
	
	AddEffect("HitCheck", this, 1, 1, nil, nil, shooter);
	AddEffect("InFlight", this, 1, 1, this);
}


public func SetDivert(int x1, int y1, int x2, int y2)
{
	var inflight = GetEffect("InFlight",this);
	inflight.ax = x1;
	inflight.ay = y1;
	inflight.bx = x2;
	inflight.by = y2;
	inflight.freeflight = true;
}

public func HitObject(object obj)
{
	Hit();
}

public func Hit()
{
	DelLine();
	Explode(20);
}

// rotate arrow according to speed
public func FxInFlightStart(object target, effect, int temp)
{
	if (temp) return;
	effect.x = target->GetX();
	effect.y = target->GetY();
}

public func FxInFlightTimer(object target, effect, int time)
{
	var oldx = effect.x;
	var oldy = effect.y;
	var newx = target->GetX();
	var newy = target->GetY();

	if (effect.freeflight)
	{
		var ax = effect.ax;
		var ay = effect.ay;
		var bx = effect.bx;
		var by = effect.by;
		
		var pos = Intersect(oldx, oldy, newx, newy, ax, ay, bx, by);
		if (pos != nil)
		{
			var angle = Angle(ax, ay, bx, by);
			var speed = 60;
			target->SetXDir(Sin(angle, speed));
			target->SetYDir(-Cos(angle, speed));
			
			effect.freeflight = false;
		}
	}
	
	effect.x = newx;
	effect.y = newy;
}

// Returns nil or coordinates of intersection.
global func Intersect(int Ax, int Ay, int Bx, int By, int Px, int Py, int Qx, int Qy)
{
	var BAx = Bx-Ax;
	var BAy = By-Ay;
	var PAx = Px-Ax;
	var PAy = Py-Ay;
	var QPx = Qx-Px;
	var QPy = Qy-Py;

	var denominator = (BAy*QPx - BAx*QPy);
	var numerator = (BAx*PAy - BAy*PAx);
	// parallel!
	if (denominator == 0)
	{
		if (numerator != 0) return nil;
		// on same line somewhere
		else
		{
			return [Ax, Ay];
		}
	}

	// in parameter bounds?
	var Y = 10000 * numerator/denominator;
	
	if (!Inside(Y, 0, 10000)) return nil;

	// we don't want division by zero...
	if (BAy != 0) {
		numerator = (PAy + Y*QPy/10000);
		denominator = BAy;
	}
	else if (BAx != 0) {
		numerator = (PAx + Y*QPx/10000);
		denominator = BAx;
	}

	// in parameter bounds
	var X = 10000*numerator / denominator;
	
	if (!Inside(X, 0, 10000)) return nil;

	// this is the point...
	var x = Ax + X*(BAx)/10000;
	var y = Ay + X*(BAy)/10000;

	return [x, y];
}

local Name = "$Name$";
local Collectible = 1;
