/*--
	Scroll: Fireball
	Author: Mimmo

	Hurl a fiery ball into your enemies.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	AddEffect("Fireball", nil, 100, 1, nil, GetID(), pClonk->GetOwner(), Angle(0, 0, ix, iy),pClonk->GetX(), pClonk->GetY());
	Sound("Fire::Fireball");
	Sound("Fire::Fireball");
	RemoveObject();
	return 1;
}



public func FxFireballStart(pTarget, effect, iTemp, owner, angle, x, y)
{
	if (iTemp) return;
	x += Sin(angle, 10)+RandomX(-1, 1);
	y+=-Cos(angle, 10)+RandomX(-1, 1);
	effect.owner = owner;
	effect.angle = angle;
	effect.x = x;
	effect.y = y;
}

public func FxFireballTimer(pTarget, effect, iEffectTime)
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
		CreateObjectAbove(Dynamite, x, y,-1)->Explode(14);
		for (var i = 0; i<=3;i++) CreateObjectAbove(Dynamite, x + Sin(i*120 +x, 13),y-Cos(i*120 +x, 13),-1)->Explode(6 + Random(4));
		return -1;
	}	
	else if (iEffectTime < 70)
	{
		angle += Sin(iEffectTime*30, 18);
		var xspeed = Sin(angle, 6);
		var yspeed = -Cos(angle, 6);

		CreateParticle("SmokeDirty", x, y, PV_Random(-2, 2), PV_Random(-2, 2), PV_Random(30, 60), Particles_SmokeTrail(), 1);
		CreateParticle("FireDense", x, y, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(5, 20), Particles_Fire(), 10);
		CreateParticle("Fire", x, y, PV_Random(0, - 10 * xspeed), PV_Random(0, - 10 * yspeed), PV_Random(20, 90), Particles_Glimmer(), 10);

		effect.x += xspeed;
		effect.y += yspeed;
	}

	return 1;
	
	
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
