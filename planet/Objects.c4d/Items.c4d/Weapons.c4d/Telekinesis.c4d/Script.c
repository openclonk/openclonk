/*--
	Orb of Telekinesis
	Author: Ringwaul

	Move objects with the force of magic!
--*/

#strict 2


protected func Initialize()
{
}

protected func HoldingEnabled() { return true; }

protected func ControlUseStart(object pClonk, ix, iy)
{
	return 1;
}

public func ControlUseHolding(object pClonk, ix, iy)
{
	var target=FindObject(Find_NoContainer(),Find_Not(Find_OCF(OCF_Alive)),Find_Distance(25,ix,iy), Find_Distance(150,0,0), Find_Exclude(pClonk));

	//Magical particle effect
	if(Distance(GetX(),GetY(),GetX()+ix,GetY()+iy)<150 && pClonk->GetProcedure()== "WALK")
	{
	//CastParticles("Magic",1,3,ix,iy,30,90,RGB(0,230,255),RGB(0,0,255));

	var partx=-(Sin(180-Angle(0,0,ix,iy), 100)/10);
	var party=-(Cos(180-Angle(0,0,ix,iy), 100)/10);
	if(Random(3)==1)CreateParticle("Magic",ix,iy,partx,party,50,RGB(0,220,250));
	}

	if(target!=nil)
	{	
		//Don't mess around with underground objects. This stops you from triggering flints underground by pulling on them.
		if(target->Stuck()==1) return 1;

		//magical ring effect
		CastParticles("MagicRing",1,0,ix,iy,170,175,RGB(0,230,255),RGB(0,100,255));

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
	return 1;
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
		  							