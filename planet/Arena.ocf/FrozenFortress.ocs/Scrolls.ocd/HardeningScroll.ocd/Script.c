/*--
	Scroll: Hardening
	Author: Mimmo

	Create a storm to blow away your enemies.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	AddEffect("HardeningSpell", nil, 100, 1, nil, GetID(), Angle(0,0,ix,iy),pClonk->GetX(), pClonk->GetY());
	RemoveObject();
	return 1;
}



public func FxHardeningSpellStart(pTarget, effect, iTemp, angle, x, y)
{
	if(iTemp) return;
	effect.xdir=Sin(angle,4);
	effect.ydir=-Cos(angle,4);
	effect.x=x;
	effect.y=y;
}

public func FxHardeningSpellTimer(pTarget, effect, iEffectTime)
{
	var xdir = effect.xdir;
	var ydir = effect.ydir;
	var x = effect.x;
	var y = effect.y;
	for(var i=0; i<4; i++)
	{
		var r = Random(360);
		var d = Random(8) + Random(5) + Random(6) + Random(6);
		CreateParticle("AirIntake", Sin(r,d) + x, -Cos(r,d) + y, xdir, ydir,16);
		CreateParticle("Air", Sin(r,d) + x, -Cos(r,d) + y, xdir, ydir,16);
	}
	if(!GBackSolid(x,y))
	{
		effect.x+=effect.xdir;
		effect.y+=effect.ydir;
		return 1;
	}
	for(var i=0; i<5; i++)
	{
		var r = Random(360);
		var d = Random(8) + Random(6) + Random(6) + Random(6)+Random(3);
		x= Sin(r,d) + effect.x;
		y = -Cos(r,d) + effect.y;
		if(GetMaterial(x,y) == Material("Snow"))
		{
			DrawMaterialQuad("Ice",x,y,x+1,y,x+1,y+1,x,y+1);
			CreateParticle("Air",x ,y ,xdir/3 ,ydir/3 ,35);
		}
	}
	if(iEffectTime > 360) { return -1; }

}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
