/*--
	Scroll: Hardening
	Author: Mimmo

	Create a storm to blow away your enemies.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	AddEffect("HardeningSpell", 0, 100, 1, 0, GetID(), Angle(0,0,ix,iy),pClonk->GetX(), pClonk->GetY());
	RemoveObject();
	return 1;
}



public func FxHardeningSpellStart(pTarget, iEffectNumber, iTemp, angle, x, y)
{
	if(iTemp) return;
	iEffectNumber.var0=Sin(angle,4);
	iEffectNumber.var1=-Cos(angle,4);
	iEffectNumber.var2=x;
	iEffectNumber.var3=y;
	iEffectNumber.var4=0;
	
}

public func FxHardeningSpellTimer(pTarget, iEffectNumber, iEffectTime)
{
	var xdir = iEffectNumber.var0;
	var ydir = iEffectNumber.var1;
	var x = iEffectNumber.var2;
	var y = iEffectNumber.var3;
	for(var i=0; i<4; i++)
	{
		var r = Random(360);
		var d = Random(8) + Random(5) + Random(6) + Random(6);
		CreateParticle("AirIntake", Sin(r,d) + x, -Cos(r,d) + y, xdir, ydir,16);
		CreateParticle("Air", Sin(r,d) + x, -Cos(r,d) + y, xdir, ydir,16);
	}
	if(!GBackSolid(x,y))
	{
		iEffectNumber.var2+=iEffectNumber.var0;
		iEffectNumber.var3+=iEffectNumber.var1;
		return 1;
	}
	for(var i=0; i<5; i++)
	{
		var r = Random(360);
		var d = Random(8) + Random(6) + Random(6) + Random(6)+Random(3);
		x= Sin(r,d) + iEffectNumber.var2;
		y = -Cos(r,d) + iEffectNumber.var3;
		if(GetMaterial(x,y) == Material("Snow"))
		{
			DrawMaterialQuad("Ice",x,y,x+1,y,x+1,y+1,x,y+1);
			CreateParticle("Air",x ,y ,xdir/3 ,ydir/3 ,35);
		}
	}
	if(iEffectTime > 360 || iEffectNumber.var4 > 500) { return -1; }


	
}

local Name = "$Name$";
local Description = "$Description$";
