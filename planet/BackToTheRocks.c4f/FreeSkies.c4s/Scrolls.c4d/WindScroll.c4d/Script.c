/*--
	Scroll: Wind
	Author: Mimmo

	Create a storm to blow away your enemies.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	AddEffect("WindScrollStorm", 0, 100, 1, 0, GetID(), Angle(0,0,ix,iy),pClonk->GetX(), pClonk->GetY(), pClonk->GetOwner());
	RemoveObject();
	return 1;
}



public func FxWindScrollStormStart(pTarget, iEffectNumber, iTemp, angle, x, y, owner)
{
	if(iTemp) return;
	iEffectNumber.var0=Sin(angle,32);
	iEffectNumber.var1=-Cos(angle,32);
	iEffectNumber.var2=x+Sin(angle,43);
	iEffectNumber.var3=y-Cos(angle,43);
	iEffectNumber.var4=owner;

	
}

public func FxWindScrollStormTimer(pTarget, iEffectNumber, iEffectTime)
{
	var xdir=iEffectNumber.var0;
	var ydir=iEffectNumber.var1;
	var x=iEffectNumber.var2;
	var y=iEffectNumber.var3;
	
	if(iEffectTime<36)
	{
			var r=Random(360);
			var d=Random(40);
			CreateParticle("AirIntake",Sin(r,d)+x,-Cos(r,d)+y,xdir/3,ydir/3 +2,64,RGB(Random(80),100+Random(50),255));
	return 1;
	}
	else if(iEffectTime<180 ) 
	{
		for(var i=0; i<5; i++)
		{
			var r=Random(360);
			var d=Random(40);
			CreateParticle("AirIntake",Sin(r,d)+x,-Cos(r,d)+y,xdir/2,ydir/2 +2,64,RGB(Random(80),100+Random(50),255));
		}
		
		for(var obj in FindObjects(Find_Distance(40,x,y),Find_Not(Find_Category(C4D_Structure))))
		{
			if(PathFree(x,y,obj->GetX(),obj->GetY()))
			{
				if(obj->GetID() == Clonk)
					obj->SetKiller(iEffectNumber.var4);
				if(xdir<0)
				{if(obj->GetXDir() > xdir) obj->SetXDir(obj->GetXDir(100) + (xdir*3)/2,100); }
				else 
				{if(obj->GetXDir() < xdir) obj->SetXDir(obj->GetXDir(100) + (xdir*3)/2,100); }
				
				if(ydir<0)
				{if(obj->GetYDir() > ydir) obj->SetYDir(obj->GetYDir(100) + (ydir*3)/2,100); }
				else 
				{if(obj->GetYDir() < ydir) obj->SetYDir(obj->GetYDir(100) + (ydir*3)/2,100); }
			}
		}
	return 1;
	}	
	return -1;
	
	
}

local Name = "$Name$";
local Description = "$Description$";
