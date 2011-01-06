/*--
	Scroll: Fireball
	Author: Mimmo

	Hurl a fiery ball into your enemies.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	AddEffect("Fireball", 0, 100, 1, 0, GetID(), pClonk->GetOwner(), Angle(0,0,ix,iy),pClonk->GetX(), pClonk->GetY());
	Sound("Fireball.ogg");
	Sound("Fireball.ogg");
	RemoveObject();
	return 1;
}



public func FxFireballStart(pTarget, effect, iTemp, owner, angle, x, y)
{
	if(iTemp) return;
	x+=Sin(angle, 10)+RandomX(-1, 1);
	y+=-Cos(angle, 10)+RandomX(-1, 1);
	effect.var0=owner;
	effect.var1=angle;
	effect.var2=x;
	effect.var3=y;
}

public func FxFireballTimer(pTarget, effect, iEffectTime)
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
		CreateObject(Dynamite,x,y,-1)->Explode(14);
		for(var i=0; i<=3;i++) CreateObject(Dynamite,x+Sin(i*120 +x,13),y-Cos(i*120 +x,13),-1)->Explode(6+Random(4));
		return -1;
	}	
	else if(iEffectTime < 70)
	{
		CreateParticle("FireballSmoke",x,y,Sin(Random(360),2),Cos(Random(360),2),RandomX(120,180),RGBa(100,100,100,70));
		CreateParticle("MagicSpark",x,y,Sin(Random(360),RandomX(5,13)),Cos(Random(360),RandomX(5,13)),RandomX(30,70),RGB(255,255,255));

		//if(!Random(10)) if(Random(2))angle++; else angle--;
		angle+=Sin(iEffectTime*30,18);
		x+=Sin(angle, 6);
		y+=-Cos(angle, 6);
		effect.var2=x;
		effect.var3=y;
		for(var i=0;i<6;++i)
		{
			var c=HSL(Random(50), 200+Random(25), Random(100));
			var rx=RandomX(-2, 2);
			var ry=RandomX(-2, 2);
			CreateParticle("MagicFire", x+rx, y+ry, Sin(angle+180,6)+ry, -Cos(angle+180,6)+rx, 80+Random(20), c);
			CreateParticle("MagicFire", x+rx, y+ry, Sin(angle+180,6)+ry, -Cos(angle+180,6)+rx, 20+Random(10), RGB(255,255,0));
		}
	}

	return 1;
	
	
}

local Name = "$Name$";
local Description = "$Description$";
