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



public func FxHardeningSpellStart(pTarget, effect, iTemp, angle, x, y)
{
	if(iTemp) return;
	effect.var0=Sin(angle,4);
	effect.var1=-Cos(angle,4);
	effect.var2=x;
	effect.var3=y;
	effect.var4=0;
	
}

public func FxHardeningSpellTimer(pTarget, effect, iEffectTime)
{
	var xdir = effect.var0;
	var ydir = effect.var1;
	var x = effect.var2;
	var y = effect.var3;
	for(var i=0; i<4; i++)
	{
		var r = Random(360);
		var d = Random(8) + Random(5) + Random(6) + Random(6);
		CreateParticle("AirIntake", Sin(r,d) + x, -Cos(r,d) + y, xdir, ydir,16);
		CreateParticle("Air", Sin(r,d) + x, -Cos(r,d) + y, xdir, ydir,16);
	}
	if(!GBackSolid(x,y))
	{
		effect.var2+=effect.var0;
		effect.var3+=effect.var1;
		return 1;
	}
	for(var i=0; i<5; i++)
	{
		var r = Random(360);
		var d = Random(8) + Random(6) + Random(6) + Random(6)+Random(3);
		x= Sin(r,d) + effect.var2;
		y = -Cos(r,d) + effect.var3;
		if(GetMaterial(x,y) == Material("Snow"))
		{
			DrawMaterialQuad("Ice",x,y,x+1,y,x+1,y+1,x,y+1);
			CreateParticle("Air",x ,y ,xdir/3 ,ydir/3 ,35);
		}
	}
	if(iEffectTime > 360 || effect.var4 > 500) { return -1; }


	
}

local Name = "$Name$";
local Description = "$Description$";
