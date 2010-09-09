/*
	FireGlobe
	Author: Newton

*/


local sx,sy,ex,ey;
local vis;
local aimed;

func Construction()
{
	vis = nil;
	aimed = false;
}

func ControlUse(object clonk, int x, int y)
{
	if(!aimed) return false;
	
	// fire fireball
	var angle = Angle(0,0,x,y);
	Exit();
	Launch(angle,120,clonk,this);
	SetDivert(sx,sy,ex,ey);
	
	return true;
}

func ControlUseStart(object clonk, int x, int y)
{
	if(aimed) return false;

	sx = x+clonk->GetX();
	sy = y+clonk->GetY();
	
	if(vis) vis->RemoveObject();
	vis = CreateObject(VisualPath,0,0,clonk->GetOwner());
	vis->Set(sx,sy,x+clonk->GetX(),y+clonk->GetY());
	vis["Visibility"]=VIS_Owner;

	return true;
}

func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int x, int y)
{
	if(aimed) return false;
	
	if(vis) vis->Set(sx,sy,x+clonk->GetX(),y+clonk->GetY());
}

func ControlUseStop(object clonk, int x, int y)
{
	if(aimed) return false;
	
	ex = x+clonk->GetX();
	ey = y+clonk->GetY();
	
	vis->Set(sx,sy,ex,ey);
	aimed=true;
	
	return true;
}

public func DelLine()
{
	if(vis) vis->RemoveObject();
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
	var xdir = Sin(angle,str);
	var ydir = Cos(angle,-str);
	SetXDir(xdir);
	SetYDir(ydir);
	
	AddEffect("HitCheck", this, 1,1, nil,nil, shooter);
	AddEffect("InFlight", this, 1,1, this);
}


public func SetDivert(int x1, int y1, int x2, int y2)
{
	var inflight = GetEffect("InFlight",this);
	EffectVar(2,this,inflight) = x1;
	EffectVar(3,this,inflight) = y1;
	EffectVar(4,this,inflight) = x2;
	EffectVar(5,this,inflight) = y2;
	EffectVar(6,this,inflight) = true;
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
public func FxInFlightStart(object target, int effect, int temp)
{
	if(temp) return;
	EffectVar(0,target,effect) = target->GetX();
	EffectVar(1,target,effect) = target->GetY();
}

public func FxInFlightTimer(object target, int effect, int time)
{
	var oldx = EffectVar(0,target,effect);
	var oldy = EffectVar(1,target,effect);
	var newx = target->GetX();
	var newy = target->GetY();

	if(EffectVar(6,target,effect))
	{
		var ax = EffectVar(2,target,effect);
		var ay = EffectVar(3,target,effect);
		var bx = EffectVar(4,target,effect);
		var by = EffectVar(5,target,effect);
		
		var xo, yo;
		if(Intersect(oldx, oldy, newx, newy, ax, ay, bx, by, xo, yo))
		{
			var angle = Angle(ax, ay, bx, by);
			var speed = 60;
			target->SetXDir(Sin(angle,speed));
			target->SetYDir(-Cos(angle, speed));
			
			EffectVar(6,target,effect) = false;
		}
	}
	
	EffectVar(0,target,effect) = newx;
	EffectVar(1,target,effect) = newy;
}

global func Intersect(int Ax, int Ay, int Bx, int By, int Px, int Py, int Qx, int Qy, &Xout, &Yout)
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
	if(denominator == 0)
	{
		if(numerator != 0) return false; 
		// on same line somewhere
		else
		{
			Xout = Ax;
			Yout = Ay;
			return true;
		}
	}

	// in parameter bounds?
	var Y = 10000 * numerator/denominator;
	
	if(!Inside(Y,0,10000)) return false;

	// we don't want division by zero...
	if(BAy != 0) {
		numerator = (PAy + Y*QPy/10000);
		denominator = BAy;
	}
	else if(BAx != 0) {
		numerator = (PAx + Y*QPx/10000);
		denominator = BAx;
	}

	// in parameter bounds
	var X = 10000*numerator / denominator;
	
	if(!Inside(X,0,10000)) return false;

	// this is the point...
	Xout = Ax+X*(BAx)/10000;
	Yout = Ay+X*(BAy)/10000;

	return true;
}

local Name = "$Name$";
