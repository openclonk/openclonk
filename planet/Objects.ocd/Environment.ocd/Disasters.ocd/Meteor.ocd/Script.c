/**
	Meteor
	A burning rock falling from the sky, explodes on impact.
	
	@author Maikel	
*/


/*-- Disaster Control --*/

public func SetChance(int chance)
{
	if (this != Meteor)
		return;
	var effect = GetEffect("IntMeteorControl");
	if (!effect)
	 	effect = AddEffect("IntMeteorControl", nil, 100, 20, nil, Meteor);
	effect.chance = chance;
	return;
}

public func GetChance()
{
	if (this != Meteor)
		return;
	var effect = GetEffect("IntMeteorControl");
	if (effect)
		return effect.chance;
	return;
}

protected func FxIntMeteorControlTimer(object target, proplist effect, int time)
{
	if (Random(100) < effect.chance && !Random(10))
	{
		// Launch a meteor.
		var meteor = CreateObjectAbove(Meteor);
		var x = Random(LandscapeWidth());
		var y = 0;
		var size = RandomX(60, 90);
		var xdir = RandomX(-22, 22);
		var ydir = RandomX(28, 36);
		meteor->Launch(x, y, size, xdir, ydir);		
	}
	return FX_OK;
}

// Scenario saving
func FxIntMeteorControlSaveScen(obj, fx, props)
{
	props->Add("Meteor", "Meteor->SetChance(%d)", fx.chance);
	return true;
}

global func LaunchMeteor(int x, int y, int size, int xdir, int ydir)
{
	var meteor = CreateObjectAbove(Meteor);
	return meteor->Launch(x, y, size, xdir, ydir);
}

/*-- Meteor --*/

public func Launch(int x, int y, int size, int xdir, int ydir)
{
	// Launch from indicated position.
	SetPosition(x, y);
	// Set the meteor's size.
	SetCon(BoundBy(size, 20, 100));
	// Set the initial velocity.
	SetXDir(xdir);
	SetYDir(ydir);
	// Set random rotation.
	SetR(Random(360));
	SetRDir(RandomX(-10, 10));
	// Safety check.
	if (!IsLaunchable())
		return false;
	// Set right action.
	AddEffect("IntMeteor", this, 100, 1, this);
	return true;
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

protected func FxIntMeteorTimer()
{
	var size = GetCon();
	// Air drag.
	var ydir = GetYDir(100);
	ydir -= size * ydir ** 2 / 11552000; // Magic number.
	SetYDir(ydir, 100);
	// Smoke trail.
	CreateParticle("Smoke", PV_Random(-2, 2), PV_Random(-2, 2), PV_Random(-3, 3), PV_Random(-3, 3), 30 + Random(60), Particles_SmokeTrail(), 5);
	// Fire trail.
	CreateParticle("MagicSpark", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), 16, Particles_SparkFire(), 4);
	CreateParticle("Fire", PV_Random(-size / 8, size / 8), PV_Random(-size / 8, size / 8), PV_Random(-1, 1), PV_Random(-1, 1), 30, Particles_FireTrail(), 6 + size / 10);
	// Sound.

	// Burning and friction decrease size.
	if (size > 10 && !Random(5))
		DoCon(-1);

	return 1;
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
	Explode(dam);
	return;
}

// Scenario saving
func FxIntMeteorSaveScen(obj, fx, props)
{
	props->AddCall("Meteor", obj, "AddEffect", "\"IntMeteor\"", obj, 100, 1, obj);
	return true;
	}

/*-- Proplist --*/

local Name = "$Name$";
