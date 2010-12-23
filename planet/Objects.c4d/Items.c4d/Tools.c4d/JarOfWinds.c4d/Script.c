/*--
	Jar of Winds
	Author: MimmoO

	Collect air until you're full, then release it with a blast.

--*/

local Amount;
local MaxCap;
local sound;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 600; }

public func FxJarReloadTimer(object target, effect, int time)
{
	target->Load();
}

public func DoFullLoad()
{
	Amount = MaxCap;
	return;
}

protected func Initialize()
{
	MaxCap = 60; //Changes duration and power of the Jar
	SetR(-45);
	AddEffect("JarReload",this,100,2,this);
	sound=false;
}

protected func ControlUse(object pClonk, iX, iY)
{
	if(CheckCanUse(pClonk) == false) return 1;

	if(!GetEffect("JarReload",this))
	{
		if(!GBackLiquid())
		{
			FireWeapon(pClonk, iX, iY);
			Amount=0;
			AddEffect("JarReload",this,100,1,this);
			Sound("WindCharge.ogg",false,nil,nil,1);
			sound=true;
		}
		
		return true;
	}
	else
	{
		pClonk->Message("Reloading!");
		return true;
	}
//	ChargeSoundStop();
}

protected func Load()
{
	
	if(Amount <= MaxCap)
	{
		var R=RandomX(-25,25);
		var D=RandomX(19,50);
		var A=Random(360);
		var SX=Sin(A + R,D);
		var SY=Cos(A + R,D);
		
		if(!GBackSolid(SX,SY) && !GBackLiquid(SX,SY) && !GBackSolid(0,0) && !GBackLiquid(0,0)) //when on a random spot in front is air...
		{
			if(!sound)
			{
				Sound("WindCharge.ogg",false,nil,nil,1);
				sound=true;
			}
			Amount += 2; //Air is sucked in.
			CreateParticle("AirIntake",
				SX,SY,
				Sin(A + R,-D / 2),
				Cos(A + R,-D / 2),
				RandomX(35,80),
				RGBa(255,255,255,128)
			);
		}
		else if(GBackSolid(0,0) || GBackLiquid(0,0))
		{
			if(sound)
			ChargeSoundStop();
		}

	}
	else
	{
		RemoveEffect("JarReload",this);
		ChargeSoundStop();
		
	}
}

protected func ChargeSoundStop()
{
	Sound("WindCharge.ogg",false,nil,nil,-1);
	Sound("WindChargeStop.ogg");
	sound=false;
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
					Max(i + 30, 90) + 75,
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
		Find_Not(Find_Category(C4D_Structure)),
		Find_Not(Find_Func("NoWindjarForce")),
		Find_Layer(GetObjectLayer()), Find_NoContainer()
								)
		)
		{
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

private func CheckCanUse(object pClonk)
{
	if(pClonk->GetProcedure() == "ATTACH") return false;
	else true;
}

local Name = "$Name$";
local Description = "$Description$";
