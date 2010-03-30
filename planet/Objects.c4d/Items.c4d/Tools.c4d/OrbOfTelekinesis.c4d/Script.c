/*--
	Orb of Telekinesis
	Author: Ringwaul

	Move objects with the force of magic!
--*/

#strict 2

local hz;
local magic;

protected func Initialize()
{
	magic=0;
}

protected func HoldingEnabled() { return true; }

protected func ControlUseStart(object pClonk, ix, iy)
{
	return 1;
}

public func ControlUseHolding(object pClonk, ix, iy)
{
	if(magic==0 || GetEffect("Charge",this)) return 1;

	//Objects which can be lifted by telekinesis
	var target=FindObject(Find_Category(C4D_Object),Find_NoContainer(),Find_Not(Find_OCF(OCF_Alive)),Find_Distance(25,ix,iy), Find_Distance(150,0,0), Find_Exclude(pClonk));


	if(target!=nil && pClonk->GetProcedure()== "WALK")
	{		
		//Magical particle effect
		if(Distance(GetX(),GetY(),GetX()+ix,GetY()+iy)<150)
		{	
		var partx=-(Sin(180-Angle(0,0,ix,iy), 100)/10);
		var party=-(Cos(180-Angle(0,0,ix,iy), 100)/10);
		if(Random(3)==1)CreateParticle("Magic",ix,iy,partx,party,magic/10+50,RGB(0,220,250));
		}

		//Don't mess around with underground objects. This stops you from triggering flints underground by pulling on them.
		if(target->Stuck()==1) return 1;

		//Magic Drain
		magic=magic-1;

		//magical ring effect with frequency
		if(hz<2) hz=hz+1;
		if(hz>=2)
		{
			CastParticles("MagicRing",1,0,ix,iy,(magic*13)/50+100,(magic*13)/50+110,RGB(0,230,255),RGB(0,100,255));
			hz=0;
		}

		var angle=Angle(target->GetX(),target->GetY(),pClonk->GetX()+ix,pClonk->GetY()+iy);
		var xspeed=Sin(180-angle,10);
		var yspeed=Cos(180-angle,10);

		if(Distance(target->GetX(),target->GetY(),pClonk->GetX()+ix,pClonk->GetY()+iy)<5)
		{
			target->SetVelocity(angle,Distance(target->GetX(),target->GetY(),pClonk->GetX()+ix,pClonk->GetY()+iy));
			Message("Stop",pClonk);
			return 1;
		}
			
		target->SetXDir(GetXDir()+xspeed);
		target->SetYDir(GetYDir()+yspeed);
	}
	return 1;
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	if(magic==0 && !GetEffect("Charge",this))
	{
		AddEffect("Charge",this,150,3,this,this);
	}
	return 1;
}

protected func FxChargeTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	if(magic<500)
	{
		PlayerMessage(Contained()->GetOwner(),"%d",Contained(),magic);	
		magic=magic+5;

		CastParticles("Magic",1,10,0,0,40,90,RGB(0,100,255),RGB(255,255,255));
	}
	if(magic>=500)
	{
		magic=500;
		return -1;
	}
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}
