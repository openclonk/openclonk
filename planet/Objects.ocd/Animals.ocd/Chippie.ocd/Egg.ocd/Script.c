/**
	Chippie Egg
	New chippies hatch from here.
	
	@author Zapper
*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;

// Default random transform of this egg.
local transform;

local age;

public func Place(int amount, proplist area)
{
	var location = nil;
	if (area) location = Loc_InArea(area->GetBoundingRectangle());

	while (amount > 0)
	{
		var p = nil;
		if (Random(2)) p = FindLocation(Loc_Tunnel(), location);
		if (p == nil)
			p = FindLocation(Loc_Solid(), location);
		if (p == nil)
		{
			--amount;
			continue;
		}
		
		// small circle
		for (var r = 0; (r < 360) && (amount > 0); r += 40 + Random(40))
		{
			var o = CreateObject(Chippie_Egg, p.x + Sin(r, 10 + RandomX(-2, 2)), p.y - Cos(r, 10 + RandomX(-2, 2)), NO_OWNER);
			o->SetCon(RandomX(90, 100));
			--amount;
		}
	}
}

public func Entrance()
{
	SetVertex(0, VTX_X, 0, 2);
	SetVertex(0, VTX_Y, 0, 2);
}

public func Construction()
{
	AddTimer("RndHatch", 100);
	transform = Trans_Mul(Trans_Rotate(RandomX(-90, 90), 0, 0, 1),  Trans_Rotate(RandomX(-90, 90), 1, 0, 0));
	this.MeshTransformation = transform;
}

public func Initialize()
{
	age = FrameCounter();
	Yoink();
	return true;
}

private func GetLifeTime()
{
	return FrameCounter() - age;
}

public func Hit(int x, int y)
{
	if (Hatch()) return;
	
	// Put one vertex into wall so that the egg sticks.
	var angle = Angle(0, 0, x, y);
	var x_capped = +Sin(angle, 5);
	var y_capped = -Cos(angle, 5);
	if (GBackSolid(x_capped, y_capped))
	{
		// Need to offset the rotation.
		var x_rotation = Sin(angle - GetR(), 5);
		var y_rotation = -Cos(angle - GetR(), 5);
		SetVertex(0, VTX_X, x_rotation, 2);
		SetVertex(0, VTX_Y, y_rotation, 2);
	}
	else
	{
		SetVertex(0, VTX_X, 0, 2);
		SetVertex(0, VTX_Y, 0, 2);
	}
		
	SetRDir(0);
	Sound("Hits::SoftHit*");
	
	var particles = 
	{
		Prototype = Particles_Material(RGB(50, 200, 50)),
		DampingX = 950, DampingY = 950,
	};
	CreateParticle("WoodChip", PV_Random(-5, 5), PV_Random(-5, 5),
					PV_Random(-10, 10), PV_Random(-10, 10),
					PV_Random(10, 20), particles, 30);
					
	Yoink();
}

private func Yoink()
{
	AddEffect("Wiggle", this, 1, 1, this);
}

private func FxWiggleStart(target, effect, temp)
{
	if (temp) return;
	effect.start = Random(360);
}

private func FxWiggleTimer(target, effect, time)
{
	var magnitude = Abs(Sin(6 * time, 1000)); 
	var x = Sin(effect.start + 4 * time, magnitude);
	var y = -Cos(effect.start + 5 * time, magnitude);
	this.MeshTransformation = Trans_Mul(transform, Trans_Scale(1000 + x/4, 1000 + y/4, 1000 + Max(x, y)/4));
	
	if (magnitude <= 1 && Random(2))
		return -1;
	return 1;
}

private func FxWiggleStop(target, effect, cause, temp)
{
	if (temp) return;
	this.MeshTransformation = transform;
}

private func RndHatch()
{
	if (!Random(30))
		Hatch();
}

private func Hatch()
{
	if (GetLifeTime() < 36 * 120) return false;
	if (Stuck()) return false;
	if (Contained()) return false;
	if (GBackSemiSolid(0, -1)) return false;
	var cnt = ObjectCount(Find_Distance(50), Find_ID(Chippie));
	if (cnt >= 5) return false;
	
	var c = CreateObject(Chippie, 0, 0, GetOwner());
	c->SetCon(50);
	c->Sound("Animals::Chippie::EggCrack*", false, 100);
	
	var particles = 
	{
		Prototype = Particles_Material(RGB(100, 255, 50)),
		DampingX = 800, DampingY = 800,
		ForceY = -GetGravity() / 10,
	};
	CreateParticle("SmokeDirty", PV_Random(-5, 5), PV_Random(-5, 5),
					PV_Random(-10, 10), PV_Random(-10, 10),
					PV_Random(10, 20), particles, 60);
	RemoveObject();
	return true;
}

// If spawned via a meteor, use a cool green skin.
public func GetMeteorSkin()
{
	return Meteor_Alien;
}
