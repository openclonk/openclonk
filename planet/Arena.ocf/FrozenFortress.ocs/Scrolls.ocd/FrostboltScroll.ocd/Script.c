/*--
	Scroll: Frostbolt
	Author: Mimmo

	Hurl a frozen bolt into your enemies.
--*/


func Initialize()
{
	return _inherited(...);
}

public func ControlUse(object pClonk, int ix, int iy)
{
	AddEffect("Frostbolt", nil, 100, 1, nil, GetID(), pClonk->GetOwner(), Angle(0, 0, ix, iy),pClonk->GetX(), pClonk->GetY());
	Sound("Fire::Fireball");
	Sound("Fire::Fireball");
	RemoveObject();
	return 1;
}



public func FxFrostboltStart(pTarget, effect, iTemp, owner, angle, x, y)
{
	if (iTemp) return;
	x += Sin(angle, 10)+RandomX(-1, 1);
	y+=-Cos(angle, 10)+RandomX(-1, 1);
	effect.owner = owner;
	effect.angle = angle;
	effect.x = x;
	effect.y = y;
	
	effect.air_particles = 
	{
		Prototype = Particles_Air(),
		R = PV_Random(100, 150),
		G = PV_Random(100, 150),
		B = PV_Random(200, 255),
		BlitMode = GFX_BLIT_Additive,
		Size = PV_Random(5, 10)
	};
	effect.fire_particles =
	{
		Prototype = Particles_Fire(),
		R = PV_Random(100, 150),
		G = PV_Random(100, 150),
		B = PV_Random(200, 255),
	};
}

public func FxFrostboltTimer(pTarget, effect, iEffectTime)
{	
	var angle = effect.angle;
	var x = effect.x;
	var y = effect.y;

	if	(	iEffectTime>67  ||
	 		GBackSolid(x, y) ||
	 		FindObject(
	 		Find_Hostile(effect.owner),
	 		Find_OCF(OCF_Alive),
	 		Find_NoContainer(),
	 		Find_Distance(16, x, y)
	 		)
	 	)
	{
		CreateObjectAbove(Dynamite, x, y, effect.owner)->BlueExplode();
		var dummy = CreateObjectAbove(Dummy, x, y,-1);
		dummy->Sound("Hits::Materials::Glass::GlassShatter");
		ScheduleCall(dummy, "RemoveObject", 36);
		for (var i = 0; i<=60;i++)
		{
			var r = Random(10)+Random(18);
			DoBlueExplosion(x + Sin(i*6 ,r),y-Cos(i*6 ,r), 2 + Random(3), nil, effect.owner, nil);
		}
		return -1;
	}	
	else if (iEffectTime < 70)
	{
		angle += Sin(iEffectTime*50, 2)*8;
		x += Sin(angle, 9);
		y+=-Cos(angle, 9);
		effect.x = x;
		effect.y = y;

		CreateParticle("Air", PV_Random(x - 3, x + 3), PV_Random(y - 3, y + 3), PV_Random(-10, 10), PV_Random(-10, 10), 5, effect.air_particles, 10);
		CreateParticle("MagicFire", PV_Random(x - 3, x + 3), PV_Random(y - 3, y + 3), PV_Random(-10, 10), PV_Random(-10, 10), 10, effect.air_particles, 10);
		
	}

	return 1;
	
	
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
