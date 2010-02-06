/*--
	Jar of Winds
	Author: MimmoO

	Collect air until youre full, then release it with a blast.

--*/

#strict 2

local ReloadTimer;
local Loaded;
local Amount;
local MaxCap;

local yOffset;
local iBarrel;

protected func Initialize()
{
	MaxCap=50; //Changes duration and power of the Jar
	SetR(-45);
}

protected func HoldingEnabled() { return true; }

protected func ControlUseStart(object pClonk, ix, iy)
{
	Amount=0;
	return 1;
}

public func ControlUseHolding(object pClonk, ix, iy)
{
	//Angle Finder
	if(GBackSolid()) return -1;
	if(GBackLiquid()) return -1;	
	//not in material
	if(Amount<=MaxCap)
	{
	
		var R=RandomX(-25,25);
		var D=RandomX(19,130);
		var SX=Sin(180-Angle(0,0,ix,iy)+R,D,);
		var SY=Cos(180-Angle(0,0,ix,iy)+R,D);
		
		if(!GBackSolid(SX,SY) && !GBackLiquid(SX,SY)) //when on a random spot in frotn is air...
	 	{
	 	Amount+=2; 				//Air is sucked in.
	 
	 	Message("Loading...|%3.0d",pClonk,(Amount*99)/MaxCap);	
	 
	 	CreateParticle("Air",
					SX,SY,
					Sin(180-Angle(0,0,ix,iy)+R,-D/2),
					Cos(180-Angle(0,0,ix,iy)+R,-D/2),
					RandomX(35,80),
					RGBa(255,255,255,128)
					);
 		}
 	}
 	
 	else //were full? say it!
 	{
 		Message("Full!",pClonk);	
 	}

    
}

protected func ControlUseStop(object pClonk, ix, iy)
{
	
	if(Amount>(MaxCap/5)) //can fire, enough air in?
	{
	 // Fire	
		FireWeapon(pClonk, ix, iy);
		return 1;	 
	}
	else Message("Not.",pClonk);	
	//Sound(" :-( ");
return 1;
}




private func FireWeapon(object pClonk,iX,iY)
{
	
	var iAngle=Angle(0,0,iX,iY);

	Message("Bang!", pClonk); //For debug.

	//Find Victims to push
	for(var i=10; i<32; i++)
	{
		
		var R=RandomX(-20,20);
		var SX=Sin(180-Angle(0,0,iX,iY)+R,i);
		var SY=Cos(180-Angle(0,0,iX,iY)+R,i);
		
	 if(!GBackSolid(SX,SY))
	 {
	 CreateParticle("Air",
					SX,SY,
					Sin(180-Angle(0,0,iX,iY)+(R),(Amount/2)+25),
					Cos(180-Angle(0,0,iX,iY)+(R),(Amount/2)+25),
					Max(i,60),
					);	
		
	 }
	}	
	
	var sinspeed=Sin(180-Angle(0,0,iX,iY)+(R/2),(Amount)+15);
	var cosspeed=Cos(180-Angle(0,0,iX,iY)+(R/2),(Amount)+15);
	
	if(pClonk->GetAction() != "Walk")
	{									//Makes the clonk be pushed backwards a bit
		var x=pClonk->GetXDir();
		var y=pClonk->GetYDir();
		pClonk->SetXDir((x)-(sinspeed/3));
		pClonk->SetYDir((y)-(cosspeed/3)); 
	}
	
	
	for( var obj in FindObjects(
					Find_Or(
						Find_Distance(10,Sin(180-Angle(0,0,iX,iY),20),Cos(180-Angle(0,0,iX,iY),20)),
						Find_Distance(18,Sin(180-Angle(0,0,iX,iY),40),Cos(180-Angle(0,0,iX,iY),40)),
						Find_Distance(25,Sin(180-Angle(0,0,iX,iY),70),Cos(180-Angle(0,0,iX,iY),70))
						   )
										
					     )
	   )
	   
	
	   
	{
		if(obj!=pClonk && PathFree(pClonk->GetX(),pClonk->GetY(),obj->GetX(),obj->GetY()))
	    {
		  var x=obj->GetXDir();
		  var y=obj->GetYDir();
		  obj->SetXDir((x)+sinspeed);
		  obj->SetYDir((y)+cosspeed); 
		  									//enemys are pushed back	
	    }	
	}
}







func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
		  									//enemys are pushed back	



