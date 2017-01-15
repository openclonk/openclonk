/**
	Boom Attack
	An evil rocket which attacks you, can be ridden as well.

	@authors Randrian, Newton, Sven2
*/


public func Construction()
{
	SetAction("Fly");
	SetComDir(COMD_None);
	// Notify friendly fire rule.
	GameCallEx("OnCreationRuleNoFF", this);
	// Add flight effects.
	CreateEffect(FxFlightRotation, 100, 1);
	CreateEffect(FxFlight, 100, 10);
	return;
}


/*-- Flight --*/

local FxFlightRotation = new Effect
{
	Construction = func()
	{
		this.rotation = 0;	
	},
	Timer = func(int time)
	{
		if (Target->GetRider())
			return FX_Execute_Kill;

		this.rotation += 2;
		if (this.rotation >= 360)
			this.rotation = 0;

		Target.MeshTransformation = Trans_Rotate(this.rotation, 0, 1, 0);
		return FX_OK;
	}
};

local FxFlight = new Effect
{
	Construction = func()
	{
		this.target = GetRandomAttackTarget(Target);
		this->Timer(0);	
	},
	Timer = func(int time)
	{
		// Find target and if not explode.
		if (!this.target)
		{
			this.target = GetRandomAttackTarget(Target);
			if (!this.target)
			{
				Target->DoFireworks(NO_OWNER);
				return FX_Execute_Kill;	
			}
		}
		
		// Adjust angle every 10 frames.
		if (!(time % 10))
		{
			var dx = this.target->GetX() - Target->GetX();
			var dy = this.target->GetY() + this.target->GetBottom() - Target->GetY();
			// At this distance, fly horizontally. When getting closer, gradually turn to direct flight into target.
			var aim_dist = 600; 
			var aim_dy = dy * (aim_dist - Abs(dx)) / aim_dist;
			var angle = Angle(0, 0, dx, aim_dy);
			Target->SetXDir(Sin(angle, Target.FlySpeed), 100);
			Target->SetYDir(-Cos(angle, Target.FlySpeed), 100);
			Target->SetR(angle);
		}
	
		// Create exhaust fire.
		var x = -Sin(Target->GetR(), 15);
		var y = +Cos(Target->GetR(), 15);
	
		var xdir = Target->GetXDir() / 2;
		var ydir = Target->GetYDir() / 2;
		Target->CreateParticle("FireDense", x, y, PV_Random(xdir - 4, xdir + 4), PV_Random(ydir - 4, ydir + 4), PV_Random(16, 38), Particles_Thrust(), 5);
		return FX_OK;
	}
};


/*-- Riding --*/

local riderattach;
local rider;

public func SetRider(object to)
{
	rider = to;
	return;
}

public func GetRider() { return rider; }

public func OnMount(object clonk)
{
	SetRider(clonk);
	var dir = -1;
	if (GetX() > LandscapeWidth() / 2)
		dir = 1;
	clonk->PlayAnimation("PosRocket", CLONK_ANIM_SLOT_Arms, Anim_Const(0));
	riderattach = AttachMesh(clonk, "main", "pos_tool1", Trans_Translate(-1000, 2000 * dir, 2000));
	return true;
}

public func OnUnmount(object clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	DetachMesh(riderattach);
	return;
}


/*-- Explosion --*/

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
	if (rider)
	{
		rider->Fling(RandomX(-5, 5), -5);
		rider->SetAction("Walk");
		SetRider(nil);
	}
	// Notify defense goal for reward and score.
	GameCallEx("OnClonkDeath", this, killed_by);
	Fireworks();
	Explode(40);
	return;
}

public func Destruction()
{
	// Notify friendly fire rule.
	GameCallEx("OnDestructionRuleNoFF", this);
}

public func HasNoNeedForAI() { return true; }


/*-- Properties --*/

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Length = 1,
		Delay = 0,
		Wdt = 15,
		Hgt = 27,
	}
};

local Name = "$Name$";
local Description = "$Description$";
local ContactCalls = true;
local FlySpeed = 100;
local HasNoFriendlyFire = true;
