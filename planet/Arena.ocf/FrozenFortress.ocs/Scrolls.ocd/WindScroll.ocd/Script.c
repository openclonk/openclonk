/*--
	Scroll: Wind
	Author: Mimmo

	Create a storm to blow away your enemies.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	AddEffect("WindScrollStorm", nil, 100, 1, nil, GetID(), Angle(0, 0, ix, iy),pClonk->GetX(), pClonk->GetY());
	RemoveObject();
	return 1;
}



public func FxWindScrollStormStart(pTarget, effect, iTemp, angle, x, y)
{
	if (iTemp) return;
	effect.xdir = Sin(angle, 32);
	effect.ydir=-Cos(angle, 32);
	effect.x = x + Sin(angle, 43);
	effect.y = y-Cos(angle, 43);

	effect.particles =
	{
		Prototype = Particles_Air(),
		Size = PV_Random(2, 5)
	};
}

public func FxWindScrollStormTimer(pTarget, effect, iEffectTime)
{
	var xdir = effect.xdir;
	var ydir = effect.ydir;
	var x = effect.x;
	var y = effect.y;
	
	if (iEffectTime<36)
	{
		var r = Random(360);
		var d = Random(40);
		CreateParticle("Air", Sin(r, d)+x,-Cos(r, d)+y, xdir/3, ydir/3, PV_Random(10, 30), effect.particles, 1);
		return 1;
	}
	else if (iEffectTime<180 ) 
	{
		CreateParticle("Air", PV_Random(x - 20, x + 20), PV_Random(y - 20, y + 20), xdir/2, ydir/2, PV_Random(10, 30), effect.particles, 5);
		for (var obj in FindObjects(Find_Distance(40, x, y),Find_Not(Find_Category(C4D_Structure))))
		{
			if (PathFree(x, y, obj->GetX(),obj->GetY()))
			{
				if (xdir<0)
				{if (obj->GetXDir() > xdir) obj->SetXDir(obj->GetXDir(100) + (xdir*3)/2, 100); }
				else 
				{if (obj->GetXDir() < xdir) obj->SetXDir(obj->GetXDir(100) + (xdir*3)/2, 100); }
				
				if (ydir<0)
				{if (obj->GetYDir() > ydir) obj->SetYDir(obj->GetYDir(100) + (ydir*3)/2, 100); }
				else 
				{if (obj->GetYDir() < ydir) obj->SetYDir(obj->GetYDir(100) + (ydir*3)/2, 100); }
			}
		}
	return 1;
	}	
	return -1;
	
	
}

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
