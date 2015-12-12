/*--
	Boom attack
	Authors: Randrian, Newton, Sven2

	An evil rocket which is hungry on the destruction of Gidls
--*/


/* Init */

public func Construction()
{
	SetAction("Fly");
	SetComDir(COMD_None);
	var fx = AddEffect("Flight",this,150,10,this);
	fx.target = g_statue;
	FxFlightTimer(this, fx, 0);
	return true;
}

public func Initialize()
{
	SetAction("Fly");
	return true;
}


/* Flight */

private func FxFlightTimer(object _this, effect, int time)
{
	// Attack statue!
	var target = g_statue;
	if (!target) { DoFireworks(NO_OWNER); return FX_OK; }
	if(!(time % 10))
	{
		// Adjust angle for something similar to a parabulum towards target
		var dx = target->GetX() - GetX(), dy = target->GetY() - GetY();
		var aim_dist = 600; // at this distance, fly horizontally. when getting closer, gradually turn to direct flight into target
		var aim_dy = dy * (aim_dist - Abs(dx)) / aim_dist;
		var angle = Angle(0,0,dx,aim_dy);
		//Log("angle %d %d %d %d", angle, FlySpeed, dx, aim_dy);
		SetXDir(Sin(angle, FlySpeed), 100);
		SetYDir(-Cos(angle, FlySpeed), 100);
		SetR(angle);
	}
	
	var x = -Sin(GetR(), 15);
	var y = +Cos(GetR(), 15);

	var xdir = GetXDir() / 2;
	var ydir = GetYDir() / 2;
	CreateParticle("FireDense", x, y, PV_Random(xdir - 4, xdir + 4), PV_Random(ydir - 4, ydir + 4), PV_Random(16, 38), Particles_Thrust(), 5);
	
	return FX_OK;
}


/* Contact / Explosion */

public func IsProjectileTarget(target,shooter) { return true; }
public func OnProjectileHit(object shot) { return DoFireworks(shot->GetController()); }

public func ContactBottom() { return Hit(); }
public func ContactTop() { return Hit(); }
public func ContactLeft() { return Hit(); }
public func ContactRight() { return Hit(); }

public func Hit() { return DoFireworks(NO_OWNER); }
public func HitObject() { return DoFireworks(NO_OWNER); }

private func DoFireworks(int killed_by)
{
	GameCallEx("OnClonkDeath", this, killed_by); // for reward
	RemoveEffect("Flight",this);
	Fireworks();
	Explode(40);
	return true;
}


/* Status */

public func IsFlyingEnemy() { return true; }

local ActMap = {

Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	Length = 1,
	Delay = 0,
	Wdt = 15,
	Hgt = 27,
},
};
local PerspectiveR = 20000;
local PerspectiveTheta = 25;
local PerspectivePhi = 30;
local FlySpeed = 100;
local Name = "$Name$";
