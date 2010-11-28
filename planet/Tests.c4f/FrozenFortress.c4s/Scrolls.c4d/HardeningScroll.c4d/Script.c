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
	EffectVar(0, pTarget, iEffectNumber)=Sin(angle,4);
	EffectVar(1, pTarget, iEffectNumber)=-Cos(angle,4);
	EffectVar(2, pTarget, iEffectNumber)=x;
	EffectVar(3, pTarget, iEffectNumber)=y;
	EffectVar(4, pTarget, iEffectNumber)=0;
	
}

public func FxHardeningSpellTimer(pTarget, iEffectNumber, iEffectTime)
{
	var xdir = EffectVar(0, pTarget, iEffectNumber);
	var ydir = EffectVar(1, pTarget, iEffectNumber);
	var x = EffectVar(2, pTarget, iEffectNumber);
	var y = EffectVar(3, pTarget, iEffectNumber);
	for(var i=0; i<4; i++)
	{
		var r = Random(360);
		var d = Random(8) + Random(5) + Random(6) + Random(6);
		CreateParticle("AirIntake", Sin(r,d) + x, -Cos(r,d) + y, xdir, ydir,16);
		CreateParticle("Air", Sin(r,d) + x, -Cos(r,d) + y, xdir, ydir,16);
	}
	if(!GBackSolid(x,y))
	{
		EffectVar(2, pTarget, iEffectNumber)+=EffectVar(0, pTarget, iEffectNumber);
		EffectVar(3, pTarget, iEffectNumber)+=EffectVar(1, pTarget, iEffectNumber);
		return 1;
	}
	for(var i=0; i<5; i++)
	{
		var r = Random(360);
		var d = Random(8) + Random(6) + Random(6) + Random(6)+Random(3);
		x= Sin(r,d) + EffectVar(2, pTarget, iEffectNumber);
		y = -Cos(r,d) + EffectVar(3, pTarget, iEffectNumber);
		if(GetMaterial(x,y) == Material("Snow"))
		{
			DrawMaterialQuad("Ice",x,y,x+1,y,x+1,y+1,x,y+1);
			CreateParticle("Air",x ,y ,xdir/3 ,ydir/3 ,35);
		}
	}
	if(iEffectTime > 360 || EffectVar(4, pTarget, iEffectNumber) > 500) { return -1; }


	
}

local Name = "$Name$";
local Description = "$Description$";
