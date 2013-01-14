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
		var meteor = CreateObject(Meteor);
		var x = Random(LandscapeWidth());
		var y = 0;
		var size = RandomX(60, 90);
		var xdir = RandomX(-22, 22);
		var ydir = RandomX(28, 36);
		meteor->Launch(x, y, size, xdir, ydir);		
	}
	return FX_OK;
}

global func LaunchMeteor(int x, int y, int size, int xdir, int ydir)
{
	var meteor = CreateObject(Meteor);
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
	CreateParticle("ExploSmoke", Random(5)-2, Random(5)-2, Random(3)-1, Random(3)-1, size + RandomX(-20,20), RGBa(130,130,130,90));
	CreateParticle("FireballSmoke", 0, 0, Random(3)-1, Random(3)-1, RandomX(120,180), RGBa(100,100,100,70));
	// Fire trail.
	CreateParticle("MagicSpark", 0, 0, Sin(Random(360),RandomX(15,33)), Cos(Random(360), RandomX(15,33)), RandomX(30,70), RGB(255,255,255));
	for (var i = 0; i < 6; i++)
	{
		var theta = RandomX(-45, 45);
		var x = Sin(theta, size / 8);
		var y = Cos(theta, size / 8);
		CreateParticle("MagicFire", x, y, Random(3)-1, Random(3)-1 ,RandomX(50, 90), HSL(Random(50), 200+Random(25), Random(100)));
	}
	// Sound.

	// Burning and friction decrease size.
	if (!Random(5))
		DoCon(-1);
	// Removal if size < 10.
	if (size < 10)
		RemoveObject();
	return 1;
}

protected func Hit(int xdir, int ydir)
{
	var size = 10 + GetCon();
	var speed2 = 20 + (xdir ** 2 + ydir ** 2) / 10000;
	// Some fire sparks.
	for (var i = 0; i < 6; i++)
	{
		var theta = RandomX(135, 225);
		var x = Sin(theta, size / 8);
		var y = Cos(theta, size / 8);
		CreateParticle("MagicFire", x, y, Random(3)-1, Random(3)-1 ,RandomX(50, 90), HSL(Random(50), 200+Random(25), Random(100)));
	}
	// Explode meteor, explode size scales with the energy of the meteor.	
	var dam = size * speed2 / 750;
	dam = BoundBy(dam, 5, 30);
	Explode(dam);
	return;
}

/*-- Proplist --*/

local Name = "$Name$";
