/*-- Bubble --*/

local time;
local alpha;

global func Bubble(int iamount, int x, int y)
{
	if(iamount==nil || iamount==0) iamount=3;

	var i = 0;
	while(i<iamount)
	{
		CreateObject(Bubble1, x, y);
		++i;
	}
}

protected func Initialize()
{
	alpha=255;
	DoCon(RandomX(25,100));
	AddEffect("Move",this,1,1,this);
}

public func FxMoveTimer(pTarget, iEffectNumber, iEffectTime)
{
	if(GBackLiquid(0,-3)==false && !GetEffect("Fade",this) || iEffectTime > 108)
	{
		AddEffect("Fade",pTarget,1,1,pTarget);
	}

	//Bubbles burst into smaller bubles
	if(Random(30)==1 && pTarget->GetCon()>100)
	{
		var i=3;
		while(i>0)
		{
			i=--i;
			var bubble=CreateObject(Bubble1);
			bubble->SetCon(pTarget->GetCon()/15*10);
			bubble->SetYDir(pTarget->GetYDir());
		}
		RemoveObject();
		return -1;
	}

	SetYDir(GetYDir() - 2 + Random(5));

	//Jittery movement
	var dir;
	if(Random(2)==1) dir=-1;
	else	dir=1;
	if(GetXDir()<6 && GetXDir()>-6)
	{
		SetXDir(GetXDir()+dir);
	}
}


public func FxFadeTimer(pTarget)
{
	if(alpha<=0)
	{
		RemoveEffect("Move",this);
		RemoveObject();
		return -1;
	}
	SetClrModulation(RGBa(255,255,255,alpha));
	alpha=alpha-5;
}