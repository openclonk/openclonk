/*--
	Scroll: Frostbolt
	Author: Mimmo

	Hurl a frozen bolt into your enemies.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	AddEffect("Frostbolt", 0, 100, 1, 0, GetID(), pClonk->GetOwner(), Angle(0,0,ix,iy),pClonk->GetX(), pClonk->GetY());
	Sound("Fireball.ogg");
	Sound("Fireball.ogg");
	RemoveObject();
	return 1;
}



public func FxFrostboltStart(pTarget, effect, iTemp, owner, angle, x, y)
{
	if(iTemp) return;
	x+=Sin(angle, 10)+RandomX(-1, 1);
	y+=-Cos(angle, 10)+RandomX(-1, 1);
	effect.var0=owner;
	effect.var1=angle;
	effect.var2=x;
	effect.var3=y;
}

public func FxFrostboltTimer(pTarget, effect, iEffectTime)
{	
	var angle=effect.var1;
	var x=effect.var2;
	var y=effect.var3;

	if	(	iEffectTime>67  ||
	 		GBackSolid(x,y) ||
	 		FindObject(
	 		Find_Hostile(effect.var0),
	 		Find_OCF(OCF_Alive),
	 		Find_NoContainer(),
	 		Find_Distance(16,x,y)
	 		)
	 	)
	{
		CreateObject(Dynamite,x,y,effect.var0)->BlueExplode();
		CreateObject(Star,x,y,-1)->Sound("glass.ogg");
		for(var i=0; i<=60;i++)
		{
			var r=Random(10)+Random(18);
			DoBlueExplosion(x+Sin(i*6 ,r),y-Cos(i*6 ,r), 2+Random(3), nil, effect.var0, nil);
			}
		return -1;
	}	
	else if(iEffectTime < 70)
	{
		for(var i=0; i<3; i++)
		CreateParticle("Air",x+Sin(angle,-i*2),y-Cos(angle,-i*2),Sin(Random(360),4),Cos(Random(360),4),RandomX(120,180),RGBa(100,100,100,70));
		CreateParticle("AirIntake",x,y,Sin(Random(360),RandomX(5,13)),Cos(Random(360),RandomX(5,13)),RandomX(30,70),RGB(255,255,255));
		angle+=Sin(iEffectTime*50,2)*8;
		x+=Sin(angle, 9);
		y+=-Cos(angle, 9);
		effect.var2=x;
		effect.var3=y;
		for(var i=0;i<6;++i)
		{
			var c=HSL(128+Random(40), 200+Random(25), Random(100));
			var rx=RandomX(-3, 3);
			var ry=RandomX(-3, 3);
			CreateParticle("Air", x+rx, y+ry, Sin(angle+180,6)+ry, -Cos(angle+180,6)+rx, 80+Random(20), c);
			CreateParticle("MagicFire", x+rx, y+ry, Sin(angle+180,6)+ry, -Cos(angle+180,6)+rx, 20+Random(10), HSL(0,0,255));
		}
	}

	return 1;
	
	
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
