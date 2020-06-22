/**
	Meteor
	A burning rock falling from the sky, explodes on impact.
	
	@author Maikel, Zapper
*/

// A meteor can spawn objects on impact.
local spawn_id, spawn_amount;

/*-- Disaster Control --*/

/**
	Enables meteorites.
	The meteorites can be set to spawn objects on impact. The amount to spawn will be spawn_amount randomized by 50%.
*/
public func SetChance(int chance, id spawn_id, int spawn_amount)
{
	spawn_amount = spawn_amount ?? 1;	
	if (GetType(this) != C4V_Def) return FatalError("Must be called as a definition call.");
	
	var effect = FindMeteoriteEffectFor(spawn_id);
	if (!effect)
	 	effect = AddEffect("IntMeteorControl", nil, 100, 20, nil, this, spawn_id, spawn_amount);
	effect.chance = chance;
	
	return true;
}

/**
	Returns the chance of meteorites that is currently set. spawn_id can be nil for the normal meteorite.
*/
public func GetChance(id spawn_id)
{
	if (GetType(this) != C4V_Def) return FatalError("Must be called as a definition call.");
	var effect = FindMeteoriteEffectFor(spawn_id);
	if (effect)
		return effect.chance;
	return 0;
}

// Finds the meteorite effect that matches a specific spawn id.
private func FindMeteoriteEffectFor(id spawn_id)
{
	var i = 0, fx;
	while (fx = GetEffect("IntMeteorControl", nil, i++))
	{
		if (fx.spawn_id == spawn_id) return fx;
	}
	return nil;
}

private func FxIntMeteorControlStart(object target, effect fx, temp, id spawn_id, int spawn_amount)
{
	if (temp) return;
	fx.spawn_id = spawn_id;
	fx.spawn_amount = spawn_amount;
}

private func FxIntMeteorControlTimer(object target, effect fx, int time)
{
	if (Random(100) < fx.chance && !Random(10))
	{
		// Launch a meteor.
		var x = Random(LandscapeWidth());
		var y = 0;
		var size = RandomX(60, 90);
		var xdir = RandomX(-22, 22);
		var ydir = RandomX(28, 36);
		var real_spawn_amount = Max(1, RandomX(fx.spawn_amount / 2, 3 * fx.spawn_amount / 2));
		LaunchMeteor(x, y, size, xdir, ydir, fx.spawn_id, real_spawn_amount);
	}
	return FX_OK;
}

// Scenario saving
public func FxIntMeteorControlSaveScen(obj, fx, props)
{
	props->Add("Meteor", "Meteor->SetChance(%d, %v, %d)", fx.chance, fx.spawn_id, fx.spawn_amount);
	return true;
}

/*-- Meteor --*/

public func Launch(int x, int y, int size, int xdir, int ydir, id spawn_id, int spawn_amount)
{
	// Launch from indicated position.
	SetPosition(x, y);
	// Set the meteor's size.
	SetCon(BoundBy(size, 20, 100));
	// Set the initial velocity.
	SetXDir(xdir);
	SetYDir(ydir);
	// Remember spawning information.
	this.spawn_id = spawn_id;
	this.spawn_amount = spawn_amount ?? 1;
	// Safety check.
	if (!IsLaunchable())
		return false;
	// Allow for some more effects (overloadable).
	this->OnAfterLaunch();
	return this;
}

public func OnAfterLaunch()
{
	// Set random rotation.
	SetR(Random(360));
	SetRDir(RandomX(-10, 10));
	// Emit light
	SetLightRange(300, 30);
	SetLightColor(RGB(255, 160, 120));
	// Set right action.
	AddEffect("IntMeteor", this, 100, 1, this);
}

private func IsLaunchable()
{
	if (GBackSemiSolid() || Stuck())
	{
		RemoveObject();
		return false;
	}
	return true;
}

