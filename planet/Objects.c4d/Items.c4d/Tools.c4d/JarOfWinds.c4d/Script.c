/*--
	Jar of Winds
	Author: MimmoO

	Collect air until you're full, then release it with a blast.

--*/

local Amount;
local MaxCap;
local charging;

local JarTrans;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryTransform(clonk)	{	return JarTrans; } // TODO change when ck has fixed the bug
public func GetCarryPhase() { return 600; }

protected func Initialize()
{
	MaxCap = 50; //Changes duration and power of the Jar
	SetR(-45);
	JarTrans = Trans_Translate(-1500,2000,0);
}

protected func HoldingEnabled() { return true; }

protected func ControlUseStart(object pClonk, ix, iy)
{
	Sound("WindCharge.ogg",false,nil,nil,1);
	charging = 1;
	Amount = 0;
	return 1;
}

public func ControlUseHolding(object pClonk, ix, iy)
{
	JarTrans = Trans_Mul(Trans_Translate(-1500,2000,0), Trans_Rotate(Angle(0,0,ix,iy) * (-1 + 2 * pClonk->GetDir()),0,0,1));
	// TODO a bit hacky, but i fear using animations here also leads to the bug like the musket and co. So I'll make the animation when the bug is fixed
	pClonk->UpdateAttach();
	//Angle Finder
	if(GBackSolid()) return -1;
	if(GBackLiquid()) return -1;	
	//not in material
	if(Amount <= MaxCap)
	{
		var R=RandomX(-25,25);
		var D=RandomX(19,130);
		var SX=Sin(180 - Angle(0,0,ix,iy) + R,D);
		var SY=Cos(180 - Angle(0,0,ix,iy) + R,D);
		
		if(!GBackSolid(SX,SY) && !GBackLiquid(SX,SY)) //when on a random spot in front is air...
		{
			Amount += 2; //Air is sucked in.

			Message("Loading...|%3.0d",pClonk,(Amount * 99) / MaxCap);	

			CreateParticle("AirIntake",
				SX,SY,
				Sin(180 - Angle(0,0,ix,iy) + R,-D / 2),
				Cos(180 - Angle(0,0,ix,iy) + R,-D / 2),
				RandomX(35,80),
				RGBa(255,255,255,128)
			);
		}
	}
	
	else //Is it full? Say so!
	{
		Message("Full!",pClonk);
		if(charging == 1) ChargeSoundStop();
		charging = 0;
	}
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	JarTrans = Trans_Translate(-1500,2000,0);
	if(Amount>(MaxCap/5)) //Is there enough air to fire?
	{
		// Fire	
		FireWeapon(pClonk, ix, iy);
		return 1;
	}
	ChargeSoundStop();
	charging = 0;
return 1;
}

func ControlUseCancel()
{
	ChargeSoundStop();
	charging = 0;
}

protected func ChargeSoundStop()
{
	Sound("WindCharge.ogg",false,nil,nil,-1);
	Sound("WindChargeStop.ogg");
}

private func FireWeapon(object pClonk,iX,iY)
{
	var iAngle=Angle(0,0,iX,iY);
	
	ChargeSoundStop();
	Sound("WindGust.ogg");

	//Find Victims to push
	for(var i=10; i<32; i++)
	{
		var R = RandomX(-20,20);
		var SX = Sin(180 - Angle(0,0,iX,iY) + R,i);
		var SY = Cos(180 - Angle(0,0,iX,iY) + R,i);
		
	if(!GBackSolid(SX,SY))
	{
		CreateParticle("Air",
					SX,SY,
					Sin(180 - Angle(0,0,iX,iY) + (R),(Amount / 2) + 25),
					Cos(180 - Angle(0,0,iX,iY) + (R),(Amount / 2) + 25),
					Max(i + 30, 90),
					);	
	}
	}
	
	var sinspeed = Sin(180 - Angle(0,0,iX,iY) + (R / 2),(Amount) + 15);
	var cosspeed = Cos(180 - Angle(0,0,iX,iY) + (R / 2),(Amount) + 15);
	
	if(pClonk->GetAction() != "Walk")
	{									//Makes the clonk firing it be pushed backwards a bit
		var x = pClonk->GetXDir();
		var y = pClonk->GetYDir();
		pClonk->SetXDir((x) - (sinspeed / 3));
		pClonk->SetYDir((y) - (cosspeed / 3)); 
	}
	
	for( var obj in FindObjects(
		Find_Or(
			Find_Distance(10,Sin(180 - Angle(0,0,iX,iY),20),Cos(180 - Angle(0,0,iX,iY),20)),
			Find_Distance(18,Sin(180 - Angle(0,0,iX,iY),40),Cos(180 - Angle(0,0,iX,iY),40)),
			Find_Distance(25,Sin(180 - Angle(0,0,iX,iY),70),Cos(180 - Angle(0,0,iX,iY),70))
		),
		Find_Not(Find_Category(C4D_Structure))
	)) {
		if(obj != pClonk && PathFree(pClonk->GetX(),pClonk->GetY(),obj->GetX(),obj->GetY()))
		{
		//enemys are pushed back
			var x = obj->GetXDir();
			var y = obj->GetYDir();
			obj->SetXDir((x) + sinspeed);
			obj->SetYDir((y) + cosspeed);
		}
	}
}


func Definition(def) {
	SetProperty("Name", "$Name$", def);
}