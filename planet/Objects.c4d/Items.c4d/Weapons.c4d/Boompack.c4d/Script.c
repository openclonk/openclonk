/*--
	Boompack
	Author: Ringwaul

	A risky method of flight.
--*/

#strict 2

local angle;
local fuel;
local rider;

public func GetCarryMode(clonk) { return CARRY_Back; }
public func GetCarryTransform(clonk)	{	return Trans_Scale(2000);	}

protected func Initialize()
{
	//flight length
	fuel=100;
}

//protected func HoldingEnabled() { return true; }

protected func ControlUseStart(object pClonk, ix, iy)
{	
	if(pClonk->GetProcedure()!="WALK" && pClonk->GetProcedure()!="FLIGHT") return 1;

	if(GetEffect("Flight",this)!=nil)
	{
		pClonk->SetAction("Tumble");
		pClonk->SetVelocity(angle,20);
		return 1;
	}

	angle=Angle(0,0,ix,iy);
	if(angle>180) angle=angle-360;//sodding angles

	AddEffect("Flight",this,150,1,this,this);
	Exit();

	//Ride the rocket!
	pClonk->SetAction("Ride",this);
	rider=pClonk;
	SetProperty("Collectible",0);

	SetR(angle);
	if(Random(2)==1)
	{
		SetRDir(5);
		return 1;
	}
	SetRDir(-5);
	return 1;
}

public func ControlUseHolding(object pClonk, ix, iy)
{
	return 1;
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	//JumpOff(pClonk);
	return 1;
}

protected func FxFlightTimer(object pTarget, int iEffectNumber, int iEffectTime)
{
	if(fuel<20 && Distance(GetX(), GetY(), rider->GetX(), rider->GetY())<30)
	{
		rider->SetAction("Tumble");
		rider->SetVelocity(angle,20);
	}

	if(fuel<=0)
	{
		Explode(30);
	}

	//Shaking motion midair
	if(GetR()<angle-25) SetRDir(GetRDir()+5);
	if(GetR()>angle+25) SetRDir(GetRDir()-5);
	SetVelocity(GetR(),70);

	fuel=fuel-1;

	var sin=-Sin(180-GetR(),16);
	var cos=-Cos(180-GetR(),16);		
	if(Random(3)==1) CastParticles("NozzleFlame",1,10,sin,cos,100,160,RGB(255,255,255),RGB(200,200,200));
	CreateParticle("BurnSmoke",sin,cos,RandomX(-2,2),RandomX(-2,2),80,RGB(255,255,255));
	if(Random(3)==1) CastParticles("Spark",1,Random(30),sin,cos,30,60,RGB(255,255,0),RGB(255,200,0));
}

protected func Hit()
{
	Message("I have hit something",this);
	if(GetEffect("Flight",this)) Explode(30);
	Sound("WoodHit");

}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
  SetProperty("Collectible",1, def);
  SetProperty("PerspectiveR", 4500, def);
  SetProperty("PerspectiveTheta", 25, def);
  SetProperty("PerspectivePhi", 30, def);
}
		  							