private func FxIntMeteorStart(object target, effect fx, bool temp)
{
	if (temp) return;
	fx.smoketrail = 
	{
		R = 255,
		B = PV_KeyFrames(0,  0, 100,    30, 0,  100, 255, 1000, 255),
		G = PV_KeyFrames(0,  0, 150,  30, 0, 100, 255, 1000, 255),
		
		Alpha = PV_KeyFrames(1000, 0, 0, 30, 255, 1000, 0),
		Size = PV_Linear(20, 60),
		Stretch = 1000,
		Phase = PV_Random(0, 4),
		Rotation = PV_Random(-GetR() - 15, -GetR() + 15),
		DampingX = 1000,
		DampingY = 1000,
		BlitMode = 0,
		CollisionVertex = 0,
		OnCollision = PC_Stop(),
		Attach = nil
	};
	fx.brighttrail = 
	{
		Prototype = fx.smoketrail,
		Alpha = PV_Linear(180, 0),
		Size = PV_Linear(20, 30),
		BlitMode = GFX_BLIT_Additive,
	};
	fx.frontburn = 
	{
		R = 255,
		B = 50,
		G = 190,
		
		Alpha = PV_KeyFrames(0, 0, 0, 500, 25, 1000, 0),
		Size = PV_Linear(4, 5),
		Stretch = 1000,
		Phase = PV_Random(0, 4),
		Rotation = PV_Random(-GetR() - 15, -GetR() + 15),
		DampingX = 1000,
		DampingY = 1000,
		BlitMode = GFX_BLIT_Additive,
		CollisionVertex = 0,
		OnCollision = PC_Stop(),
		Attach = ATTACH_Front | ATTACH_MoveRelative
	};
}

protected func FxIntMeteorTimer(object target, effect fx, bool temp)
{
	var size = GetCon();
	// Air drag.
	var ydir = GetYDir(100);
	ydir -= size * ydir ** 2 / 11552000; // Magic number.
	SetYDir(ydir, 100);
	// Smoke trail.
	CreateParticle("SmokeThick", 0, 0, PV_Random(-3, 3), PV_Random(-3, 3), 200, fx.smoketrail, 5);
	// Flash
	CreateParticle("SmokeThick", 0, -4, PV_Random(-3, 3), PV_Random(-3, 3), 3, fx.brighttrail, 2);
	// left and right burn
	CreateParticle("FireDense", PV_Random(-5, 5), 15, PV_Random(-5, -20), PV_Random(-15, -30), 20, fx.frontburn, 30);
	CreateParticle("FireDense", PV_Random(-5, 5), 15, PV_Random(5, 20), PV_Random(-15, -30), 20, fx.frontburn, 30);
	// Sound.

	// Burning and friction decrease size.
	if (size > 10 && !Random(5))
		DoCon(-1);

	return FX_OK;
}

// Scenario saving
func FxIntMeteorSaveScen(obj, fx, props)
{
	props->AddCall("Meteor", obj, "AddEffect", "\"IntMeteor\"", obj, 100, 1, obj);
	return true;
}

protected func Hit(int xdir, int ydir)
{
	var size = 10 + GetCon();
	var speed2 = 20 + (xdir ** 2 + ydir ** 2) / 10000;
	// Some fire sparks.
	var particles =
	{
		Prototype = Particles_Fire(),
		Attach = nil
	};
	CreateParticle("Fire", PV_Random(-size / 4, size / 4), PV_Random(-size / 4, size / 4), PV_Random(-size/4, size/4), PV_Random(-size/4, size/4), 30, particles, 20 + size);
	// Explode meteor, explode size scales with the energy of the meteor.	
	var dam = size * speed2 / 500;
	dam = BoundBy(size/2, 5, 30);
	// Blow up a dummy object so that the explosion can happen before we spawn items.
	var dummy = CreateObject(Dummy, 0, 0, GetController());
	dummy->Explode(dam);
	
	// Fling around some objects?
	if (spawn_id)
	{
		for (var i = 0; i < spawn_amount; ++i)
		{
			var angle = Angle(0, 0, -xdir, -ydir) + RandomX(-45, 45);
			var force = BoundBy(GetCon() / 2, 10, 50) + RandomX(-10, 10);
			var item = CreateObject(spawn_id, 0, 0, GetOwner());
			item->SetSpeed(Sin(angle, force), -Cos(angle, force));
		}
	}
	
	RemoveObject();
	return;
}


/*-- Target --*/

public func OnLightningStrike(object lightning, int damage)
{
	SetController(lightning->GetController());
	if (GetDamage() + damage >= 6)
		Hit();
	return;
}

public func IsProjectileTarget(object projectile)
{
	return true;
}

public func OnProjectileHit(object projectile)
{
	SetController(projectile->GetController());
	Hit();
	return;
}

public func IsMeteor() { return true; }


/*-- Proplist --*/

local Name = "$Name$";
