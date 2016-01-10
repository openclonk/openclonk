/*--
	Boom attack
	Authors: Randrian, Newton, Sven2

	An evil rocket which is hungry on the destruction of Windmills
--*/


/* Init */

public func Construction()
{
	SetAction("Fly");
	SetComDir(COMD_None);
	var fx = AddEffect("Flight", this, 150, 10, this);
	fx.target = GetRandomWindmill();
	FxFlightTimer(this, fx, 0);
	AddEffect("FlightRotation", this, 151, 1, this);
	return true;
}

/* Flight */

local rotation = 0;

private func FxFlightRotationTimer(object _this, effect, int time)
{
	if (rider) return FX_Execute_Kill;

	rotation += 2;
	if (rotation > 360) rotation = 0;

	this.MeshTransformation = Trans_Rotate(rotation, 0,1);
	return FX_OK;
}

private func FxFlightTimer(object _this, effect, int time)
{
	// Attack!
	if (!effect.target)
	{
		if (g_lost) return DoFireworks(NO_OWNER);
		effect.target = GetRandomWindmill();
	}
	if(!(time % 10))
	{
		// Adjust angle
		var dx = effect.target->GetX() - GetX(), dy = effect.target->GetY()+50 - GetY();
		var aim_dist = 600; // at this distance, fly horizontally. when getting closer, gradually turn to direct flight into target
		var aim_dy = dy * (aim_dist - Abs(dx)) / aim_dist;
		var angle = Angle(0,0,dx,aim_dy);
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

/* Riding */

local riderattach;
local rider;

public func OnMount(clonk)
{
	rider = clonk;
	var iDir = -1;
	if (GetX() > LandscapeWidth()/2) iDir = 1;
	clonk->PlayAnimation("PosRocket", CLONK_ANIM_SLOT_Arms, Anim_Const(0), Anim_Const(1000));
	riderattach = AttachMesh(clonk, "main", "pos_tool1", Trans_Mul(Trans_Translate(-1000,2000*iDir,2000)));

	return true;
}

public func OnUnmount(clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	DetachMesh(riderattach);
	return true;
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
	if (rider) {
		rider->SetAction("Walk");
		rider->Fling(RandomX(-5,5), -5);
	}
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

local FlySpeed = 100;
local Name = "$Name$";